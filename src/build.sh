#!/usr/bin/env sh

SOURCES=posix_hurst.c
PROGRAM=hurst
INCLUDE=~/include

mkdir -p ../bin
gcc -Wall -g -Og -DMAIN -I$INCLUDE -o ../bin/$PROGRAM $SOURCES
