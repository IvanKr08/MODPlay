#include <Windows.h>
#include <stdarg.h>
#include "console.h"
//===

static HANDLE stdi = 0, stdo = 0;

void initConsole() {
    AllocConsole();
    stdo = GetStdHandle((DWORD)-11);
    stdi = GetStdHandle((DWORD)-10);
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

static char printRowBuff[] = "?? | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! | ERR OR !!!! |\n";
void printRow() {
    if (song.row < 10) {
        printRowBuff[0] = '0';
        printRowBuff[1] = song.row + '0';
    }
    else {
        printRowBuff[0] = (song.row / 10) + '0';
        printRowBuff[1] = (song.row % 10) + '0';
    }

    for (uint8 i = 0; i < 4; i++) {
        cstr base = printRowBuff + 4 + (14 * i); //Buffer + "XX |"Offset + (ChannelWidth * CurrentChannel)
        Note note = getNote(i);

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

        //Effect
        if (note.effectArg != 0 || note.effect != 0) {
            base[8]  = hex[note.effect];
            base[9]  = '-';
            base[10] = hex[note.effectArg >> 4];
            base[11] = hex[note.effectArg & 15];
        }
        else {
            base[8]  = ' ';
            base[9]  = ' ';
            base[10] = ' ';
            base[11] = ' ';
        }
    }
    WriteConsoleA(stdo, printRowBuff, 61, 0, 0);

    //return;
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(stdo, &info);
    COORD coord = { .X = 9, .Y = info.dwCursorPosition.Y - 1 };

    for (uint8 i = 0; i < 4; i++) {
        DWORD tmp;
        uint8 sample = getNote(i).sample;
        FillConsoleOutputAttribute(stdo, colors[sample & 15], 1, coord, &tmp);
        coord.X++;
        FillConsoleOutputAttribute(stdo, colors[sample & 15], 1, coord, &tmp);
        coord.X += 13;
    }
}