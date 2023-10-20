#include "fx.h"

static void volumeSlide(Channel* chan, uint8 adjustVal) {
    if (!adjustVal) return;
    int8 volShift = adjustVal > 0x0F ? (adjustVal >> 4) : -adjustVal;

    if (((uint8)(chan->volume + volShift)) <= 64)   chan->volume += volShift;
    else if (volShift < 0)                          chan->volume = 0;
    else                                            chan->volume = 64;
}

static void tonePortamento(Channel* chan, int16 adjustVal) {
    if (!chan->targetPeriod) return;
    int32 diff = chan->basePeriod + chan->portamento - chan->targetPeriod;

    if (diff > 0) {
        if (diff - adjustVal > 0) {
            chan->portamento -= adjustVal;
            return;
        }
        goto lDone;
    }
    {
        if (diff + adjustVal < 0) {
            chan->portamento += adjustVal;
            return;
        }
        goto lDone;
    }

lDone:
    chan->basePeriod   = chan->targetPeriod;
    chan->portamento   = 0;
    chan->targetPeriod = 0;
}

void processTickFX(Channel* chan) {
    switch (chan->effect) {
        //Portamento up
        case 0x1: {
            if (!song.rowTick) break;
            chan->portamento -= MUL16(chan->effectArg);
            break;
        }

        //Portamento down
        case 0x2: {
            if (!song.rowTick) break;
            chan->portamento += MUL16(chan->effectArg);
            break;
        }

        //Tone portamento
        case 0x3: {
            if (!song.rowTick) break;
            if (chan->effectArg) chan->tonePortMem = chan->effectArg;
            tonePortamento(chan, MUL16(chan->tonePortMem));
            break;
        }

        //Vibrato
        case 0x4: {
            if (!song.rowTick) break;
            break;
        }

        //Volume slide + Tone portamento
        case 0x5: {
            if (!song.rowTick) break;
            volumeSlide(chan, chan->effectArg);
            tonePortamento(chan, MUL16(chan->tonePortMem));
            break;
        }

        //Volume slide + Vibrato
        case 0x6: {
            if (!song.rowTick) break;
            volumeSlide(chan, chan->effectArg);
            break;
        }

        //Tremolo
        case 0x7: {
            if (!song.rowTick) break;
            break;
        }

        //Volume slide
        case 0xA: {
            if (!song.rowTick) break;
            volumeSlide(chan, chan->effectArg);
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

        //Set panning (Not used)
        case 0x8: {
            break;
        }
    
        //Set offset
        case 0x9: {
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
                    chan->portamento -= MUL16(subA);
                    break;
                }
                case 0x2: {
                    chan->portamento += MUL16(subA);
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