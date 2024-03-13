#include "utils.h"
#include "mod.h"
#include "console.h"
#include <Windows.h>

void dumpSound(apisample *buff, uint32 totalSamples);

#define PERIODDIVIDEND_NTSC 57272720            //C-5 - 8363
#define PERIODDIVIDEND_PAL  56750320            //C-5 - 8287
#define PERIODDIVIDEND      PERIODDIVIDEND_PAL

#define ARGL(effectArg) (effectArg >> 4) //Extended effect
#define ARGR(effectArg) (effectArg & 15) //Extended effect arg

#define GETPERIOD(note, finetune) (tunedPeriods[((note) - 1 % 84) + ((finetune & 15) * 84)])

#define VOLUP(volVal, amount, limit) (volVal) = ((volVal) + (amount) < (limit)) ? (volVal) + (amount) : (limit)
#define VOLDOWN(volVal, amount)      (volVal) = ((volVal) - (amount) > 0)       ? (volVal) - (amount) : 0

#define SETPAN15(chanPan, val)    (chanPan) = ((val) == 15)  ? 256 : ((val) * 17)
#define SETPAN255(chanPan, val)   (chanPan) = ((val) == 255) ? 256 : (val)

Song song;

//Очень неудачный вариант микшера. Клиппинг применяется на каждом канале.
//Переписать, изменив порядок действий и метод усиление.
//Требуется поддержка звуков больше 64KiB, сглаживание громкости, линейная интерполяция, глобальная громкость
//Раздельная Л/П громкость имеет смысл при наличии огибающей и глобальной громкости.
//Инкремент указателя должен быть быстрее обращения по индексу
void fillBuffer(apisample *buff, uint32 totalSamples) {
    memset(buff, A_SILENCECONST, totalSamples * A_SAMPLESIZE);

    for (uint32 gPos = 0; gPos < totalSamples;) {
        //Остаточный размер буфера (Отрендерить до конца) || Сколько осталось отрендерить до конца такта
        uint32 maxToRender = min(totalSamples - gPos, song.samplesPerTick - song.alreadyRendered);

        for (uint8 channel = 0; channel < song.channelCount; channel++) {
            Channel *chan = GETCHANNEL(channel);

            //Optimize channels with zero volume 
            if (!chan->playing) continue;

            Sample *sample = GETSAMPLE(chan->sample);

            for (uint32 i = gPos; i < maxToRender + gPos; i++) {
                uint16 index = DIV4K(chan->progress);
                if (index >= sample->loopEnd) {
                    if (sample->hasLoop) {
                        //Preserve progress fract and fix first looped index 
                        chan->progress -= MUL4K(sample->loopSize);
                        index = DIV4K(chan->progress);

                        if (index >= sample->loopEnd) {
                            chan->playing = 0;
                            break;
                        }
                    }
                    else {
                        chan->playing = 0;
                        break;
                    }
                }

                //Остается надеяться, что это будет арифметический сдвиг
                int32 data = (sample->data[index] * A_AMPLIFY * chan->playVolume) >> 12; //8 - громкость, 4 - усиление

                int32 r = buff[i * 2]     + DIV256(data * (256 - chan->panning));
                int32 l = buff[i * 2 + 1] + DIV256(data * chan->panning);

                //Не очень удачное решение
                CLAMP(r, -32768, 32767);
                CLAMP(l, -32768, 32767);

                buff[i * 2] = r;
                buff[i * 2 + 1] = l;

                chan->progress += chan->currentStep;
            }
        }

        gPos += maxToRender;
        song.alreadyRendered += maxToRender;

        if (song.alreadyRendered >= song.samplesPerTick) onTick();
    }
}

//=== Playback ===//
static void volumeSlide(Channel *chan, uint8 speed) {
    if (!speed) speed = chan->volSlideMem;
    else chan->volSlideMem = speed;

    uint8 spdDown = speed & 0xF, spdUp = speed >> 4;

    //Указание сдвига одновременно вверх и вниз недопустимо (Но при этом должно приводить к сдвигу вверх)
    //if (spdDown && spdUp) return;
    if (spdUp) VOLUP  (chan->volume, MUL4(spdUp), 256);
    else       VOLDOWN(chan->volume, MUL4(spdDown));
}

