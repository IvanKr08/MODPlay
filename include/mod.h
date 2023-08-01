#pragma once
#include "utils.h"
#include "console.h"
#include "fx.h"

#pragma pack(push, 1)
typedef struct {
    uint8 l;
    uint8 r;
} LRSample;
#pragma pack(pop)

typedef struct {
    uint32 length;
    uint8* data;
    uint8  name[23];
    uint8  finetune;
    uint8  volume;
    uint8  num;
} InstrSample;

typedef struct {
    uint8 sample;
    uint16 period;
    uint8  effect;
    uint8  effectArg;
} Note;

typedef Note Pattern[256];

typedef struct {
    uint32       progress;    //Sample progress (X << 16)
    uint8        volume;      //Channel volume
    InstrSample* sample;      //Pointer to the current sample
    InstrSample* nextSample;  //Pointer to the next sample (Used when sample specified without a note)
    bool         playing;     //Playing state
    Note note;
} Channel;

typedef struct {
    //Strings
    uint8       name[21];         //Song name
    uint8       magic[5];         //File format

    //Patterns
    Pattern*    patterns;         //Patterns array
    uint8       patternCount;     //Count of patterns
    uint8       pattern;          //Current pattern
    uint8       row;              //Position in pattern

    //Song map
    uint8       positions[128];   //Song map
    uint8       positionCount;    //Song length
    uint8       position;         //Position in song map (Song progress)

    //Rendering
    Channel     channels[4];      //Channels data
    uint32      tickRenderCount;    //How much has been rendered for this tick

    //Playback speed
    uint32      ticker;           //Global counter
    uint32      tps;              //Ticks per second
    uint8       tpr;              //Ticks per row (F-(XX < 32)). Default: 6
    uint8       tempo;            //Tempo. Default: 125 (Should update tps)
    
    //Other
    InstrSample samples[32];      //Samples
} Song;

//How much is left to mix
extern Song song;

InstrSample* getSample(uint8 sample);
Note         getNote(uint8 channel);
Channel*     getChannel(uint8 channel);

void         fillBuffer(LRSample* buff, HWAVEOUT h);
void         loadSong(wstr fileName);