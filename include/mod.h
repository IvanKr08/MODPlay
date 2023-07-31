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
    uint32 progress;
    bool playing;
    Note note;
} Channel;

typedef struct {
    uint8       name[21];       //Song name
    uint8       magic[5];       //File format

    Pattern*    patterns;       //Patterns array
    uint8       patternCount;   //Count of patterns
    uint8       pattern;        //Current pattern

    uint8       positions[128]; //Song map
    uint8       positionCount;  //Song length
    uint8       position;       //Position in song map (Song progress)

    InstrSample samples[32];    //Samples

    Channel     channels[4];    //Channels data

    uint8       row;            //Position in pattern
    uint32      leftToMix;      //How much is left to mix
    uint32      tempo;          //Current ticks per second
    uint32      ticksPerRow;    //Current ticks per row
} Song;

extern Song song;

Note getNote(uint8 channel);
void fillBuffer(LRSample* buff, HWAVEOUT h);
void loadSong(wstr fileName);