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
#define MAKEMAGIC(cstr4) (cstr4[3] | (cstr4[2] << 8) | (cstr4[1] << 16) | (cstr4[0] << 24))
int periodCompare(const void *arg1, const void *arg2) { return *((uint16 *)arg2) - *((uint16 *)arg1); }

static wstr debugName =
//L"F:\\Music\\MODULE\\MOD\\occ_san_geen.mod";
//L"F:\\Music\\MODULE\\MOD\\space_debris.mod";
//L"F:\\Music\\MODULE\\MOD\\hymn_to_aurora.mod";
//L"F:\\Music\\MODULE\\finetune.mod";
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
//L"F:\\Music\\MODULE\\MODCONV\\00006.mod";
//L"F:\\Music\\MODULE\\MOD\\fairlight.MOD";
//L"F:\\Music\\MODULE\\MOD\\MCHAN\\DOPE.MOD";
//L"F:\\Music\\MODULE\\speedPrecTest.MOD";
L"F:\\Music\\MODULE\\MOD\\MCHAN\\far away smoothloop.mod";
//L"F:\\Music\\MODULE\\MOD\\MCHAN\\experience.mod";
//L"F:\\Music\\MODULE\\MODCONV\\aryx.mod";
//L"F:\\Music\\MODULE\\S3M\\unreal - wormhole part.S3M";
//L"F:\\Music\\MODULE\\S3M\\aryx.S3M";
//L"F:\\Music\\MODULE\\S3M\\2ND_PM.s3m";
//L"F:\\Music\\MODULE\\S3M\\strshine.s3m";
//L"F:\\Music\\MODULE\\S3M\\ChronologieP4.s3m";
//L"F:\\Music\\MODULE\\S3M\\autonom.s3m";
//L"F:\\Music\\MODULE\\S3M\\celestial_fantasia.s3m";

