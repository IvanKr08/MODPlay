#include "mod.h"

Song song;

//=== Load file ===//

static void readFile(HANDLE file, uint32 count, void* buff) {
    BOOL readResult = ReadFile(file, buff, count, 0, 0);
    if (readResult == FALSE) {
        printS("File read error.\n");
        hangThread();
    }
}
static uint8 readByte(HANDLE file) {
    uint8 val;
    readFile(file, 1, &val);
    return val;
}
static uint16 readWord(HANDLE file) {
    uint16 val;
    readFile(file, 2, &val);
    return (val >> 8) | ((val & 255) << 8);
}
static uint32 readDWord(HANDLE file) {
    uint32 val;
    readFile(file, 4, &val);
    return ((val >> 24) & 0xff) | ((val << 8) & 0xff0000) | ((val >> 8) & 0xff00) | ((val << 24) & 0xff000000);
}

void loadSong(wstr fileName) {
    HANDLE file;
    uint32 fileSize;

    //Open file
    file = CreateFileW(fileName, FILE_READ_DATA, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        printS("Could not open file.\n");
        hangThread();
    }
    fileSize = GetFileSize(file, 0);
    
    //Song name
    readFile(file, 20, song.name);

    //Sample headers
    for (uint32 i = 1; i < 32; i++) {
        InstrSample *sample = &song.samples[i];
        readFile(file, 22, sample->name);
        sample->length      = MUL2(readWord(file));
        sample->finetune    =     (readByte(file));
        sample->volume      =     (readByte(file));
        sample->repeatStart = MUL2(readWord(file));
        sample->repeatEnd   = MUL2(readWord(file));
        
        //No one-shot loops, sorry.
        sample->repeatEnd += sample->repeatStart;
        if (sample->repeatEnd > 2) sample->length = sample->repeatStart + sample->repeatEnd;
    }

    //Song length in patterns
    song.positionCount = readByte(file);
    //Magic byte
    song.resetPos = readByte(file);
    //Pattern map
    readFile(file, 128, &song.positions);
    //File format
    readFile(file, 4, &song.magic);

    //Print song info
    printFormat("Handle: %i.\nSize: %i.\nSong: \"%s\"\nMagic: \"%s\"\nSamples:\nReset pos: %i\n", 5, file, fileSize, song.name, song.magic, song.resetPos);

    //Print sample info
    for (uint32 i = 1; i < 32; i++) {
        printFormat("%0-2i | Name: \"%-22s\"\n", 2,
            i,
            song.samples[i].name
        );
        printFormat("%0-2i | Size in bytes: %-6i | Finetune: %-1X | Volume: %-2i | Repeat start: %-6i | Repeat end: %-6i |\n\n", 6,
            i,
            song.samples[i].length,
            song.samples[i].finetune,
            song.samples[i].volume,
            song.samples[i].repeatStart,
            song.samples[i].repeatEnd
        );
    }

    // ===
    // Patterns
    // ===

    //Print pattern info and find its count
    printFormat("Song length: %i.\nSong positions:\n", 1, song.positionCount);
    for (uint32 i = 0; i < 8; i++) {
        printS("| ");
        for (uint32 j = 0; j < 16; j++) {
            uint8 pattern = song.positions[MUL8(i) + j];
            if (song.patternCount < pattern) song.patternCount = pattern;
            printFormat("%-3i ", 1, pattern);
        }
        printS("|\n");
    }
    printC('\n');
    song.patternCount++;
    printFormat("Pattern count: %i.\n", 1, song.patternCount);

    //Alloc pattern buffer
    song.patterns = malloc(sizeof(Pattern) * song.patternCount);
    if (song.patterns == 0) {
        printS("Could not allocate memory for patterns.\n");
        hangThread();
    }

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
                    if (notes[m] <= period) {
                        note->note = m + 1;
                        break;
                    }
                }
                else note->note = 0;
            }
        }
    }

    // ===
    // Samples
    // ===

    //Load samples
    printS("Message:\n");
    for (uint32 i = 1; i < 32; i++)
    {
        if (song.samples[i].length > 0) {
            song.samples[i].data = malloc(song.samples[i].length);
            if (song.samples[i].data == 0) {
                printS("Could not allocate memory for samples.\n");
                hangThread();
            }

            readFile(file, song.samples[i].length, song.samples[i].data);
        }
        printFormat("%02i | %-22s |\n", 2, i, song.samples[i].name);
    }

    printS("EOF\n");
    CloseHandle(file);
}

