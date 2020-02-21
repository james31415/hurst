#define _XOPEN_SOURCE
#include <stdlib.h>
#include "csvreader.h"

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

struct quote_list
{
  struct tm Date;
  double Close;

  struct quote_list* Next;
};

struct position
{
    u32 Row;
    u32 Column;
};

struct user_data
{
  struct position* Position;
  struct quote_list* QuoteList;
  struct quote_list* Working;
};

ROW_CALLBACK(UpdateRow)
{
    struct user_data* LocalUserData = (struct user_data*)UserData;
    struct position* pos = LocalUserData->Position;
    ++pos->Row;
    pos->Column = 0;

    return 1;
}

struct quote_list*
add_quote(struct quote_list* head, struct quote_list* working) {
  struct quote_list* temp = (struct quote_list*)malloc(sizeof(*temp));
  temp->Date = working->Date;
  temp->Close = working->Close;
  temp->Next = NULL;

  if (head == NULL) {
    head = temp;
  } else {
    struct quote_list* p = head;
    while (p->Next != NULL) {
      p = p->Next;
    }
    p->Next = temp;
  }

  return head;
}

FIELD_CALLBACK(PrintField)
{
    struct user_data* LocalUserData = (struct user_data*)UserData;
    struct position* pos = LocalUserData->Position;
    if (pos->Column == 0) { // Date
      struct tm tm;
      strptime(Field, "%Y%m%d", &tm);

      LocalUserData->Working->Date = tm;
    } else if (pos->Column == 5) { // Close
      double Close = atof(Field);
      LocalUserData->Working->Close = Close;

      LocalUserData->QuoteList = add_quote(LocalUserData->QuoteList, LocalUserData->Working);
    }

    ++pos->Column;
}

u64
GetFileSize(const char* FileName) {
  u64 Result = 0;

  struct stat buffer;
  int status = stat(FileName, &buffer);
  if (status == 0) {
    Result = (u64)buffer.st_size;
  }

  return Result;
}

u8*
ReadEntireFile(const char* FileName) {
  FILE* File = fopen(FileName, "r");
  if (File == NULL) {
    printf("Could not open %s\n", FileName);
    exit(1);
  }

  printf("Successfully opened %s\n", FileName);

  u64 FileSize = GetFileSize(FileName);

  u8* Result = (u8*)malloc(FileSize + 1);
  if (!Result) {
    printf("malloc failed! %s (%d)", __FILE__, __LINE__);
    exit(1);
  }
  fread(Result, FileSize, 1, File);
  Result[FileSize] = 0;

  fclose(File);

  return Result;
}

#ifdef MAIN
int main(int argc, char* argv[])
{
  struct position Position = {0};
  struct quote_list QuoteList = {0};
  struct quote_list Working = {0};
  struct user_data UserData = {0};
  UserData.Position = &Position;
  UserData.QuoteList = &QuoteList;
  UserData.Working = &Working;

  struct reader Reader = {0};

  Reader.FieldCallback = PrintField;
  Reader.RowCallback = UpdateRow;
  Reader.UserData = &UserData;

  if (argc < 2)
  {
    printf("Need filename");
    return -1;
  }

  const char* FileName = argv[1];
  printf("File: %s\n", FileName);

  Reader.Buffer = ReadEntireFile(FileName);
  Reader.BufferSize = (u32)strlen((const char*)Reader.Buffer);

  ParseCsvFile(&Reader);

  free(Reader.Buffer);

  struct quote_list* node = &QuoteList;
  while (node) {
    char Buffer[256];
    strftime(Buffer, 256, "%d - %m - %Y", &node->Date);
    printf("%s: %g\n", Buffer, node->Close);

    node = node->Next;
  }

  return 0;
}
#endif