void dumpSound(apisample *buff, uint32 totalSamples) {
    static HANDLE f = 0;
    static bool init = 0;

    if (!init) {
        f = CreateFileA("C:\\Users\\IvanKr08\\desktop\\SNDDUMP.RAW", GENERIC_ALL, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        init = 1;
    }

    WriteFile(f, buff, totalSamples * A_SAMPLESIZE, 0, 0);
}

//=== File IO ===//
static void   skip(HANDLE file, int32 amount) {
    SetFilePointer(file, amount, 0, FILE_CURRENT);
}
static void   setPos(HANDLE file, int32 pos) {
    SetFilePointer(file, pos, 0, FILE_BEGIN);
}
static void   readFile(HANDLE file, uint32 count, void* buff) {
#if P_INTERNALFILE
    static uint32 offset;
    for (size_t i = 0; i < count; i++, offset++) ((byte*)buff)[i] = ((byte*)file)[offset];
#else
    fatal(!ReadFile(file, buff, count, 0, 0), "File read error");
#endif
}

static uint8  read8(HANDLE file) {
    uint8 val;
    readFile(file, 1, &val);
    return val;
}

static uint16 read16B(HANDLE file) {
    uint16 val;
    readFile(file, 2, &val);
    return (val >> 8) | ((val & 0xFF) << 8);
}
static uint32 read32B(HANDLE file) {
    uint32 val;
    readFile(file, 4, &val);
    return ((val >> 24) & 0xFF) | ((val << 8) & 0xFF0000) | ((val >> 8) & 0xFF00) | ((val << 24) & 0xFF000000);
}

static uint16 read16L(HANDLE file) {
    uint16 val;
    readFile(file, 2, &val);
    return val;
}
static uint32 read32L(HANDLE file) {
    uint32 val;
    readFile(file, 4, &val);
    return val;
}
//=== ===//

//Тут серьезно не хватает загрузчика 15-семпловых файлов SoundTracker и нормального обнаружение дефектных модулей
static void loadMOD(HANDLE file) {
    song.modType = MT_GENMOD;

    ARRALLOC(song.name, 20);
    readFile(file, 20, song.name);

    //Sample headers
    ARRALLOCZERO(song.samples, 32);
    for (uint32 i = 1; i < 32; i++) {
        Sample* sample = GETSAMPLE(i);
        ARRALLOC(sample->name, 23);
        readFile(file, 22, sample->name);
        sample->name[22] = 0;

        sample->length      = MUL2(read16B(file));
        sample->finetune    =     (read8(file));
        sample->volume      =     (read8(file));
        sample->loopStart   = MUL2(read16B(file));
        sample->loopSize    = MUL2(read16B(file));
        sample->loopEnd     = sample->loopStart + sample->loopSize;
        sample->hasLoop     = 1;

        //Stealed from OpenMPT
        LIMIT(sample->loopEnd, sample->length);
        LIMIT(sample->loopStart, sample->length);

        if (sample->loopStart >= sample->length  ||
            sample->loopStart >= sample->loopEnd ||
            sample->loopEnd   <  4               ||
            sample->loopEnd - sample->loopStart < 4) sample->hasLoop = 0;

        if (!sample->hasLoop) sample->loopEnd = sample->length; //No one-shot loops, sorry.
    }
    //===



    song.positionCount = read8(file);
    song.resetPos      = read8(file);
    ARRALLOC(song.positions, 128);
    readFile(file, 128, song.positions);



    //Calc channel count
    readFile(file, 4, &song.magic);
    printFormat("Magic: \t\t%s\n", 1, song.magic);
    
    uint32 songMagic = MAKEMAGIC(song.magic);
    if (songMagic ==   MAKEMAGIC("M.K.") ||
        songMagic ==   MAKEMAGIC("M!K!"))                    song.channelCount = 4;
    else if ((songMagic & 0xFFFFFF) == MAKEMAGIC("\0CHN"))   song.channelCount = (songMagic >> 24) - '0';
    else if ((songMagic & 0xFFFF)   == MAKEMAGIC("\0\0CH"))  song.channelCount = ((songMagic >> 24) - '0') * 10 + ((songMagic >> 16) - '0');
    else                                                     fatalCode(songMagic, "Unknown file magic.");
    
    printFormat("Channels: \t%i\n", 1, song.channelCount);
    ARRALLOCZERO(song.channels, song.channelCount);
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
        uint32 rawNote  = (read32B(file));
        note->sample    = ((rawNote & 0b11110000000000000000000000000000) >> 24);
        period          = ((rawNote & 0b00001111111111110000000000000000) >> 16);
        note->sample   |= ((rawNote & 0b00000000000000001111000000000000) >> 12);
        note->effect    = ((rawNote & 0b00000000000000000000111100000000) >> 8);
        note->effectArg = ((rawNote & 0b00000000000000000000000011111111));
        note->note      = 0;
        note->vol       = 0;

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

    song.tempo          = 125;
    song.ticksPerRow    = 6;
    for (uint8 i = 0, chan = 0; i < song.channelCount; i++, chan = i % 4) GETCHANNEL(i)->panning = (chan == 1 || chan == 2) ? 192 : 64;
}

static void loadS3MPatterns(HANDLE file, uint16 *patPtrs) {
    for (uint8 i = 0; i < song.patternCount; i++) {
        setPos(file, MUL16(patPtrs[i]) + 2);

        for (uint8 row = 0; row < 64;) {
            uint8 what = read8(file);
            if (!what) {
                row++;
                continue;
            }

            uint8 channel = what & 0x1F;
            if (channel >= song.channelCount) {
                if (what & 0x20) skip(file, 2);
                if (what & 0x40) skip(file, 1);
                if (what & 0x80) skip(file, 2);
                continue;
            }

            //Add variable-sized pattern support
            Note *cell = &song.patterns[(i * 64 * song.channelCount) + (row * song.channelCount) + channel];

            //Note
            if (what & 0x20) {
                uint8 note = read8(file);

                if      (note  < 0xF0) note = (note & 0x0F) + 12 * (note >> 4) + 13;
                else if (note == 0xFF) note = 0;
                cell->note = note;

                cell->sample = read8(file);
                if (cell->sample > song.sampleCount || (cell->sample && !GETSAMPLE(cell->sample)->data)) {
                    printFormat("WARN: Invalid sample number! (%i)\n", 1, cell->sample);
                    cell->sample = 0;
                }
            }

            //Volume
            if (what & 0x40) {
                cell->vol         = read8(file);
                if (cell->vol < 65) cell->vol++;
                else                cell->vol = 0;
            }

            //Effect
            if (what & 0x80) {
                cell->effect    = read8(file);
                cell->effectArg = read8(file);

                switch (cell->effect) {
                    case 'A' - '@':
                        cell->effect = 0xF;
                        break;
                    case 'B' - '@':
                        cell->effect = 0xB;
                        break;
                    case 'C' - '@':
                        cell->effect = 0xD;
                        break;
                    case 'D' - '@':
                        cell->effect = 0xA;
                        break;
                    case 'E' - '@' + 999:
                        cell->effect = 0x2;
                        break;
                    case 'F' - '@' + 999:
                        cell->effect = 0x1;
                        break;
                    case 'G' - '@' + 999:
                        cell->effect = 0x3;
                        break;
                    case 'H' - '@':
                        cell->effect = 0x4;
                        break;
                    case 'O' - '@':
                        cell->effect = 0x9;
                        break;
                    case 'T' - '@':
                        cell->effect = 0xF;
                        break;
                    default:
                        cell->effect = cell->effectArg = 0;
                }
            }
        }
    }
}

static void loadS3M(HANDLE file) {
    song.modType = MT_ST3;

    uint8 tmp[16384];

    ARRALLOC(song.name, 28);
    readFile(file, 28, song.name);
    printFormat("Signature: \t0x%08X\n", 1, read32L(file));  //Sigs

    song.positionCount = read16L(file);
    song.sampleCount   = read16L(file);
    song.patternCount  = read16L(file);

    uint16 *smpPtrs, *patPtrs;
    ARRALLOC(smpPtrs, song.sampleCount);
    ARRALLOC(patPtrs, song.patternCount);
    ARRALLOC(song.positions, song.positionCount);
    ARRALLOCZERO(song.samples, song.sampleCount + 1);

    read16L(file);                  //Flags
    song.trackVer = read16L(file);
    read16L(file);                  //Sample sign
    readFile(file, 4, song.magic);  //SCRM signature
    read8(file);                    //Global vol

    song.ticksPerRow = read8(file);
    song.tempo       = read8(file);

    uint8 master = read8(file);   //Master vol + stereo flag
    bool  stereo = master >> 7;
    read8(file);   //Ultraclick
    uint8 hasPan = read8(file);   //252=read pan values in header, anything else ignores pan values in header
    read32L(file); //Reserved
    read32L(file); //Reserved

    read16L(file); //Special parapointer



    //Channel detecting
    uint8 chanSet[32];
    readFile(file, 32, chanSet);
    for (uint8 i = 0; i < 32; i++) {
        printFormat("%4i", 1, chanSet[i]);

        if (chanSet[i] == 255) continue;
        song.channelCount = i + 1;
    }
    printC('\n');
    ARRALLOCZERO(song.channels, song.channelCount);
    //===



    readFile(file, song.positionCount   , song.positions);
    readFile(file, song.sampleCount  * 2, smpPtrs);
    readFile(file, song.patternCount * 2, patPtrs);

    if      (!stereo)       for (uint8 i = 0; i < song.channelCount; i++) GETCHANNEL(i)->panning = 128;
    else if (hasPan == 252) for (uint8 i = 0; i < song.channelCount; i++) {
        uint8 pan = read8(file) & 0b1111;
        if (pan & 0b10000);
        else GETCHANNEL(i)->panning = ((pan) == 15) ? 256 : ((pan) * 17);
        pan &= 0b1111;

        printC('|');
        for (uint8 j = 0; j < pan; j++) printC(' ');
        printC('*');
        for (uint8 j = 0; j < 15 - pan; j++) printC(' ');
        printC('|');
        printC('\n');
    }
    else for (uint8 i = 0; i < song.channelCount; i++) GETCHANNEL(i)->panning = (chanSet[i] & 8) ? 204 : 51;

    for (size_t i = 0; i < song.sampleCount; i++) {
        setPos(file, MUL16(smpPtrs[i]));

        Sample *s = GETSAMPLE(i + 1);
        ARRALLOC(s->fileName, 13);
        
        uint8 type = read8(file);
        readFile(file, 12, s->fileName);
        s->fileName[12] = 0;

        //PCM
        if (type == 1) {
            uint32 sndPtr = read8(file);
                   sndPtr = (sndPtr << 16) + read16L(file);

            s->length       = read32L(file);
            s->loopStart    = read32L(file);
            s->loopEnd      = read32L(file);
            s->loopSize     = s->loopEnd - s->loopStart;
            s->volume       = read8(file);

            ARRALLOC(s->data, s->length);
            ARRALLOC(s->name, 28);

            read8(file);
            read8(file);

            s->hasLoop = read8(file) & 1;
            if (!s->hasLoop) s->loopEnd = s->length;

            s->baseFreq = read32L(file);
            if (!s->baseFreq) s->baseFreq = 8363;
            else if (s->baseFreq < 1024) s->baseFreq = 1024;

            readFile(file, 12, tmp);
            readFile(file, 28, s->name);
            read32L(file);

            
            setPos(file, MUL16(sndPtr));
            readFile(file, s->length, s->data);
            for (size_t i = 0; i < s->length; i++) s->data[i] = s->data[i] ^ 128;
        }
        else if (type == 0) {
            readFile(file, 35, tmp);
            ARRALLOC(s->name, 28);
            readFile(file, 28, s->name);
        }
    }

    ARRALLOCZERO(song.patterns, song.channelCount * 64 * song.patternCount);

    loadS3MPatterns(file, patPtrs);

    printS("Samples:\nNo | Name                         | Vol    Freq        | Length    Lp. Strt  Lp. End  |\n");
    for (uint32 i = 0; i < song.sampleCount + 1; i++)
        printFormat("%0-2i | %-28s | V: %-2i  FREQ: %-5i | L: %-5i  S: %-5i  E: %-5i |\n", 7,
            i,
            song.samples[i].name,
            song.samples[i].volume,
            song.samples[i].baseFreq,
            song.samples[i].length,
            song.samples[i].loopStart,
            song.samples[i].loopEnd
        );
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
        (file = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE,
        "Could not open file.");
    fatal(
        (fileSize = GetFileSize(file, 0)) > 2097152 /* 2MiB */,
        "File too big.");

    wstr type = PathFindExtensionW(fileName);

    printS("File: \t\t");
    printW(fileName);
    printC('\n');
    printS("Extension: \t");
    printW(type);
    printFormat("\nHandle: \t0x%X\nSize: \t\t%u\n", 2, file, fileSize);
    setConsoleTitle(fileName);

    if      (!lstrcmpiW(type, L".S3M")) loadS3M(file);
    else if (!lstrcmpiW(type, L".MOD")) loadMOD(file);
    else                                fatal(1, "Unknown extension");

    printS("Starting playback...\n");
    song.samplesPerTick = MUL512(A_SAMPLERATE) / (MUL1K(song.tempo) / 5);
    song.position       = 0;
    song.row            = 0;
    song.patternBreak   = 255;
    song.positionJump   = 255;
    song.pattern        = song.positions[song.position];
    onTick();
#endif
}