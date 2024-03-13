#include "console.h"
#include <Windows.h>
#include <stdarg.h>
//===

//#define WriteConsoleA()
static HANDLE stdi = 0, stdo = 0;

void setConsoleTitle(wstr title) {
    SetConsoleTitleW(title);
}

void consoleInit() {
    AllocConsole();
    stdo = GetStdHandle((DWORD)-11);
    stdi = GetStdHandle((DWORD)-10);
}

void handleInput() {
    uint32 events = 0;
    GetNumberOfConsoleInputEvents(GetStdHandle(-10), &events);

    for (; events > 0; events--) {
        uint32 numOfEvents = 0;
        INPUT_RECORD keyInfo;

        ReadConsoleInputW(GetStdHandle(-10), &keyInfo, 1, &numOfEvents);
        if (keyInfo.Event.KeyEvent.bKeyDown) {
            switch (keyInfo.Event.KeyEvent.wVirtualKeyCode) {
                case VK_ADD:
                case VK_PRIOR:
                    song.patternBreak = 0;
                    break;
                case VK_SUBTRACT:
                case VK_NEXT:
                    song.patternBreak = 0;
                    song.positionJump = (song.position - 1) >= 0 ? (song.position - 1) : 0; //(song.positionCount - 1)
                    break;
                case VK_RIGHT:
                    song.tempoModifier++;
                    if (keyInfo.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) song.tempoModifier += 15;
                    CLAMP(song.tempoModifier, -256, 1024);
                    printFormat("Tempo modifier: %i\n", 1, song.tempoModifier);
                    setTempo(song.tempo);
                    break;
                case VK_LEFT:
                    song.tempoModifier--;
                    if (keyInfo.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) song.tempoModifier -= 15;
                    CLAMP(song.tempoModifier, -256, 1024);
                    printFormat("Tempo modifier: %i\n", 1, song.tempoModifier);
                    setTempo(song.tempo);
                    break;
            }
        }
    }
}

void printI(int32 value) {
    char num[16];
    _itoa_s(value, num, 16, 10);
    printS(num);
    printC('\n');
}
void printC(char value) {
    WriteConsoleA(stdo, &value, 1, 0, 0);
}
void printS(cstr string) {
    WriteConsoleA(stdo, string, strlen(string), 0, 0);
}
void printW(wstr string) {
    WriteConsoleW(stdo, string, wcslen(string), 0, 0);
}
void printFormat(cstr format, uint32 count, ...) {
    char string[1024];
    va_list args;
    
    va_start(args, count);
    vsprintf_s(string, 1024, format, args);
    va_end(args);

    printS(string);
}

static WORD colors[] = {
    FOREGROUND_RED,
    FOREGROUND_GREEN,
    FOREGROUND_BLUE,
    FOREGROUND_GREEN | FOREGROUND_BLUE,
    FOREGROUND_GREEN | FOREGROUND_RED,
    FOREGROUND_RED   | FOREGROUND_BLUE,
    FOREGROUND_RED   | FOREGROUND_INTENSITY,
    FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
    FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    FOREGROUND_GREEN | FOREGROUND_RED  | FOREGROUND_INTENSITY,
    FOREGROUND_RED   | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    FOREGROUND_RED,
    FOREGROUND_GREEN,
    FOREGROUND_BLUE,
    FOREGROUND_GREEN | FOREGROUND_BLUE,
};
static cstr hex = "0123456789ABCDEF";

static char printRowBuff1[] = "?? | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! |EOB\n";
static char printRowBuff[] = "?? | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! | ERR OR ER ROR! |\n";

void printRow() {
    if (song.row < 10) {
        printRowBuff[0] = '0';
        printRowBuff[1] = song.row + '0';
    }
    else {
        printRowBuff[0] = (song.row / 10) + '0';
        printRowBuff[1] = (song.row % 10) + '0';
    }

    for (uint8 i = 0; i < song.channelCount; i++) {
        cstr base = printRowBuff + 4 + (17 * i); //Buffer + "XX |"Offset + (ChannelWidth * CurrentChannel)
        Note note = GETNOTE(i);

        //Note
        cstr noteName = noteNames[note.note];
        base[1] = noteName[0];
        base[2] = noteName[1];
        base[3] = noteName[2];

        //Sample
        if (note.sample == 0) {
            base[5] = ' ';
            base[6] = ' ';
        }
        else if (note.sample < 10) {
            base[5] = ' ';
            base[6] = note.sample + '0';
        }
        else {
            base[5] = (note.sample / 10) + '0';
            base[6] = (note.sample % 10) + '0';
        }

        //Volume
        if (song.modType == MT_GENMOD && note.effect == 0xC) {
            note.vol    = note.effectArg + 1;
            note.effect = note.effectArg = 0;
        }
        if (!note.vol) {
            base[8] = ' ';
            base[9] = ' ';
        }
        else if (note.vol < 10) {
            base[8] = ' ';
            base[9] = note.vol + '0' - 1;
        }
        else {
            base[8] = ((note.vol - 1) / 10) + '0';
            base[9] = ((note.vol - 1) % 10) + '0';
        }

        //Effect
        if (note.effectArg != 0 || note.effect != 0) {
            base[11]  = hex[note.effect];
            base[12]  = '-';
            base[13] = hex[note.effectArg >> 4];
            base[14] = hex[note.effectArg & 15];
        }
        else {
            base[11]  = ' ';
            base[12]  = ' ';
            base[13] = ' ';
            base[14] = ' ';
        }
    }
    printRowBuff[4 + (17 * song.channelCount)] = '\n';
    WriteConsoleA(stdo, printRowBuff, 5 + (17 * song.channelCount), 0, 0);

    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(stdo, &info);
    COORD coord = { .X = 9, .Y = info.dwCursorPosition.Y - 1 };

    for (uint8 i = 0; i < song.channelCount; i++) {
        DWORD tmp;
        uint8 sample = GETNOTE(i).sample;
        FillConsoleOutputAttribute(stdo, colors[sample & 15], 2, coord, &tmp);
        coord.X += 3;
        FillConsoleOutputAttribute(stdo, FOREGROUND_BLUE | FOREGROUND_INTENSITY, 2, coord, &tmp);
        coord.X += 14;
    }
}

/*
void printSystem(cstr format, uint32 count, ...) {
    char string[1024];
    va_list args;
    int32 len = 0;

    va_start(args, count);
    len = vsprintf_s(string, 1024, format, args);
    va_end(args);

    system(string);
}
*/
//===