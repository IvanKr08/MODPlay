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
    uint8 volume;
} Channel;

typedef struct {
    uint8       name[21];         //Song name
    uint8       magic[5];         //File format

    Pattern*    patterns;         //Patterns array
    uint8       patternCount;     //Count of patterns
    uint8       pattern;          //Current pattern
    uint8       row;              //Position in pattern

    uint8       positions[128];   //Song map
    uint8       positionCount;    //Song length
    uint8       position;         //Position in song map (Song progress)

    InstrSample samples[32];      //Samples
    Channel     channels[4];      //Channels data
    uint32      ticker;           //Global tick counter

    uint32      tempo;            //Current tempo
    uint32      ticksRow;         //Current ticks per row

    uint32      alreadyRendered;  //How much has been rendered for this tick
    uint32      ticksPerSecond;   //Ticks per second
} Song;
//How much is left to mix
extern Song song;

Note getNote(uint8 channel);
void fillBuffer(LRSample* buff, HWAVEOUT h);
void loadSong(wstr fileName);