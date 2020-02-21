@echo off
setlocal
set pwd="%~dp0"
REM call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" > nul
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat" > nul
cd /d %pwd%

set SOURCES=win32_reader.c
set PROGRAM=reader

if not exist ..\bin mkdir ..\bin

cl -analyze -nologo -W4 -Od -Zi -DMAIN -D_DEBUG -D_CRT_SECURE_NO_WARNINGS -o ..\bin\%PROGRAM%.exe %SOURCES%
