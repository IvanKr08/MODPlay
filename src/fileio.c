// Prevent clipping based on number of channels... If all channels are playing at full volume, "256 / #channels"
// is the maximum possible sample pre-amp without getting distortion (Compatible mix levels given).
// The more channels we have, the less likely it is that all of them are used at the same time, though, so cap at 32...
//m_nSamplePreAmp = Clamp(256 / m_nChannels, 32, 128);

#include <Windows.h>
#include <shlwapi.h>
#include "fileio.h"
#include "console.h"
#include "mod.h"

#define P_INTERNALFILE 0
#define MAKEMODMAGIC(cstr4) (cstr4[3] | (cstr4[2] << 8) | (cstr4[1] << 16) | (cstr4[0] << 24))
int periodCompare(const void *arg1, const void *arg2) { return *((uint16 *)arg2) - *((uint16 *)arg1); }

static wstr debugName =
//L"F:\\Music\\MODULE\\MOD\\occ_san_geen.mod";
//L"F:\\Music\\MODULE\\MOD\\space_debris.mod";
//L"F:\\Music\\MODULE\\MOD\\hymn_to_aurora.mod";
//L"F:\\Music\\MODULE\\finetune.mod";
L"C:\\Users\\IvanKr08\\Desktop\\VibratoReset.mod";
//L"F:\\Music\\MODULE\\MOD\\GSLINGER.MOD";
//L"F:\\Music\\MODULE\\MOD\\space muzaxx.mod";
//L"F:\\Music\\MODULE\\C5.MOD";
//L"F:\\Music\\MODULE\\MOD\\emax-doz.mod";
//L"F:\\Music\\MODULE\\MOD\\intro_number_61.mod";
//L"F:\\Music\\MODULE\\MOD\\eye of the tiger.mod";
//L"F:\\Music\\MODULE\\MOD\\SHADOWRU.MOD";
//L"F:\\Music\\MODULE\\strshine2.mod";
//L"F:\\Music\\MODULE\\vibrato.mod";
//L"F:\\Music\\MODULE\\tremolo.mod";
//L"F:\\Music\\MODULE\\00006.mod";
//L"F:\\Music\\MODULE\\MOD\\fairlight.MOD";
//L"F:\\Music\\MODULE\\MOD\\DOPE.MOD";
//L"F:\\Music\\MODULE\\speedPrecTest.MOD";
//L"G:\\OpenMPTWiki\\OpenMPTWiki\\resources.openmpt.org\\player_tests\\mod\\AmigaLimitsFinetune.mod";

