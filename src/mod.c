#include "utils.h"
#include "console.h"
#include "tables.h"
#include "mod.h"

//=== Load file

Song song;

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
    for (size_t i = 1; i < 32; i++) {
        readFile(file, 22, song.samples[i].name);
        song.samples[i].length      = readWord(file) << 1;
        song.samples[i].finetune    = readByte(file);
        song.samples[i].volume      = readByte(file);
        readWord(file);
        readWord(file);
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
    printFormat("Handle: %i.\nSize: %i.\nSong: \"%s\".\nSamples:\n", 3, file, fileSize, song.name);
    for (size_t i = 1; i < 32; i++) {
        printFormat("| Name: %-22s | Size in bytes: %-6i | Finetune: %-1X | Volume: %-2i |\n", 4, song.samples[i].name, song.samples[i].length, song.samples[i].finetune, song.samples[i].volume);
    }
    printC('\n');

    // ===
    // Patterns
    // ===

    //Print pattern info and find its count
    printFormat("Song length: %i.\nSong positions:\n", 1, song.positionCount);
    for (size_t i = 0; i < 8; i++) {
        printS("| ");
        for (size_t j = 0; j < 16; j++) {
            int pattern = song.positions[j + (i << 3)];
            if (song.patternCount < pattern) song.patternCount = pattern;
            printFormat("%-3i ", 1, pattern);
        }
        printS("|\n");
    }
    printC('\n');
    song.patternCount++;
    printFormat("Pattern count: %i.\nMagic: \"%s\".\n", 2, song.patternCount, song.magic);

    //Alloc pattern buffer
    song.patterns = malloc(sizeof(Pattern) * song.patternCount);
    if (song.patterns == 0) {
        printS("Could not allocate memory for patterns.\n");
        hangThread();
    }

    //Load & print patterns data
    //printS("Patterns:");
    for (size_t i = 0; i < song.patternCount; i++) {
        //printFormat("\n| Pattern %i:\n", 1, i);
        for (size_t j = 0; j < 64; j++) {
            //printS("|");
            for (size_t n = 0; n < 4; n++) {
                Note* note  = &song.patterns[i][(j << 2) + n];
                uint32 rawNote  = (readDWord(file));
                note->sample    = ((rawNote & 0b11110000000000000000000000000000) >> 24);
                note->period    = ((rawNote & 0b00001111111111110000000000000000) >> 16);
                note->sample   |= ((rawNote & 0b00000000000000001111000000000000) >> 12);
                note->effect    = ((rawNote & 0b00000000000000000000111100000000) >> 8);
                note->effectArg = ((rawNote & 0b00000000000000000000000011111111) >> 0);
                //printRow();
                //Print note
                //cstr noteName = "...";
                //if (note->period != 0) for (int m = 0; m < 36; m++) if (note->period == tuning0[m]) noteName = noteNames[m + 1];
                
                //printFormat(" %s %-2i %01X%02X |", 4, noteName, note->sample, note->effect, note->effectArg);
            }
            //printC('\n');
        }
        //printS("\n");
    }
    // ===
    // Samples
    // ===

    //Load samples
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
            for (size_t j = 0; j < song.samples[i].length; j++) song.samples[i].data[j] ^= 128;

            printFormat("Sample \"%-22s\" of size %-6i has been loaded.\n", 2, song.samples[i].name, song.samples[i].length);
        }
    }

    printS("EOF\n");
    CloseHandle(file);
}

//=== Play

//===Unfinished 128K Samples version
// 
// uint16 frac = channel->progress & 0x00003FFF;
// uint32 intg = channel->progress & 0x7FFFC000;
// uint16 step = MUL16K(srcSR) / A_SR;
// uint16 srcIndex = ((channel->progress & 0x00003FFF) < 8192) ? DIV16K(channel->progress) : DIV16K(channel->progress) + 1;

//#define ADDSAT8(val1, val2) ((val1) + (val2) > 127 ? 127)
void resample(uint8* src, uint32 srcSize, uint8* buff, uint32 renderCount, uint32 srcSR, Channel* channel) {
    uint32 i = 0;
    if (!channel->playing) goto lExit;
    uint32 step = MUL64K(srcSR) / A_SR;

    for (; i < renderCount; i++) {
        //If srcSize == 65535, this may lead in overflow...
        uint16 index = ((channel->progress & 65535) < 32767) ? DIV64K(channel->progress) : DIV64K(channel->progress) + 1; 
        if (index >= srcSize) {
            channel->playing = 0;
            goto lExit;
        }
        else buff[i] = ((uint8)(((int8)(src[index] - 128)) * (channel->volume) / 64) + 128);
        channel->progress += step;
    }

lExit:
    for (; i < renderCount; i++) buff[i] = 128;
}