static void tonePortamento(Channel *chan, int16 speed) {
    if (!chan->targetPeriod) return;

    //Расстояние от текущего периода до целевого
    int32 diff = chan->basePeriod + chan->portamento - chan->targetPeriod;

    if (diff > 0) {
        //Если цель еще не достигнута
        if ((diff - speed) > 0) {
            chan->portamento -= speed;
            return;
        }
        //Иначе если цель достигнута или опережена
        goto lDone;
    }
    {
        if ((diff + speed) < 0) {
            chan->portamento += speed;
            return;
        }
        goto lDone;
    }

lDone:
    //Включаем целевую ноту и сбрасываем портаменто
    chan->basePeriod = chan->targetPeriod;
    chan->baseNote = chan->targetNote;
    chan->portamento = 0;
    chan->targetPeriod = 0;
}

static int8 getVibratoDelta(uint8 type, uint8 pos) {
    switch (type & 0x03) {
        case 0:	//Sine
            return vibrSine[pos];

        case 1:	//Ramp down
            return (pos < 32 ? 0 : 255) - pos * 4;

        case 2:	//Square
            return pos < 32 ? 127 : -127;

        case 3:	//Random
            return vibrRandom[pos];
    }
}

static void vibrato(Channel *chan) {
    if (!chan->vibratoDepth || !chan->vibratoSpeed) return;

    chan->vibrato     = MUL16((getVibratoDelta(chan->vibratoType, chan->vibratoPos) * chan->vibratoDepth) >> 6);
    chan->vibratoPos += chan->vibratoSpeed;
    chan->vibratoPos &= 0x3F;
}

static void tremolo(Channel *chan) {
    if (!chan->tremoloDepth || !chan->tremoloSpeed) return;

    if (chan->volume > 0) {
        chan->tremolo = (getVibratoDelta(chan->tremoloType, chan->tremoloPos) * chan->tremoloDepth) >> 3;
    }
    chan->tremoloPos += chan->tremoloSpeed;
    chan->tremoloPos &= 0x3F;
}

static void invertLoop(Channel *chan) {
    // EFx implementation for MOD files (PT 1.1A and up: Invert Loop)
    // This effect trashes samples. Thanks to 8bitbubsy for making this work. :)
    if (!chan->invertLoopSpeed || !GETSAMPLE(chan->sample)->hasLoop) return;
    /*
    chn.nEFxDelay += ModEFxTable[chn.nEFxSpeed & 0x0F];
    if (chn.nEFxDelay < 128)
        return;
    chn.nEFxDelay = 0;

    const SmpLength loopStart = pModSample->uFlags[CHN_LOOP] ? pModSample->nLoopStart : pModSample->nSustainStart;
    const SmpLength loopEnd = pModSample->uFlags[CHN_LOOP] ? pModSample->nLoopEnd : pModSample->nSustainEnd;

    if (++chn.nEFxOffset >= loopEnd - loopStart)
        chn.nEFxOffset = 0;

    // TRASH IT!!! (Yes, the sample!)
    const uint8 bps = pModSample->GetBytesPerSample();
    uint8 *begin = mpt::byte_cast<uint8 *>(pModSample->sampleb()) + (loopStart + chn.nEFxOffset) * bps;
    for (auto &sample : mpt::as_span(begin, bps))
    {
        sample = ~sample;
    }
    pModSample->PrecomputeLoops(*this, false);
    */
}

void setTempo(uint8 speed) {
    if (speed == 0) fatal(0, "F-00. Stopping...");

    if (speed < 32) song.ticksPerRow = speed;
    else {
        song.tempo = speed;;
        //А НУЖНО БЫЛО ИСПОЛЬЗОВАТЬ ДРОБНЫЕ ВЫЧИСЛЕНИЯ. СУКА, НЕДЕЛЯ ПОИСКА
        song.samplesPerTick = MUL512(A_SAMPLERATE) / (MUL1K((speed + song.tempoModifier) < 8 ? 8 : (speed + song.tempoModifier)) / 5);
    }
}

