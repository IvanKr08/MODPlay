#include "fx.h"

#define FX_ADJUSTVOLUME(volume, adjustVal) {\
    if (song.rowTick != 0 && adjustVal != 0){\
        int8 volShift = adjustVal > 0x0F ? (adjustVal >> 4) : -adjustVal;\
        if (((uint8)(volume + volShift)) <= 64) volume += volShift;\
        else if (volShift < 0) volume = 0;\
        else volume = 64;\
    }\
}
    
void processTickFX(Channel* chan) {
    switch (chan->effect) {
        //Portamento up
        case 0x1: {
            if (song.rowTick == 0) break;
            chan->portamento -= chan->effectArg;
            break;
        }

        //Portamento down
        case 0x2: {
            if (song.rowTick == 0) break;
            chan->portamento += chan->effectArg;
            break;
        }
     
        //Volume slide + Tone portamento
        case 0x5: {
            FX_ADJUSTVOLUME(chan->volume, chan->effectArg);
            break;
        }

        //Volume slide + Vibrato
        case 0x6: {
            FX_ADJUSTVOLUME(chan->volume, chan->effectArg);
            break;
        }

        //Volume slide
        case 0xA: {
            FX_ADJUSTVOLUME(chan->volume, chan->effectArg);
            break;
        }

        default: {
            break;
        }
    }
}

void processRowFX(Channel *chan) {
    switch (chan->effect) {
        //Arpeggio
        case 0x0: {

            break;
        }

        //Tone portamento
        case 0x3: {

            break;
        }

        //Vibrato
        case 0x4: {

            break;
        }

        //Volume slide + Tone portamento
        case 0x5: {

            break;
        }

        //Volume slide + Vibrato
        case 0x6: {

            break;
        }

        //Tremolo
        case 0x7: {

            break;
        }

        //Set panning (Not used)
        //-
    
        //Set offset
        case 0x9: {
            if (chan->progress > 0) break;

            if (chan->effectArg != 0) chan->progress = chan->effectArg << 24;
            else chan->progress = chan->offsetMem << 24;
            break;
        }

        //Position jump
        case 0xB: {

            break;
        }

        //Set volume
        case 0xC: {
            chan->volume = (chan->effectArg > 64) ? 64 : chan->effectArg;
            break;
        }

        //Pattern break
        case 0xD: {
            song.patternBreak = chan->effectArg;
            break;
        }

        //===Extended effects
        case 0xE: {
            uint8 subC = chan->effectArg >> 4;
            uint8 subA = chan->effectArg & 0x0F;

            switch (subC) {
                case 0x1: {
                    chan->portamento -= subA;
                    break;
                }
                case 0x2: {
                    chan->portamento += subA;
                    break;
                }
                case 0xA: {
                    chan->volume = (chan->volume + subA <= 64) ? chan->volume + subA : 64;
                    break;
                }
                case 0xB: {
                    chan->volume = (chan->volume - subA >= 0) ? chan->volume - subA : 0;
                    break;
                }
                default: {

                }
            }
            break;
        }

        //Set tempo/speed
        case 0xF: {
            if (chan->effectArg == 0) fatal(0, "F-00. Stopping...");

            if (chan->effectArg < 32) {
                song.tpr = chan->effectArg;
            }
            else {
                song.tempo = chan->effectArg;
                song.tps   = MUL2(song.tempo) / 5;
                song.spt   = A_SR / song.tps;
            }
            break;
        }
    }
}