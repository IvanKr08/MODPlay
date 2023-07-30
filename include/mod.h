#pragma once
#include "utils.h"

#pragma pack(push, 1)
typedef struct {
    uint8 l;
    uint8 r;
} LRSample;
#pragma pack(pop)

typedef struct {
    uint16 period;
    uint8  sample;
    uint8  effect;
    uint8  effectArg;
} Note;

typedef Note Pattern[256];

typedef struct {
    uint32 length;
    uint8* data;
    uint8  name[23];
    uint8  finetune;
    uint8  volume;
} InstrSample;

typedef struct {
    int32 progress;
    Note note;
} Channel;

typedef struct {
    Pattern*    patterns;       //Patterns
    InstrSample samples[32];    //Samples
    uint8       positions[128]; //Song map
    uint8       name[21];       //Song name
    uint8       magic[5];       //File format
    uint8       positionCount;  //Count of used positions
    //===
    Channel     channels[4];    //Channels data
    uint8       patternCount;   //Count of patterns
    uint8       position;       //Position in song map
    uint8       pattern;        //Current pattern
    uint8       row;            //Position in pattern
} Song;

extern Song song;

Note getNote(uint8 channel);
void fillBuffer(LRSample* buff, HWAVEOUT h);
void loadSong(wstr fileName);