#define CF_EXTFILE

#include "mod.h"
    
Song song;

//=== Load file ===//
static void   readFile(HANDLE file, uint32 count, void* buff) {
#ifdef CF_EXTFILE
    BOOL readResult = ReadFile(file, buff, count, 0, 0);
    fatal(!readResult, "File read error");
#else
    static uint32 offset;
    for (size_t i = 0; i < count; i++, offset++) ((byte*)buff)[i] = ((byte*)file)[offset];
#endif
}
static uint8  readByte(HANDLE file) {
    uint8 val;
    readFile(file, 1, &val);
    return val;
}
static uint16 readWord(HANDLE file) {
    uint16 val;
    readFile(file, 2, &val);
    return (val >> 8) | ((val & 0xFF) << 8);
}
static uint32 readDWord(HANDLE file) {
    uint32 val;
    readFile(file, 4, &val);
    return ((val >> 24) & 0xFF) | ((val << 8) & 0xFF0000) | ((val >> 8) & 0xFF00) | ((val << 24) & 0xFF000000);
}



void loadSong(wstr fileName) {
#ifdef CF_EXTFILE
    HANDLE file = CreateFileW(fileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    fatal(file == INVALID_HANDLE_VALUE, "Could not open file");
    int32 fileSize = GetFileSize(file, 0);
    fatal(fileSize > 2097152 /* 2MiB */, "File too big");

    printS("File: \t\t");
    printW(fileName);
    printFormat("\nHandle: \t0x%X\nSize: \t\t%i\nType: \t\tMOD\n\n", 2, file, fileSize);
#else
    HANDLE file = LockResource(LoadResource(GetModuleHandleA(0), FindResourceA(GetModuleHandleA(0), "IDF_SONG", RT_RCDATA)));
    int32 fileSize = -1;
#endif
    readFile(file, 20, song.name);

    //Sample headers
    for (uint32 i = 1; i < 32; i++) {
        InstrSample *sample = &song.samples[i];
        readFile(file, 22, sample->name);
        sample->length      = MUL2(readWord(file));
        sample->finetune    =     (readByte(file));
        sample->volume      =     (readByte(file));
        sample->loopStart   = MUL2(readWord(file));
        sample->loopEnd     = MUL2(readWord(file)) + sample->loopStart;
        sample->hasLoop = 1;

        //Stealed from OpenMPT
        if (sample->loopStart >= sample->length         ||
            sample->loopStart >= sample->loopEnd        ||
            sample->loopEnd   <  4                      ||
            sample->loopEnd   -  sample->loopStart < 4   ) sample->hasLoop = 0;

        if (!sample->hasLoop) sample->loopEnd = sample->length; //No one-shot loops, sorry.
    }

    song.positionCount  = readByte(file);
    song.resetPos       = readByte(file);
    readFile(file, 128, &song.positions);
    readFile(file, 4, &song.magic);

    //Print song info
    printS("Samples:\nNo | Name                   | Vol    FnTune   | Length    Lp. Strt  Lp. End  |\n");
    for (uint32 i = 1; i < 32; i++) {
        printFormat ("%0-2i | %-22s | V: %-2i  FTUN: %-2i | L: %-5i  S: %-5i  E: %-5i |\n", 7,
            i,
            song.samples[i].name,
            song.samples[i].volume,
            int4[song.samples[i].finetune],
            song.samples[i].length,
            song.samples[i].loopStart,
            song.samples[i].loopEnd
        );
    }

    //Print pattern info and find its count
    printS("\nSong positions:\n");
    for (uint32 i = 0; i < 8; i++) {
        printS("| ");
        for (uint32 j = 0; j < 16; j++) {
            uint8 pattern = song.positions[MUL16(i) + j];
            if (song.patternCount < pattern) song.patternCount = pattern;
            printFormat("%-3i ", 1, pattern);
        }
        printS("|\n");
    }
    song.patternCount++;
    printFormat("Song: \t\t\"%s\"\nMagic: \t\t\"%s\"\nReset: \t\t%i\n", 3, song.name, song.magic, song.resetPos);
    printFormat("Pattern count: \t%i\nSong length: \t%i\n", 2, song.patternCount, song.positionCount);

    //Alloc pattern buffer
    song.patterns = memAlloc(sizeof(Pattern) * song.patternCount);
    fatal(!song.patterns, "Could not allocate memory for patterns");

    //Read patterns
    for (uint32 i = 0, j, n, m; i < song.patternCount; i++) {
        for (j = 0; j < 64; j++) {
            for (n = 0; n < 4; n++) {
                Note* note      = &song.patterns[i][MUL4(j) + n];
                uint16 period   = 0;
                uint32 rawNote  = (readDWord(file));
                note->sample    = ((rawNote & 0b11110000000000000000000000000000) >> 24);
                period          = ((rawNote & 0b00001111111111110000000000000000) >> 16);
                note->sample   |= ((rawNote & 0b00000000000000001111000000000000) >> 12);
                note->effect    = ((rawNote & 0b00000000000000000000111100000000) >> 8);
                note->effectArg = ((rawNote & 0b00000000000000000000000011111111) >> 0);
                if (period != 0) for (m = 0; m < 84; m++) {
                    if (notePeriods[m] <= period) {
                        note->note = m + 1;
                        break;
                    }
                }
                else note->note = 0;
            }
        }
    }

    //Load samples
    for (uint32 i = 1; i < 32; i++) {
        if (song.samples[i].length > 0) {
            song.samples[i].data = memAlloc(song.samples[i].length);
            fatal(!song.samples[i].data, "Could not allocate memory for samples");

            readFile(file, song.samples[i].length, song.samples[i].data);
        }
    }

    printS("EOF\n");
#ifdef CF_EXTFILE
    CloseHandle(file);
#endif
}
//===



//=== Getters (Replace with macros) ===//
InstrSample* getSample(uint8 sample) {
    return &song.samples[sample];
}
Note getNote(uint8 channel) {
    return song.patterns[song.pattern][MUL4(song.row) + channel];
}
Channel* getChannel(uint8 channel) {
    return &song.channels[channel];
}
//===



//=== Playback ===//
void resample(float* buff, uint32 renderCount, Channel* chan) {
    uint32 i = 0;
    if (!chan->playing || chan->volume == 0) goto lFillOut;
    InstrSample* sample = getSample(chan->sample);

    for (; i < renderCount; i++) {
        uint16 index = DIV64K(chan->progress);
        if (index >= sample->loopEnd) {
            if (sample->hasLoop) {
                chan->progress = MUL64K(sample->loopStart);
                index = 0;
            }
            else {
                chan->playing = 0;
                goto lFillOut;
            }
        }
        buff[i] = (((float)sample->data[index]) * chan->volume) / 64;
        chan->progress += chan->currentStep;
    }

lFillOut:;
    for (; i < renderCount; i++) buff[i] = 0;
}



void onTick() {

    //Update row
    if (!song.rowTick) {

        //Update pattern
        if (song.patternBreak < 64) {
            song.row = song.patternBreak;
            printS("Pattern break.\n");
            song.patternBreak = 255;

            song.position++;
            if (song.position >= song.positionCount) {
                printS("Reset.\n");
                song.position = 0;
            }
            song.pattern = song.positions[song.position];

            printFormat("Pattern: %3i, position: %3i\n", 2, song.pattern, song.position);
        }

        if (song.row >= 64) {
            song.row = 0;
            song.position++;
            if (song.position >= song.positionCount) {
                printS("Reset.\n");
                song.position = 0;
            }
            song.pattern = song.positions[song.position];

            printFormat("Pattern: %3i, position: %3i\n", 2, song.pattern, song.position);
        }
        //===

        //Update channels
        for (uint32 i = 0; i < 4; i++) {
            Note     note = getNote(i);
            Channel* chan = getChannel(i);

            chan->effect    = note.effect;
            chan->effectArg = note.effectArg;

            if (note.sample) {
                chan->qSample   = note.sample;
                chan->volume    = getSample(note.sample)->volume;
            }

            //(E-5X) Set Finetune
            if (note.effect == 0xE && (note.effectArg >> 4) == 5) chan->finetune = (note.effectArg & 0x0F);

            if (note.note) {

                //Plain note
                if (note.effect != 3 && note.effect != 5) {

                    //Update sample
                    if (chan->qSample) {
                        chan->sample = chan->qSample;
                        chan->finetune = getSample(chan->sample)->finetune;
                        chan->qSample = 0;
                    }

                    //(9-XX) Set offset
                    if (note.effect == 9) {
                        chan->offsetMem = note.effectArg ? note.effectArg : chan->offsetMem;
                        chan->progress = chan->offsetMem << 24;
                    }
                    else chan->progress = 0;

                    chan->playing = 1;
                    //chan->basePeriod = tunedPeriods[(note.note - 1) + (chan->finetune * 84)];
                    
                    chan->basePeriod = ((tunedPeriodsCompact[chan->finetune * 12 + (note.note - 1) % 12] << 5) >> ((note.note - 1) / 12));
                    //printI(tunedPeriods[(note.note - 1) + (chan->finetune * 84)]);
                    //printI(chan->basePeriod);
                    chan->portamento = 0;
                    chan->targetPeriod = 0;
                }

                //If there is no note specified it slides towards the last note specified in
                //the Porta to Note effect.
                //-^- FALSE

                //Tone portamento
                else if (chan->basePeriod) chan->targetPeriod = tunedPeriods[(note.note - 1) + (chan->finetune * 84)];
                //else if (chan->basePeriod) chan->targetPeriod = (chan->targetPeriod = (tunedPeriodsCompact[chan->finetune * 12 + (note.note - 1) % 12] << 5) >> ((note.note - 1) / 12));
            }

            processRowFX(chan);
        }
        //===

        printRow();
        song.row++;
        song.rowTick = 0;
    }
    //===

    //Update tick
    song.tickRenderCount = 0;

    for (uint32 i = 0; i < 4; i++) {
        Channel* chan = getChannel(i);

        if (chan->playing) {
            /*
            if (chan->effect == 0 && chan->effectArg != 0) {
                switch (song.rowTick % 3) {
                case 0:
                    chan->basePeriod = ((tunedPeriodsCompact[chan->finetune * 12 + (chan.note - 1) % 12] << 5) >> ((note.note - 1) / 12));
                case 1:
                    chan->basePeriod = ((tunedPeriodsCompact[chan->finetune * 12 + (note.note) % 12] << 5) >> ((note.note) / 12));
                case 2:
                    chan->basePeriod = ((tunedPeriodsCompact[chan->finetune * 12 + (note.note + 1) % 12] << 5) >> ((note.note + 1) / 12));
                }
            }
            else
            */

            uint32 period = getChannel(i)->basePeriod + getChannel(i)->portamento;
            fatal(!period, "Zero period!");
                    //MUL8(7093789) 57272720 56750320
            //                    14187580
            //chan->currentFreq = 57272720 / period;
            chan->currentFreq = 56750320 / period;
            //if (song.row % 32 < 16) chan->currentFreq = 14187580 / DIV4(period);
            //                                          56750320 -^-
            //                                          56750320
            //else chan->currentFreq = 57272720 / period;
            chan->currentStep = MUL64K(getChannel(i)->currentFreq) / A_SR;
            //if (i == 1) printI(chan->currentFreq);
        }
        processTickFX(chan);
        //if (i == 3) printI(chan->volume);
    }

    if (++song.rowTick == song.tpr) song.rowTick = 0;
    song.ticker++;
    //===
}

uint8 b[44100 * 6];
int bp = 0;
void WriteToFile(char* data, char* filename)
{
    HANDLE hFile;
    DWORD dwBytesToWrite = strlen(data);
    DWORD dwBytesWritten;
    BOOL bErrorFlag = FALSE;

    hFile = CreateFileA(filename,  // name of the write
        FILE_APPEND_DATA,          // open for appending
        FILE_SHARE_READ,           // share for reading only
        NULL,                      // default security
        OPEN_ALWAYS,               // open existing file or create new file 
        FILE_ATTRIBUTE_NORMAL,     // normal file
        NULL);                     // no attr. template

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printS("!!!Wrote u bytes to successfully.\n");
        return;
    }

    while (dwBytesToWrite > 0)
    {
        bErrorFlag = WriteFile(
            hFile,              // open file handle
            data,               // start of data to write
            dwBytesToWrite,     // number of bytes to write
            &dwBytesWritten,    // number of bytes that were written
            NULL);              // no overlapped structure

        if (!bErrorFlag)
        {
            printS("!!Wrote u bytes to successfully.\n");
            break;
        }

        printS("Wrote u bytes to successfully.\n");

        data += dwBytesWritten;
        dwBytesToWrite -= dwBytesWritten;
    }

    CloseHandle(hFile);
}