//===

//=== Play ===//

//*Replace with macros
InstrSample* getSample(uint8 sample) {
    return &song.samples[sample];
}
Note getNote(uint8 channel) {
    return song.patterns[song.pattern][MUL4(song.row) + channel];
}
Channel* getChannel(uint8 channel) {
    return &song.channels[channel];
}

void loopSong() {
    printS("Reset.\n");
    song.position = song.resetPos;
}

void resample(int8* buff, uint32 renderCount, Channel* chan) {
    if (!chan->playing) return;

    InstrSample* sample = getSample(chan->sample);

    for (uint32 i = 0; i < renderCount; i++) {
        uint16 index = DIV64K(chan->progress);
        if (index >= sample->length) {
            if (sample->repeatEnd > 2) {
                chan->progress = MUL64K(sample->repeatStart);
                index = 0;
            }
            else {
                chan->playing = 0;
                return;
            }
        }
        buff[i] = DIV64(sample->data[index] * chan->volume);
        chan->progress += chan->currentStep;
    }
}

void processChannel(uint8 channel) {
    Note     note = getNote(channel);
    Channel* chan = getChannel(channel);

    chan->effect    = note.effect;
    chan->effectArg = note.effectArg;

    if (note.sample != 0) {
        InstrSample* sample = getSample(note.sample);

        chan->sample   = note.sample;
        chan->volume   = sample->volume;
        chan->finetune = sample->finetune;
    }

    if (note.note != 0) {
        chan->progress    = 0;
        chan->playing     = 1;
        chan->basePeriod  = periods[(note.note - 1) + (chan->finetune * 84)];
        chan->currentFreq = 56750320 / chan->basePeriod;
        chan->currentStep = MUL64K(chan->currentFreq) / A_SR;
    }

    processRowFX(channel);
}

void update() {
    //Update row and position
    if (song.row >= 64) {
        song.row = 0;
        song.position++;
        if (song.position >= song.positionCount) loopSong();
        song.pattern = song.positions[song.position];
        printFormat("Pattern: %3i, position: %3i\n", 2, song.pattern, song.position);
    }

    //Update channels
    for (uint32 i = 0; i < 4; i++) processChannel(i);
}

void renderChannel(uint8 channel, uint8 *buff, uint32 buffSize) {
    resample(
        buff,
        buffSize,
        getChannel(channel)
    );
}

void fillBuffer(LRSample* buff, HWAVEOUT h) {
    static bool init = 0;
    uint8
        buffl[A_SPB]  = { 0 },
        buffr[A_SPB]  = { 0 },
        buffl2[A_SPB] = { 0 },
        buffr2[A_SPB] = { 0 };

    if (!init) {
        song.tempo   = 125;
        song.tpr     = 6;
        song.tps     = 50;
        song.bpt     = A_SR / 50;
        song.pattern = song.positions[0];
        for (uint8 i = 0; i < 4; i++) getChannel(i)->channelID = i;
        printS("Starting playback...\n");
        init = 1;
    }

    for (uint32 i = 0; i < A_SPB;) {
        //Остаточный размер MME буфера (Отрендерить до конца) || Сколько осталось отрендерить до конца такта
        uint32 maxToRender = min(A_SPB - i, song.bpt - song.tickRenderCount);

        renderChannel(0, buffl + i, maxToRender);
        renderChannel(1, buffr + i, maxToRender);
        renderChannel(2, buffr2 + i, maxToRender);
        renderChannel(3, buffl2 + i, maxToRender);

        i                    += maxToRender;
        song.tickRenderCount += maxToRender;

        if (song.tickRenderCount == song.bpt) {
            if (!(song.ticker % song.tpr)) {
                update();
                printRow();
                song.row++;
            }
            for (uint32 i = 0; i < 4; i++) {
                processTickFX(i);
            }

            song.tickRenderCount = 0;
            song.ticker++;
        }
    }

    float stereoMix = 0.25;
    for (uint32 i = 0; i < A_SPB; i++)
    {
        buff[i].l = ((((buffl[i] ^ 128) + (buffl2[i] ^ 128)) * (1 - stereoMix))
                   + (((buffr[i] ^ 128) + (buffr2[i] ^ 128)) * (stereoMix))) / 2;
        buff[i].r = ((((buffl[i] ^ 128) + (buffl2[i] ^ 128)) * (stereoMix))
                   + (((buffr[i] ^ 128) + (buffr2[i] ^ 128)) * (1 - stereoMix))) / 2;
    }
}


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