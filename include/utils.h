#pragma once
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

typedef int32           mixsample;
typedef int16           apisample;

#define DIV2(val)   ((val) >> 1)
#define DIV4(val)   ((val) >> 2)
#define DIV8(val)   ((val) >> 3)
#define DIV16(val)  ((val) >> 4)
#define DIV64(val)  ((val) >> 6)
#define DIV256(val) ((val) >> 8)
#define DIV1K(val)  ((val) >> 10)
#define DIV16K(val) ((val) >> 14)
#define DIV32K(val) ((val) >> 15)
#define DIV64K(val) ((val) >> 16)

#define MUL2(val)   ((val) << 1)
#define MUL4(val)   ((val) << 2)
#define MUL8(val)   ((val) << 3)
#define MUL16(val)  ((val) << 4)
#define MUL64(val)  ((val) << 6)
#define MUL256(val) ((val) << 8)
#define MUL512(val) ((val) << 9)
#define MUL1K(val)  ((val) << 10)
#define MUL16K(val) ((val) << 14)
#define MUL32K(val) ((val) << 15)
#define MUL64K(val) ((val) << 16)

static int A_AMPLIFY = 2024;
#define A_SILENCECONST    0 //0 for signed, 128 for uint8, 32768 for uint16
#define A_SAMPLEDEPTH     sizeof(apisample)
#define A_CHANNELS        2
#define A_SAMPLERATE      48000
#define A_BUFFERSIZE      9600 //In bytes
#define A_SAMPLESIZE     (A_CHANNELS * A_SAMPLEDEPTH)
#define A_BUFFERSAMPLES  (A_BUFFERSIZE / A_SAMPLESIZE) //Audio samples per buffer

#define ARRSIZE(arr) (sizeof(arr) / sizeof(*(arr)))
#define ARRALLOC(target, count) ((target) = memAlloc((count) * sizeof(*(target))))
#define ABS(x) ((x) >= 0) ? ((x)) : ((-x))
#define INLINE __forceinline

#if A_CHANNELS != 2
#error Non-stereo output is not supported!
#endif

void* memAlloc(uint32 amount);
void fatal(bool val, cstr msg);
void fatalCode(uint32 val, cstr msg);