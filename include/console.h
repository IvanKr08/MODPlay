#pragma once
#include "utils.h"
#include "mod.h"
#include "tables.h"

void initConsole();

void printI(int32 value);
void printC(char value);
void printS(cstr string);
void printFormat(cstr format, uint32 count, ...);
void printSystem(cstr format, uint32 count, ...);

void printRow();