Note getNote(uint8 channel) {
    return song.patterns[song.pattern][MUL4(song.row) + channel];
}

void loopSong() {
    song.tempo = 12;
    song.ticksRow = 7;
    song.ticksPerSecond = MUL2(song.tempo) / 5;
    song.position = 0;
    song.pattern = song.positions[0];
    for (size_t i = 0; i < 4; i++) song.channels[i].volume = 64;
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

    //Update notes
    for (size_t i = 0; i < 4; i++) {
        Note note = getNote(i);
        if (note.sample != 0 || note.period != 0) {
            song.channels[i].progress = 0;
            song.channels[i].playing = 1;
            if (note.sample != 0) song.channels[i].note.sample = note.sample;
            if (note.period != 0) song.channels[i].note.period = note.period;
        }
        if (note.effect == 0xC) {
            song.channels[i].volume = (note.effectArg > 64) ? 64 : note.effectArg;
        }
        else if (note.sample != 0) {
            song.channels[i].volume = song.samples[note.sample].volume;
        }
        if (note.effect == 0xF && note.effectArg < 32) {
            song.ticksRow = note.effectArg;
        }
    }
}

void renderChannel(uint8 channel, uint8 *buff, uint32 buffSize) {
    Note note = song.channels[channel].note;

    resample(
        song.samples[note.sample].data,
        song.samples[note.sample].length,
        buff,
        buffSize,
        (7093789.2 / (2 * note.period)),
        &song.channels[channel]
    );
}

void fillBuffer(LRSample* buff, HWAVEOUT h) {
    static uint32 clck = 0, init = 0;
    uint8 buffl[A_SPB] = { 0 }, buffr[A_SPB] = {0}, buffl2[A_SPB] = { 0 }, buffr2[A_SPB] = { 0 };

    if (!init) {
        song.tempo = 125;
        song.ticksRow = 6;
        song.ticksPerSecond = MUL2(song.tempo) / 5;
        song.pattern = song.positions[0];
        for (size_t i = 0; i < 4; i++) song.channels[i].volume = 64;

        printS("Starting playback...\n");
        //update();
        //printRow();
        init = 1;
    }

    for (uint32 i = 0; i < A_SPB;) {
        uint32 bpt = (A_SR / song.ticksPerSecond); //Bytes per tick
        //Остаточный размер MME буфера (Отрендерить до конца) || Сколько осталось отрендерить до конца такта
        uint32 maxToRender = min(A_SPB - i, bpt - song.alreadyRendered);

        renderChannel(0, buffl + i, maxToRender);
        renderChannel(1, buffr + i, maxToRender);
        renderChannel(2, buffr2 + i, maxToRender);
        renderChannel(3, buffl2 + i, maxToRender);

        i += maxToRender;
        song.alreadyRendered += maxToRender;

        if (song.alreadyRendered > bpt) MessageBoxA(0, "song.bytesPerTick < song.alreadyRendered!", "ERROR!", MB_ICONERROR);
        else if (song.alreadyRendered == bpt) {
            if (!(song.ticker % song.ticksRow)) {
                update();
                printRow();
                song.row++;
            }
            song.alreadyRendered = 0;
            song.ticker++;
        }
    }
    for (size_t i = 0; i < A_SPB; i++)
    {
        buff[i].l = DIV2(buffl[i]) + DIV2(buffl2[i]);
        buff[i].r = DIV2(buffr[i]) + DIV2(buffr2[i]);
        //buff[i].l = (buffl[i]);
        //buff[i].r = (buffr2[i]);
    }

    return;
    for (size_t i = 0; i < A_SPB; i++)
    {
        uint8 tmp = (buff[i].l * 0.15) + buff[i].r * 0.85;
        uint8 tmp2 = (buff[i].r * 0.15) + buff[i].l * 0.85;
        buff[i].l = tmp2;
        buff[i].r = tmp;
    }

    for (size_t i = 0; i < A_SPB; i++)
    {
        buff[i].l = buffl[i] / 2;
        buff[i].r = buffr[i] / 2;
    }
    for (size_t i = 0; i < A_SPB; i++)
    {
        buff[i].l += buffl[i] / 2;
        buff[i].r += buffr[i] / 2;
    }

    return;
}

//printFormat("NextRow %i\n", 1, song.ticker / 6);
//printFormat("NextTick %i\n", 1, song.ticker);