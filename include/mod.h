#pragma once
#include "utils.h"

#pragma pack(push, 1)
typedef struct {
    apisample l;
    apisample r;
} LRSample;
#pragma pack(pop)

typedef struct {
    uint32 length;
    uint32 loopStart;
    uint32 loopEnd;
    uint32 loopSize;
    uint8  finetune;
    uint8  volume;
    int8  *data;
    char   name[23];
    bool   hasLoop;
} Sample;

typedef struct {
    uint8  sample;
    uint8  note;
    uint8  effect;
    uint8  effectArg;
} Note;

typedef struct {
    //Playback
    bool        playing;            //Playing state
    uint8       baseNote;           //Note corresponding basePeriod
    uint16      basePeriod;         //Finetuned note period
    uint32      progress;           //Sample progress (fixed1616)
    uint32      currentFreq;        //Precalculated frequency (fixed1616)
    uint32      currentStep;        //Step in source array in resampler (fixed1616)
    
    uint8       qSample;            //Sample to be used in the next notes
    uint8       sample;             //Sample number
    
    //FX
    uint8       effect, effectArg;  //Row effect

    int16       volume;             //(0-256) Multiplied by 4
    int16       playVolume;         //(0-256) Final volume used in mixer
    int16       panning;            //(0-256) Clamp 255 to 256
    uint8       finetune;           //(E-5X)
    int32       portamento;         //(1-XX) (2-XX) (3-XX) (Period shift / 16)
    uint8       targetNote;         //Note corresponding targetPeriod
    uint16      targetPeriod;       //(3-XX) Tone portamento target period (0 if reached)
    bool        wasArp;             //Used to reset basePeriod. Set in arpeggio handler, reset on new row

    //Vibrato / Tremolo
    int32       vibrato, tremolo;
    uint8       vibratoSpeed, vibratoDepth, vibratoPos, vibratoType;
    uint8       tremoloSpeed, tremoloDepth, tremoloPos, tremoloType;

    //Memory
    uint8       offsetMem;          //(9-00) Offset
    uint8       tonePortMem;        //(3-00) Tone portamento
} Channel;

typedef struct {
    //Strings
    uint8       name[21];           //Song name
    uint8       magic[5];           //File format

    //Patterns
    Note       *patterns;           //Patterns array
    uint8       patternCount;       //Count of patterns
    uint8       pattern;            //Current pattern
    uint8       row;                //Position in pattern

    //Song map
    uint8       positions[128];     //Song map
    uint8       positionCount;      //Song length
    uint8       position;           //Position in song map (Song progress)
    uint8       resetPos;

    //Rendering
    uint32      alreadyRendered;    //How much samples has been rendered for this tick
    uint32      samplesPerTick;                //Samples per tick

    //Playback speed
    uint32      ticker;             //Global counter
    uint8       rowTick;            //Current tick in a row
    uint8       ticksPerRow;        //Ticks per row (F-(XX < 32)). Default: 6
    uint8       tempo;              //Tempo. Default: 125 (Should update ticksPerSecond)
    
    //Control queue
    uint8       positionJump;       //Jump to the position if < 128
    uint8       patternBreak;       //Break to the next pattern if < 64

    //Other
    Sample      samples[32];
    Channel    *channels;
    uint8       channelCount;
} Song;

extern Song song;

#define GETSAMPLE(sample) (&song.samples[sample])
#define GETNOTE(channel)  (song.patterns[(song.pattern * 64 * song.channelCount) + (song.row * song.channelCount) + channel]) //Optimize
#define GETCHANNEL(num)   (&song.channels[num])

void fillBuffer(apisample* buff, uint32 totalSamples);
void onTick();


//    uint32      ticksPerSecond;                //Ticks per second