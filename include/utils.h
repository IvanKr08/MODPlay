#pragma once
#include <stdio.h>
#include <stdlib.h>

typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64; //Четверные слова появились только в С99, но поддерживаются в Open Watcom и VS6
typedef signed   __int8  int8;
typedef signed   __int16 int16;
typedef signed   __int32 int32;
typedef signed   __int64 int64;  //Тут и так полно нестандартных для C89 вещей (Комментарии, объявления...), так что в любом случае нестандарт

typedef int8            bool;
typedef char*           cstr;
typedef wchar_t*        wstr;

typedef int32           mixsample;
typedef int16           apisample;

#define A_AMPLIFY         1024
#define A_SILENCECONST    0                            //0 for signed, 128 for uint8, 32768 for uint16
#define A_SAMPLEDEPTH     sizeof(apisample)            //Bytes per mono sample
#define A_CHANNELS        2                            //Only stereo output is supported
#define A_SAMPLERATE      48000                        
#define A_BUFFERSIZE      9600                         //In bytes
#define A_SAMPLESIZE     (A_CHANNELS * A_SAMPLEDEPTH)  //Full 
#define A_BUFFERSAMPLES  (A_BUFFERSIZE / A_SAMPLESIZE) //Audio samples per buffer

#define DIV2(val)   ((val) >> 1)
#define DIV4(val)   ((val) >> 2)
#define DIV8(val)   ((val) >> 3)
#define DIV16(val)  ((val) >> 4)
#define DIV64(val)  ((val) >> 6)
#define DIV256(val) ((val) >> 8)
#define DIV1K(val)  ((val) >> 10)
#define DIV4K(val)  ((val) >> 12)
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
#define MUL4K(val)  ((val) << 12)
#define MUL16K(val) ((val) << 14)
#define MUL32K(val) ((val) << 15)
#define MUL64K(val) ((val) << 16)

#define ARRSIZE(arr) (sizeof(arr) / sizeof(*(arr)))

//https://stackoverflow.com/questions/19785518/is-dereferencing-null-pointer-valid-in-sizeof-operation
//Вроде обещают, что sizeof является интринсиком, так что настоящего разыменование нет
#define ARRALLOC(target, count)     target = memAlloc((count) * sizeof(*target)) 
#define ARRALLOCZERO(target, count) target = memAlloc((count) * sizeof(*target)); memset(target, 0, (count) * sizeof(*target));

#define ABS(x) ((x) >= 0) ? ((x)) : ((-x))
#define CLAMP(val, low, high) if ((val) < (low)) (val) = (low); else if ((val) > (high)) (val) = (high)
#define LIMIT(val, high) if ((val) > (high)) (val) = (high) //Same as CLAMP(val, 0, high), but for unsigned values

#if A_CHANNELS != 2
#error Non-stereo output is not supported!
#endif

void  warn(cstr msg);
void  fatal(bool val, cstr msg);
void  fatalCode(uint32 val, cstr msg);

void *memAlloc(size_t amount);
void  memFree(void *ptr);