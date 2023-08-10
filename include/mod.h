#pragma once
#include "utils.h"
#include "console.h"
#include "fx.h"

//typedef uint16 FREQ; //Frequency typedef
//typedef uint8 NOTEID;  //Note ID (Ex. C-4, G#5, no note) typedef

#pragma pack(push, 1)
typedef struct {
    int8 l;
    int8 r;
} LRSample;
#pragma pack(pop)

typedef struct {
    uint32 length;
    uint32 repeatStart;
    uint32 repeatEnd;
    uint8  finetune;
    uint8  volume;
    int8*  data;
    char   name[23];
} InstrSample;

typedef struct {
    uint8  sample;
    uint8  note;
    uint8  effect;
    uint8  effectArg;
} Note;

typedef Note Pattern[256];

typedef struct {
    //Playback
    uint8        channelID;
    bool         playing;       //Playing state
    uint32       progress;      //Sample progress (i << 16)
    uint16       basePeriod;    //Finetuned note
    uint32       currentFreq;   //
    uint32       currentStep;
    uint8        sample;
    //InstrSample* currentSample; //

    //FX
    uint8        effect;
    uint8        effectArg;
    uint8        volume;        //Current volume (Set from sample if row sample != 0)
    uint8        finetune;      //(E-5X) Last finetune (Set from sample if row sample != 0)
    uint8        lastOffset;    //(9-XX) Last offset
    //Tone portamento (Slide to note)
    //int8        tpDir;
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
    uint8       resetPos;

    //Rendering
    Channel     channels[4];      //Channels data
    uint32      tickRenderCount;  //How much samples has been rendered for this tick
    uint32      bpt;              //Bytes per tick

    //Playback speed
    uint32      ticker;           //Global counter
    uint32      tps;              //Ticks per second
    uint8       tpr;              //Ticks per row (F-(XX < 32)). Default: 6
    uint8       tempo;            //Tempo. Default: 125 (Should update tps)
    
    //Other
    InstrSample samples[32];      //Samples
} Song;

extern Song song;

InstrSample* getSample(uint8 sample);
Note         getNote(uint8 channel);
Channel*     getChannel(uint8 channel);

void         fillBuffer(LRSample* buff, HWAVEOUT h);
void         loadSong(wstr fileName);

//InstrSample* sample;      //Pointer to the current sample
//InstrSample* nextSample;  //Pointer to the next sample (Used when sample specified without a note)