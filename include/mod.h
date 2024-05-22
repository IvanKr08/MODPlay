#pragma once
#include "utils.h"

enum ModType { MT_GENMOD, MT_ST3 };
enum Effect {
    EFF_NOP,          //Zero Effect ( No sign )
    EFF_ARPEGGIO,     //| MOD 0 | S3M J |
    EFF_PORTUP,       //| MOD 1 | S3M F |
    EFF_PORTDOWN,     //| MOD 2 | S3M E |
    EFF_PORTTONE,     //| MOD 3 | S3M G |
    EFF_VIBRATO,      //| MOD 4 | S3M H |
    EFF_VOLPORT,      //| MOD 5 | S3M L |
    EFF_VOLVIBR,      //| MOD 6 | S3M K |
    EFF_TREMOLO,      //| MOD 7 | S3M R |
    EFF_PAN255,       //| MOD 8 | S3M X |
    EFF_OFFSET,       //| MOD 9 | S3M O |
    EFF_VOLSLIDE,     //| MOD A | S3M D |
    EFF_POSJUMP,      //| MOD B | S3M B |
    EFF_VOLUME,       //| MOD C | S3M   | Only MOD
    EFF_PATBREAK,     //| MOD D | S3M C |
    EFF_MODEXT,       //| MOD E | S3M   | Only MOD
    EFF_MODTEMPO,     //| MOD F | S3M   | Only MOD

    EFF_SPEED,        //| MOD   | S3M A | Only S3M
    EFF_TREMOR,       //| MOD   | S3M I | Only S3M
    EFF_VOLSLIDECHAN, //| MOD   | S3M M | Only S3M
    EFF_VOLCHAN,      //| MOD   | S3M N | Only S3M
    EFF_PANSLIDE,     //| MOD   | S3M P | Only S3M
    EFF_RETRIG,       //| MOD   | S3M Q | Only S3M
    EFF_S3MEXT,       //| MOD   | S3M S | Only S3M
    EFF_TEMPO,        //| MOD   | S3M T | Only S3M
    EFF_VIBRFINE,     //| MOD   | S3M U | Only S3M
    EFF_VOLGLOB,      //| MOD   | S3M V | Only S3M
    EFF_VOLSLIDEGLOB, //| MOD   | S3M W | Only S3M
    EFF_PANBRELLO,    //| MOD   | S3M Y | Only S3M
    EFF_MACRO         //| MOD   | S3M Z | Only S3M
};

#pragma pack(push, 1)
typedef struct {
    apisample l;
    apisample r;
} LRSample;
#pragma pack(pop)

typedef struct {
    uint32 baseFreq;
    uint32 length;
    bool   hasLoop;
    uint32 loopStart;
    uint32 loopEnd;
    uint32 loopSize;
    uint8  finetune;
    uint8  volume;
    int8  *data;
    char  *name;
    char  *fileName;
} Sample;

typedef struct {
    uint8  sample;      //0 = No sample;
    uint8  note;        //0 = No note; 1 = C-0; 254 = ^^^;
    uint8  effect;      
    uint8  effectArg;   
    uint8  vol;         //0 = No command; 1 = Volume 0; 65 = Volume 64; 66 = Pan 0;
} Note;

typedef struct {
    uint16  rows;   //Row count
    Note   *data;   //{channelCount} * {rows}
} Pattern;

typedef struct {
    //Playback
    bool        playing;            //Playing state
    bool        doNotMix;
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
    uint8       volSlideMem;        //S3M

    uint8 nEFxDelay;
    uint32 nEFxOffset;
    uint8 nEFxSpeed;
    Note delNote;
} Channel;

typedef struct {
    //Info
    cstr        name;               //Song name
    char        magic[5];           //File format
    uint32      magicID;            //Make from file format
    uint32      trackVer;           //S3M tracker version
    uint8       modType;            //Module format
    uint8       resetPos;

    //Patterns
    uint8       position;           //Position in song map (Song progress)
    uint8       pattern;            //Current pattern
    Note       *patternIt;          //Current pattern data cell
    uint16      row;                //Current row

    //Rendering
    uint32      alreadyRendered;    //How much samples has been rendered for this tick
    uint32      samplesPerTick;     //Samples per tick

    //Playback speed
    uint32      ticker;             //Global counter
    uint8       rowTick;            //Current tick in a row
    uint8       ticksPerRow;        //Ticks per row (F-(XX < 32)). Default: 6

    uint8       tempo;              //Tempo. Default: 125 (Should update ticksPerSecond)
    int32       tempoModifier;      //Used to adjust tempo from keyboard
    int32       mixVol;
    bool        hidePat;
    
    //Control queue
    int16       positionJump;       //Jump to the position if >= 0
    int16       patternBreak;       //Break to the next pattern if >= 0

    //Data
    Sample     *samples;            //sampleCount + 1. Zero sample should be empty
    Channel    *channels;
    Pattern    *patterns;
    uint8      *positions;          //Song map
    uint8       sampleCount;        //Actual sample count (sampleCount = 5 -> [0, 1, 2, 3, 4, 5])
    uint8       channelCount;
    uint8       patternCount;
    uint8       positionCount;      //Song length



    uint8 delay;
    bool pause;
    bool bit7Pan;
} Song;

extern Song song;

#define GETSAMPLE(sample) (&song.samples[sample])
#define GETNOTE(channel)  (song.patterns[song.pattern].data[(song.row * song.channelCount) + channel]) //Optimize
#define GETCHANNEL(num)   (&song.channels[num])

void fillBuffer(apisample* buff, uint32 totalSamples);
void onTick();
void setTempo(uint8 speed);


//    uint32      ticksPerSecond;                //Ticks per second