static void processRowFX(Channel *chan) {
    uint8 extEff = 0, extArg = 0;

    if (chan->wasArp) {
        chan->basePeriod = GETPERIOD(chan->baseNote, chan->finetune);
        chan->wasArp = 0;
    }
    if (chan->effect != 0x4 && chan->effect != 0x6) chan->vibrato = 0;
    if (chan->effect != 0x7) chan->tremolo = 0;

    switch (chan->effect) {
        case 0x8: //Panning
            SETPAN255(chan->panning, chan->effectArg);
            break;

        case 0xB: //Position jump
            song.positionJump = chan->effectArg;
            break;

        case 0xC: //Volume
            chan->volume = (chan->effectArg >= 64) ? 256 : MUL4(chan->effectArg);
            break;

        case 0xD: //Pattern break
            song.patternBreak = chan->effectArg;
            break;

        case 0xF: //Set tempo/speed
            setTempo(chan->effectArg);
            break;

        case 0xE: //Extended effects
            extEff = ARGL(chan->effectArg);
            extArg = ARGR(chan->effectArg);

            switch (extEff) {
                case 0x1: //Fine portamento up
                    chan->portamento -= MUL16(extArg);
                    break;

                case 0x2: //Fine portamento down
                    chan->portamento += MUL16(extArg);
                    break;

                case 0x8: //Panning 
                    SETPAN15(chan->panning, extArg);
                    break;

                case 0xA: //Fine volume slide up
                    VOLUP(chan->volume, MUL4(extArg), 256);
                    break;

                case 0xB: //Fine volume slide down
                    VOLDOWN(chan->volume, MUL4(extArg));
                    break;
            }

            break;
    }
}

//Exclude first tick
static void processTickFX(Channel *chan) {
    switch (chan->effect) {
        case 0x0: {
            if (!chan->effectArg) break;
            chan->wasArp = 1;

            //Overflow alarm
            switch (song.rowTick % 3)
            {
                case 0:
                    chan->basePeriod = GETPERIOD(chan->baseNote, chan->finetune);
                    break;

                case 1:
                    chan->basePeriod = GETPERIOD(chan->baseNote + (chan->effectArg >> 4), chan->finetune);
                    break;

                case 2:
                    chan->basePeriod = GETPERIOD(chan->baseNote + (chan->effectArg & 0xF), chan->finetune);
                    break;
            }

            break;
        }

        case 0x1: //Portamento up (No memory in MOD)
            chan->portamento -= MUL16(chan->effectArg);
            break;

        case 0x2: //Portamento down (No memory in MOD)
            chan->portamento += MUL16(chan->effectArg);
            break;

        case 0x3: //Tone portamento
            tonePortamento(chan, MUL16(chan->effectArg ? (chan->tonePortMem = chan->effectArg) : chan->tonePortMem));
            break;

        case 0x4: //Vibrato
            if (chan->effectArg & 0xF) chan->vibratoDepth = chan->effectArg & 0xF;
            if (chan->effectArg >> 4)  chan->vibratoSpeed = chan->effectArg >> 4;
            vibrato(chan);
            break;

        case 0x5: //Volume slide + Tone portamento
            volumeSlide(chan, chan->effectArg);
            tonePortamento(chan, MUL16(chan->tonePortMem));
            break;

        case 0x6: //Volume slide + Vibrato
            volumeSlide(chan, chan->effectArg);
            vibrato(chan);
            break;

        case 0x7: //Tremolo
            if (chan->effectArg & 0xF) chan->tremoloDepth = chan->effectArg & 0xF;
            if (chan->effectArg >> 4)  chan->tremoloSpeed = chan->effectArg >> 4;
            tremolo(chan);
            break;

        case 0xA: //Volume slide
            volumeSlide(chan, chan->effectArg);
            break;
    }
}

static void navigate() {
    if (song.patternBreak < 64) {  song.row = song.patternBreak; printS("Pattern break.\n"); }
    else                           song.row = 0;

    if (song.positionJump < 128) { song.position = song.positionJump; printS("Position jump.\n"); }
    else                           song.position++;

    if (song.position >= song.positionCount) {
        printS("Reset.\n");
        song.position = 0;
    }

    while (song.positions[song.position] == 254) song.position++; //Fix this
    song.pattern = song.positions[song.position];
    song.positionJump = 255;
    song.patternBreak = 255;

    printFormat("Pattern: %3i, position: %3i.\n", 2, song.pattern, song.position);
}

