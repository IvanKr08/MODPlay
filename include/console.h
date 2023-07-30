#pragma once
#include "utils.h"

void initConsole();

void printI(int32 value);
void printC(char value);
void printS(cstr string);
void printFormat(cstr format, uint32 count, ...);

void printRow();