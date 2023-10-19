#pragma once
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;
typedef signed   char   int8;
typedef signed   short  int16;
typedef signed   int    int32;
typedef int32           bool;
typedef char*           cstr;
typedef wchar_t*        wstr;

void* memAlloc(uint32 count);
void fatal(bool val, cstr msg);

#pragma region Shift MUL/DIV
#define DIV2(val) ((val) >> 1)
#define DIV4(val) ((val) >> 2)
#define DIV8(val) ((val) >> 3)
#define DIV16(val) ((val) >> 4)
#define DIV64(val) ((val) >> 6)
#define DIV256(val) ((val) >> 8)
#define DIV16K(val) ((val) >> 14)
#define DIV32K(val) ((val) >> 15)
#define DIV64K(val) ((val) >> 16)

#define MUL2(val) ((val) << 1)
#define MUL4(val) ((val) << 2)
#define MUL8(val) ((val) << 3)
#define MUL16(val) ((val) << 4)
#define MUL64(val) ((val) << 6)
#define MUL256(val) ((val) << 8)
#define MUL16K(val) ((val) << 14)
#define MUL32K(val) ((val) << 15)
#define MUL64K(val) ((val) << 16)
#pragma endregion

#define A_SR  (44100)       //SampleRate (11025 | 22050 | 44100)
#define A_BPB (4410)        //Bytes per buffer
#define A_SPB (DIV2(A_BPB)) //Samples per buffer