void onTick() {
    //Update row
    if (song.rowTick == 0) {
        if (song.row > 63 || song.patternBreak < 64 || song.positionJump < 128) navigate();

        //Update channels
        for (uint8 i = 0; i < song.channelCount; i++) {
            Note     note = GETNOTE(i);
            Channel *chan = GETCHANNEL(i);

            chan->effect    = note.effect;
            chan->effectArg = note.effectArg;

            if (note.sample) {
                if (note.effect != 3 && note.effect != 5) chan->qSample = note.sample;
                chan->volume = MUL4(GETSAMPLE(note.sample)->volume);
            }

            if (note.note) {
                //Plain note
                if (note.effect != 3 && note.effect != 5) {

                    //Update sample
                    if (chan->qSample) {
                        chan->sample = chan->qSample;
                        chan->finetune = GETSAMPLE(chan->sample)->finetune;
                        chan->qSample = 0;
                    }

                    //(9-XX) Set offset
                    if (note.effect == 9) {
                        chan->offsetMem = note.effectArg ? note.effectArg : chan->offsetMem;
                        chan->progress = chan->offsetMem << 8 << 12;
                    }
                    else chan->progress = 0;

                    //(E-5X) Set Finetune
                    if (note.effect == 0xE && ARGL(note.effectArg) == 5)
                        chan->finetune = (note.effectArg & 0x0F);

                    chan->basePeriod = tunedPeriods[(note.note - 1) + (chan->finetune * 84)];
                    chan->baseNote = note.note;
                    chan->portamento = 0;

                    //Новая нота меняет текущий период и сбрасывает портаменто, но не сбрасывает целевую ноту. (PortaTarget.mod) { chan->targetPeriod = 0; }
                    chan->playing = 1;
                }

                //Tone portamento
                else if (chan->basePeriod) {
                    chan->targetPeriod = tunedPeriods[(note.note - 1) + (chan->finetune * 84)];
                    chan->targetNote = note.note;
                }
            }

            if (note.vol) {
                chan->volume = (note.vol - 1) * 4;
            }

            processRowFX(chan);
        }
        //===

        printRow();
        song.row++;
        song.rowTick = 0;
    }
    //===
    else for (uint32 i = 0; i < song.channelCount; i++) processTickFX(GETCHANNEL(i));

    //Update tick
    song.alreadyRendered = 0;
    for (uint32 i = 0; i < song.channelCount; i++) {
        Channel *chan = GETCHANNEL(i);
        if (chan->playing) {
            int32 period = chan->basePeriod + chan->portamento + chan->vibrato;

            //Переполнение практически нереально
            period = (period > -1) ? period : 0;
            if (period == 0) {
                chan->currentStep = chan->currentFreq = 0;
                continue;
            }

            if (song.modType == MT_ST3) {
                if (chan->baseNote == 254) {
                    chan->playing = chan->currentFreq = chan->currentStep = 0;
                    goto lSkip;
                }
                if (!chan->sample) {
                    printS("WARN: Zero sample!\n");
                    goto lSkip;
                }

                uint8  noteNum        = chan->baseNote - 13;
                uint32 note_st3period = (8363 * 16 * (s3mFreq[noteNum % 12] >> (noteNum / 12))) / GETSAMPLE(chan->sample)->baseFreq;
                note_st3period       += chan->vibrato / 4;
                chan->currentFreq     = 14317456 / note_st3period;

                //int32 octaveFreq = GETSAMPLE(chan->sample)->baseFreq << (noteNum / 12);
                //printI(octaveFreq);
                //if (!octaveFreq) continue;
                //period = 8363 * (freqS3MTable[noteNum % 12] << 5) / octaveFreq;
                //if (!period) continue;
                //chan->currentFreq = 3665268736 / (period << 8);
            }
            else {
                chan->currentFreq = PERIODDIVIDEND / period;
            }

            lSkip:
            //Слишком мало места под высокочастотные звуки. Временное исправление. Нужен uint64
            //chan->currentFreq = (chan->currentFreq < 65536) ? chan->currentFreq : 65535;
            chan->currentStep = (chan->currentFreq << 12u) / (uint32)A_SAMPLERATE;
            chan->playVolume = chan->volume + chan->tremolo;
            CLAMP(chan->playVolume, 0, 256);
        }
    }

    handleInput();

    if (++song.rowTick == song.ticksPerRow) song.rowTick = 0;
    song.ticker++;
    //===
}
//===



























          //chan->currentFreq = (7093789 / 2) / period;

                  //chan->basePeriod = ((tunedPeriodsCompact[chan->finetune * 12 + (note.note - 1) % 12] << 5) >> ((note.note - 1) / 12));





//chan->currentFreq = ((((3546895L * 4) << 4) / (period))) * 4;
//chan->currentStep = (((long long)chan->currentFreq << 32) / (48000ULL << 4)) >> 16;
//printI(8363ULL * ((1712ULL << 8ULL) << 4ULL) / (((period / 16ULL) << 8ULL) + 4ULL));

            //Если опустить громкость до нуля, а после поднять ее, то семпл продолжит
            //воспроизведение с неправильной позиции (Частично исправлено)
            //~~~(if (vol == 0) progress = min(progress + maxToRender, end) )~~~