void fillBuffer(LRSample* buff, uint32 size) {
    uint32 spb = size / 4;
    static bool init = 0;
    //float buff1[size], buff2[size], buff3[size], buff4[size];
    float *buff1, *buff2, *buff3, *buff4;
    buff1 = malloc(size * 4);
    buff2 = malloc(size * 4);
    buff3 = malloc(size * 4);
    buff4 = malloc(size * 4);

    if (!init) {
        song.tempo    = 125;
        song.tpr      = 6;
        song.tps      = 50;
        song.spt      = A_SR / (MUL2(song.tempo) / 5);
        song.position = 0;
        //song.row      = 40;
        song.pattern  = song.positions[song.position];
        song.patternBreak = 255;
        printS("Starting playback...\n");
        init = 1;
        onTick();
    }

    for (uint32 i = 0; i < spb;) {
        //Остаточный размер MME буфера (Отрендерить до конца) || Сколько осталось отрендерить до конца такта
        uint32 maxToRender = min(spb - i, song.spt - song.tickRenderCount);


        resample(buff1 + i, maxToRender, getChannel(0));
        resample(buff2 + i, maxToRender, getChannel(1));
        resample(buff3 + i, maxToRender, getChannel(2));
        resample(buff4 + i, maxToRender, getChannel(3));

        i                    += maxToRender;
        song.tickRenderCount += maxToRender;

        if (song.tickRenderCount == song.spt) onTick();
    }

    for (uint32 i = 0; i < spb; i++)
    {
        float stereoMix = 0.25;
        float l = buff1[i] / 2 + buff4[i] / 2;
        float r = buff2[i] / 2 + buff3[i] / 2;

        buff[i].l = ((int8)(l * 0.75 + r * 0.25)) * 100;
        buff[i].r = ((int8)(r * 0.75 + l * 0.25)) * 100;


        //b[bp++] = (int8)((buff1[i]) / 1) ^ 128;
        //b[bp++]   = (int8)((buff1[i] + buff2[i] + buff3[i] + buff4[i]) / 4) ^ 128;
        //buff[i].l = (int8)((buff1[i] + buff2[i] + buff3[i] + buff4[i]) / 4) ^ 128;
        //buff[i].r = (int8)((buff1[i] + buff2[i] + buff3[i] + buff4[i]) / 4) ^ 128;
        //if (bp == 44100 * 6) {
            //fatal(0, "buffEnd");
            //WriteToFile(b, "F:\\test\\test.raw");
        //}
        //buff[i].l = ((int8)buff4[i]) ^ 128;
        //buff[i].r = 0;

        //continue;
        //buff[i].l = (((((buff1[i] ^ 128) + (buff4[i] ^ 128)) * (1 - stereoMix))
        //           + (((buff2[i] ^ 128) + (buff3[i] ^ 128)) * (stereoMix))) / 2);
        //buff[i].r = (((((buff1[i] ^ 128) + (buff4[i] ^ 128)) * (stereoMix))
        //           + (((buff2[i] ^ 128) + (buff3[i] ^ 128)) * (1 - stereoMix))) / 2);
    }

    free(buff1);
    free(buff2);
    free(buff3);
    free(buff4);
}
//===


