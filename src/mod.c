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
        readFile(file, 22, song.samples[i].name);
        song.samples[i].length      = MUL2(readWord(file));
        song.samples[i].finetune    = readByte(file);
        song.samples[i].volume      = readByte(file);
        song.samples[i].repeatStart = MUL2(readWord(file));
        song.samples[i].repeatEnd   = MUL2(readWord(file));
    }

    //Song length in patterns
    readFile(file, 1, &song.positionCount);
    //Magic byte
    readByte(file);
    //Pattern map
    readFile(file, 128, &song.positions);
    //File format
    readFile(file, 4, &song.magic);

    //Print song info
    printFormat("Handle: %i.\nSize: %i.\nSong: \"%s\"\nMagic: \"%s\"\nSamples:\n", 4, file, fileSize, song.name, song.magic);

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

    for (uint32 i = 0; i < song.patternCount; i++) {
        for (uint32 j = 0; j < 64; j++) {
            for (size_t n = 0; n < 4; n++) {
                Note* note  = &song.patterns[i][(j << 2) + n];
                uint32 rawNote  = (readDWord(file));
                note->sample    = ((rawNote & 0b11110000000000000000000000000000) >> 24);
                note->period    = ((rawNote & 0b00001111111111110000000000000000) >> 16);
                note->sample   |= ((rawNote & 0b00000000000000001111000000000000) >> 12);
                note->effect    = ((rawNote & 0b00000000000000000000111100000000) >> 8);
                note->effectArg = ((rawNote & 0b00000000000000000000000011111111) >> 0);
            }
        }
    }

    // ===
    // Samples
    // ===

    //Load samples
    printS("Message:\n");
    for (size_t i = 1; i < 32; i++)
    {
        if (song.samples[i].length > 0) {
            song.samples[i].data = malloc(song.samples[i].length);
            if (song.samples[i].data == 0) {
                printS("Could not allocate memory for samples.\n");
                hangThread();
            }

            readFile(file, song.samples[i].length, song.samples[i].data);

            //Signed -> unsigned sample conversion.
            //for (size_t j = 0; j < song.samples[i].length; j++) song.samples[i].data[j] ^= 128;
        }
        printFormat("%02i | %-22s |\n", 2, i, song.samples[i].name);
    }

    printS("EOF\n");
    CloseHandle(file);
}

//===

//=== Play ===//

uint8 uint8toint4[] = {0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1};

void resample(int8* buff, uint32 renderCount, uint16 period, Channel* chan) {
    if (!chan->playing || period == 0) return;
    InstrSample*
        sample = getSample(chan->note.sample);
    uint32
        srcSR = 3546895 / period,// (+uint8toint4[getSample(chan->note.sample)->finetune]),
        step = MUL64K(srcSR) / A_SR,
        srcSize = (sample->repeatEnd < 2) ? sample->repeatEnd : sample->length;

    for (uint32 i = 0; i < renderCount; i++) {
        //Correct rounding variant (If srcSize == 65535, this may lead in overflow...)
        //((chan->progress & 65535) < 32767) ? DIV64K(chan->progress) : DIV64K(chan->progress) + 1; 
        uint16 index = DIV64K(chan->progress);
        if (index >= srcSize) {
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
        chan->progress += step;
    }
}

InstrSample *getSample(uint8 sample) {
    return (sample < 32) ? (&song.samples[sample]) : (&song.samples[0]);
}
Note getNote(uint8 channel) {
    return song.patterns[song.pattern][MUL4(song.row) + channel];
}
Channel *getChannel(uint8 channel) {
    return &song.channels[channel & 3];
}

void loopSong() {
    song.position = 0;
}

void processChannel(uint8 channel) {
    Note newNote = getNote(channel);
    Channel* chan = getChannel(channel);

    if (newNote.sample != 0 || newNote.period != 0) {
        chan->progress = 0;
        chan->playing = 1;
        if (newNote.sample != 0) {
            chan->note.sample = newNote.sample;
            chan->volume = getSample(newNote.sample)->volume;
        }
        if (newNote.period != 0) {
            chan->note.period = newNote.period;
        }
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
    Note note = song.channels[channel].note;

    resample(
        buff,
        buffSize,
        note.period,
        getChannel(channel)
    );
}

void fillBuffer(LRSample* buff, HWAVEOUT h) {
    static uint32 clck = 0, init = 0;
    uint8 buffl[A_SPB] = { 0 }, buffr[A_SPB] = { 0 }, buffl2[A_SPB] = { 0 }, buffr2[A_SPB] = { 0 };

    if (!init) {
        song.tempo = 125;
        song.tpr = 6;
        song.tps = MUL2(song.tempo) / 5;
        song.pattern = song.positions[0];
        for (size_t i = 0; i < 4; i++) song.channels[i].volume = 64;

        printS("Starting playback...\n");
        init = 1;
    }

    for (uint32 i = 0; i < A_SPB;) {
        uint32 bpt = (A_SR / song.tps); //Bytes per tick
        //Остаточный размер MME буфера (Отрендерить до конца) || Сколько осталось отрендерить до конца такта
        uint32 maxToRender = min(A_SPB - i, bpt - song.tickRenderCount);

        renderChannel(0, buffl + i, maxToRender);
        renderChannel(1, buffr + i, maxToRender);
        renderChannel(2, buffr2 + i, maxToRender);
        renderChannel(3, buffl2 + i, maxToRender);

        i += maxToRender;
        song.tickRenderCount += maxToRender;

        if (song.tickRenderCount > bpt) MessageBoxA(0, "song.bytesPerTick < song.alreadyRendered!", "ERROR!", MB_ICONERROR);
        else if (song.tickRenderCount == bpt) {
            if (!(song.ticker % song.tpr)) {
                update();
                printRow();
                song.row++;
            }
            else for (uint32 i = 0; i < 4; i++) {
                if (getNote(i).effect != 0 && getNote(i).effectArg != 0) getChannel(i)->lastEffect = getNote(i);
                processTickFX(i);
            }

            song.tickRenderCount = 0;
            song.ticker++;
        }
    }

    for (uint32 i = 0; i < A_SPB; i++)
    {
        buff[i].l = buffl[i] ^ 128;
        buff[i].r = buffr[i] ^ 128;
    } //return;
    float stereoMix = 0.20;
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