//=== File IO ===//
void dumpSound(apisample* buff, uint32 totalSamples) {
    static HANDLE f = 0;
    static bool init = 0;

    if (!init) {
        f = CreateFileA("C:\\Users\\IvanKr08\\desktop\\SNDDUMP.RAW", GENERIC_ALL, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        init = 1;
    }

    WriteFile(f, buff, totalSamples * A_SAMPLESIZE, 0, 0);
}
static void   readFile(HANDLE file, uint32 count, void* buff) {
#if P_INTERNALFILE
    static uint32 offset;
    for (size_t i = 0; i < count; i++, offset++) ((byte*)buff)[i] = ((byte*)file)[offset];
#else
    fatal(!ReadFile(file, buff, count, 0, 0), "File read error");
#endif
}
static uint8  readByte(HANDLE file) {
    uint8 val;
    readFile(file, 1, &val);
    return val;
}
static uint16 readWordBE(HANDLE file) {
    uint16 val;
    readFile(file, 2, &val);
    return (val >> 8) | ((val & 0xFF) << 8);
}
static uint32 readDWordBE(HANDLE file) {
    uint32 val;
    readFile(file, 4, &val);
    return ((val >> 24) & 0xFF) | ((val << 8) & 0xFF0000) | ((val >> 8) & 0xFF00) | ((val << 24) & 0xFF000000);
}
//=== ===//

static void loadMOD(HANDLE file) {
    readFile(file, 20, song.name);



    //Sample headers
    for (uint32 i = 1; i < 32; i++) {
        Sample* sample = &song.samples[i];
        readFile(file, 22, sample->name);
        sample->length      = MUL2(readWordBE(file));
        sample->finetune    =     (readByte(file));
        sample->volume      =     (readByte(file));
        sample->loopStart   = MUL2(readWordBE(file));
        sample->loopSize    = MUL2(readWordBE(file));
        sample->loopEnd     = sample->loopStart + sample->loopSize;
        sample->hasLoop     = 1;

        //Stealed from OpenMPT
        if (sample->loopStart >= sample->length  ||
            sample->loopStart >= sample->loopEnd ||
            sample->loopEnd   <  4               ||
            sample->loopEnd - sample->loopStart < 4) sample->hasLoop = 0;

        if (!sample->hasLoop) sample->loopEnd = sample->length; //No one-shot loops, sorry.
    }
    //===



    song.positionCount = readByte(file);
    song.resetPos      = readByte(file);
    readFile(file, 128, &song.positions);



    //Calc channel count
    readFile(file, 4, &song.magic);
    printFormat("Magic: \t\t%s\n", 1, song.magic);
    
    uint32 songMagic = MAKEMODMAGIC(song.magic);
    if (songMagic == MAKEMODMAGIC("M.K.") ||
        songMagic == MAKEMODMAGIC("M!K!"))                      song.channelCount = 4;
    else if ((songMagic & 0xFFFFFF) == MAKEMODMAGIC("\0CHN"))   song.channelCount = (songMagic >> 24) - '0';
    else if ((songMagic & 0xFFFF)   == MAKEMODMAGIC("\0\0CH"))  song.channelCount = ((songMagic >> 24) - '0') * 10 + ((songMagic >> 16) - '0');
    else                                                        fatalCode(songMagic, "Unknown file magic.");
    
    printFormat("Channels: \t%i\n", 1, song.channelCount);
    ARRALLOC(song.channels, song.channelCount);
    memset(song.channels, 0, song.channelCount * sizeof(Channel));
    //===



    //Print song info
    printS("Samples:\nNo | Name                   | Vol    FnTune   | Length    Lp. Strt  Lp. End  |\n");
    for (uint32 i = 1; i < 32; i++)
        printFormat("%0-2i | %-22s | V: %-2i  FTUN: %-2i | L: %-5i  S: %-5i  E: %-5i |\n", 7,
            i,
            song.samples[i].name,
            song.samples[i].volume,
            int4[song.samples[i].finetune],
            song.samples[i].length,
            song.samples[i].loopStart,
            song.samples[i].loopEnd
        );
    //===



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
    printFormat("Song: \t\t\"%s\"\nReset: \t\t%i\n", 2, song.name, song.resetPos);
    printFormat("Pattern count: \t%i\nSong length: \t%i\n", 2, song.patternCount, song.positionCount);
    //===
    
    
    
    //Read patterns
    ARRALLOC(song.patterns, song.patternCount * 64U * song.channelCount);
    for (uint32 i = 0; i < (song.patternCount * 64U * song.channelCount); i++) {
        Note  *note     = &song.patterns[i];
        uint16 period   = 0, *noteAddr = 0;;
        uint32 rawNote  = (readDWordBE(file));
        note->sample    = ((rawNote & 0b11110000000000000000000000000000) >> 24);
        period          = ((rawNote & 0b00001111111111110000000000000000) >> 16);
        note->sample   |= ((rawNote & 0b00000000000000001111000000000000) >> 12);
        note->effect    = ((rawNote & 0b00000000000000000000111100000000) >> 8);
        note->effectArg = ((rawNote & 0b00000000000000000000000011111111));
        note->note      = 0;

        if (period)   noteAddr   = bsearch(&period, notePeriods, 84, 2, periodCompare);
        if (noteAddr) note->note = (uint16)(noteAddr - notePeriods) + 1;
    }
    //===



    //Load samples
    for (uint32 i = 1; i < 32; i++) {
        if (song.samples[i].length > 0) {
            ARRALLOC(song.samples[i].data, song.samples[i].length);
            readFile(file, song.samples[i].length, song.samples[i].data);
        }
    }
    //===



    printS("EOF\n");
#if !P_INTERNALFILE
    CloseHandle(file);
#endif



    //Init playback variables
    song.tempo          = 125;
    song.ticksPerRow    = 6;
    song.samplesPerTick = MUL512(A_SAMPLERATE) / (MUL1K(song.tempo) / 5);
    song.position       = 0;
    song.row            = 0;
    song.patternBreak   = 255;
    song.positionJump   = 255;
    song.pattern        = song.positions[song.position];
    for (uint8 i = 0, chan = 0; i < song.channelCount; i++, chan = i % 4)
        GETCHANNEL(i)->panning = (chan == 1 || chan == 2) ? 192 : 64;

    printS("Starting playback...\n");
    onTick();
    //===
}

void loadSongFile() {
    HANDLE file     = 0;
    DWORD  fileSize = 0;

#if P_INTERNALFILE
    {
        HANDLE modul = GetModuleHandleA(0);
        HRSRC  res   = FindResourceA(modul, "IDF_SONG", RT_RCDATA);
        file         = LockResource(LoadResource(modul, res));
        fileSize     = SizeofResource(modul, res);
    }
#else 
    int   argc = 0;
    wstr *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    wstr  fileName = L"";

    if (argc > 1)   fileName = argv[1];
#ifdef _DEBUG
    fileName = debugName;
#endif
    //fileName = debugName;
    //Fix warning
    if (!fileName) return;

    fatal(
        fileName == L"" || !fileName,
        "File not specified.");
    fatal(
        GetFileAttributesW(fileName) == -1,
        "File not found.");
    fatal(
        (file = CreateFileW(fileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE,
        "Could not open file.");
    fatal(
        (fileSize = GetFileSize(file, 0)) > 2097152 /* 2MiB */,
        "File too big.");

    wstr type = PathFindExtensionW(fileName);

    fatal(
        lstrcmpiW(type, L".MOD"),
        "Not a .MOD.");

    printS("File: \t\t");
    printW(fileName);
    printC('\n');
    printS("Extension: \t");
    printW(type);
    printFormat("\n\nHandle: \t0x%X\nSize: \t\t%u\n", 2, file, fileSize);
    setConsoleTitle(fileName);

    loadMOD(file);
#endif
}