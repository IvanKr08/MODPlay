#pragma once
#include <Windows.h>
#include "utils.h"
#include "console.h"

#pragma pack(push, 1)
typedef struct {
    int16 l;
    int16 r;
} LRSample;
#pragma pack(pop)

typedef struct {
    uint32 length;
    uint32 loopStart;
    uint32 loopEnd;
    uint8  finetune;
    uint8  volume;
    int8*  data;
    char   name[23];
    bool   hasLoop;
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
    bool        playing;            //Playing state
    uint32      progress;           //Sample progress (<< 16)
    uint16      basePeriod;         //Finetuned note period
    uint32      currentFreq;        //Precalculated frequency (Remember to update with any period change!)
    uint32      currentStep;        //Step in src array in resampler
    uint8       sample;             //Sample num

    uint8       qSample;            //Sample to be used in the next notes
    uint8       qFinetune;          //Finetune to be used in the next notes

    //FX
    uint8       effect;             //Effect num
    uint8       effectArg;          //Effect arg
    uint8       finetune;           //(E-5X) Last finetune (Set from sample if row sample != 0)
    uint8       arpBaseNote;        //(0-XX) Arpeggio base note

    uint8       volume;             //Current volume (Set from sample if row sample != 0)

    int32       portamento;         //(1-XX) (2-XX) (3-XX) Period shift (<< 4)
    uint16      targetPeriod;       //(3-XX) Tone portamento target period (0 if reached)

    //Memory
    uint8       offsetMem;          //(9-00) Last offset
    uint8       tonePortMem;        //(3-00) Tone portamento memory
} Channel;

typedef struct {
    //Strings
    uint8       name[21];           //Song name
    uint8       magic[5];           //File format

    //Patterns
    Pattern*    patterns;           //Patterns array
    uint8       patternCount;       //Count of patterns
    uint8       pattern;            //Current pattern
    uint8       row;                //Position in pattern

    //Song map
    uint8       positions[128];     //Song map
    uint8       positionCount;      //Song length
    uint8       position;           //Position in song map (Song progress)
    uint8       resetPos;

    //Rendering
    Channel     channels[4];        //Channels data
    uint32      tickRenderCount;    //How much samples has been rendered for this tick
    uint32      spt;                //Samples per tick

    //Playback speed
    uint32      ticker;             //Global counter
    uint8       rowTick;            //Current tick in a row
    uint32      tps;                //Ticks per second
    uint8       tpr;                //Ticks per row (F-(XX < 32)). Default: 6
    uint8       tempo;              //Tempo. Default: 125 (Should update tps)
    
    //Control queue
    uint8       patternBreak;       //Break to the next pattern if < 64

    //Other
    InstrSample samples[32];        //Samples
} Song;

extern Song song;

InstrSample* getSample(uint8 sample);
Note         getNote(uint8 channel);
Channel*     getChannel(uint8 channel);

void         fillBuffer(LRSample* buff, uint32 size);
void         loadSong(wstr fileName);