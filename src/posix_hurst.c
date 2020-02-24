#define _XOPEN_SOURCE
#include <stdlib.h>
#include "csvreader.h"

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

#include <math.h>

typedef float r32;
typedef double r64;

struct quote_list
{
  struct tm Date;
  r64 Close;

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
      r64 Close = atof(Field);
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

u32
LargestPowerOfTwo(u32 Number)
{
  if (Number < 1)
  {
    return 0;
  }

  u32 Result = 1;
  for (u32 i = 0; i < sizeof(u32); ++i)
  {
    u32 Current = 1 << i;

    if (Current > Number)
      break;

    Result = Current;
  }

  return Result;
}

r64 Log(r64 Number)
{
  return log(Number);
}

u32
ComputeLogReturns(r64* Data, u32 NumberOfDataPoints, r64** LogReturns)
{
  u32 NumberOfLogReturns = NumberOfDataPoints - 1;
  *LogReturns = malloc(NumberOfLogReturns * sizeof(r64));
  r64* LogReturn = *LogReturns;
  for (u32 DataIndex = 0; DataIndex < NumberOfLogReturns; ++DataIndex)
  {
    *LogReturn = Log(Data[DataIndex + 1] / Data[DataIndex]);
    ++LogReturn;
  }

  return NumberOfLogReturns;
}

struct linear_params
{
  r64 Slope;
  r64 Intercept;
};

void
LinearRegress(r64* X, r64* Y, u32 N, struct linear_params* Params)
{
  r64 sum_xy = 0.0;
  r64 sum_x = 0.0;
  r64 sum_x2 = 0.0;
  r64 sum_y = 0.0;

  for (u32 Index = 0; Index < N; ++Index)
  {
    sum_xy += X[Index] * Y[Index];
    sum_x += X[Index];
    sum_x2 += X[Index] * X[Index];
    sum_y += Y[Index];
  }

  Params->Slope = (sum_xy - sum_y * sum_x / N) / (sum_x2 - sum_x * sum_x / N);
  Params->Intercept = sum_y / N - Params->Slope * sum_x / N;
}

u32
RemoveAR1(r64* LogReturns, u32 NumberOfLogReturns, r64** RescaledData)
{
  u32 NumberOfRescaledData = NumberOfLogReturns - 1;

  struct linear_params Params;
  LinearRegress(LogReturns, LogReturns + 1, NumberOfRescaledData, &Params);
  r64 c = Params.Intercept;
  r64 m = Params.Slope;

  *RescaledData = malloc(NumberOfRescaledData * sizeof(r64));
  r64* RescaledDatum = *RescaledData;
  for (u32 DataIndex = 0; DataIndex < NumberOfRescaledData; ++DataIndex)
  {
    *RescaledDatum = LogReturns[DataIndex + 1] - (c + m * LogReturns[DataIndex]);
    ++RescaledDatum;
  }

  return NumberOfRescaledData;
}

r64
ComputeHurstExponent(r64* Data, u32 NumberOfDataPoints)
{
  r64* LogReturns;
  u32 NumberOfLogReturns = ComputeLogReturns(Data, NumberOfDataPoints, &LogReturns);

  r64* RescaledData;
  /* u32 NumberOfRescaledData = */ RemoveAR1(LogReturns, NumberOfLogReturns, &RescaledData);

  r64 HurstExponent = 0.0;
  return HurstExponent;
}

u32
PrepareData(struct quote_list* Head, r64** Data)
{
  u32 DataSize = 0;
  struct quote_list* Node = Head;
  while (Node) {
    ++DataSize;

    Node = Node->Next;
  }

  *Data = malloc(DataSize * sizeof(r64));
  if (*Data == NULL) {
    printf("PrepareData: Failed to alloc\n");
    exit(1);
  }

  r64* Datum = *Data;

  Node = Head;
  while (Node)
  {
    *Datum = Node->Close;

    ++Datum;
    Node = Node->Next;
  }

  return DataSize;
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

  r64* Quotes = 0;
  u32 QuoteSize = PrepareData(&QuoteList, &Quotes);

  r64 HurstExponent = ComputeHurstExponent(Quotes, QuoteSize);
  printf("Hurst exponent: %g\n", HurstExponent);

  return 0;
}
#endif
