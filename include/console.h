#pragma once
#include "utils.h"
#include "mod.h"
#include "tables.h"

void consoleInit();

void setConsoleTitle(wstr title);

void printI(int32 value);
void printC(char value);
void printS(cstr string);
void printS(cstr string);
void printW(wstr string);
void printFormat(cstr format, uint32 count, ...);

void printRow();