//for (uint32 i = 0; i < A_SPB; i++) { buff[i].l = buffr[i] ^ 128; buff[i].r = buffr[i] ^ 128; } return;
//printFormat("Sample \"%-22s\" of size %-6i has been loaded.\n", 2, song.samples[i].name, song.samples[i].length);
//Load & print patterns data
//printS("Patterns:");
        //printFormat("\n| Pattern %i:\n", 1, i);
            //printS("|");
            //printRow();
            //Print note
            //cstr noteName = "...";
            //if (note->period != 0) for (int m = 0; m < 36; m++) if (note->period == tuning0[m]) noteName = noteNames[m + 1];

            //printFormat(" %s %-2i %01X%02X |", 4, noteName, note->sample, note->effect, note->effectArg);
            //printC('\n');
        //printS("\n");

//buff[i].l = DIV2(buffl[i]) + DIV2(buffl2[i]);
//buff[i].r = DIV2(buffr[i]) + DIV2(buffr2[i]);
//update();
//printRow();
//buff[i].l = (buffl[i]);
//buff[i].r = (buffr2[i]);
//printFormat("NextRow %i\n", 1, song.ticker / 6);
//printFormat("NextTick %i\n", 1, song.ticker);

    //return;
    //for (size_t i = 0; i < A_SPB; i++)
    //{
    //    uint8 tmp = (buff[i].l * 0.15) + buff[i].r * 0.85;
    //    uint8 tmp2 = (buff[i].r * 0.15) + buff[i].l * 0.85;
    //    buff[i].l = tmp2;
    //    buff[i].r = tmp;
    //}

    //for (size_t i = 0; i < A_SPB; i++)
    //{
    //    buff[i].l = buffl[i] / 2;
    //    buff[i].r = buffr[i] / 2;
    //}
    //for (size_t i = 0; i < A_SPB; i++)
    //{
    //    buff[i].l += buffl[i] / 2;
    //    buff[i].r += buffr[i] / 2;
    //}

    //return;

//===Unfinished 128K Samples version
// 
// uint16 frac = channel->progress & 0x00003FFF;
// uint32 intg = channel->progress & 0x7FFFC000;
// uint16 step = MUL16K(srcSR) / A_SR;
// uint16 srcIndex = ((channel->progress & 0x00003FFF) < 8192) ? DIV16K(channel->progress) : DIV16K(channel->progress) + 1;