#include "console.h"
#include "utils.h"
#include "mod.h"
#include "tables.h"

//===

static HANDLE stdi = 0, stdo = 0;

void initConsole() {
    AllocConsole();
    stdo = GetStdHandle(-11);
    stdi = GetStdHandle(-10);
}

void printI(int32 value) {
    char num[16];
    itoa(value, num, 10);
    printS(num);
    printC('\n');
}
void printC(char value) {
    WriteConsoleA(stdo, &value, 1, 0, 0);
}
void printS(cstr string) {
    WriteConsoleA(stdo, string, strlen(string), 0, 0);
}
void printFormat(cstr format, uint32 count, ...) {
    char string[1024];
    va_list args;
    int32 len = 0;
    
    va_start(args, count);
    len = vsprintf_s(string, 1024, format, args);
    va_end(args);

    printS(string);
}

void printRow() {
    char string[61];

    ITOA2_ZERO(song.row, string);

    string[2] = ' ';
    string[3] = '|';

    for (size_t i = 0; i < 4; i++) {
        cstr base = string + 4 + (14 * i);
        Note note = getNote(i);

        base[0] = ' ';
        if (note.period != 0) {
            for (int m = 0; m < 36; m++) {
                if (note.period == tuning0[m]) {
                    cstr name = noteNames[m + 1];
                    base[1] = name[0];
                    base[2] = name[1];
                    base[3] = name[2];
                    break;
                }
            }
        }
        else {
            base[1] = ' ';
            base[2] = ' ';
            base[3] = ' ';
        }
        base[4] = ' ';

        if (note.sample == 0) {
            base[5] = ' ';
            base[6] = ' ';
        }
        else {
            ITOA2(note.sample, base + 5);
        }

        base[7] = ' ';
        if (note.effectArg != 0 && note.effect != 0) {
            base[8] = DTOH(note.effect & 15);
            base[9] = '-';
            base[10] = DTOH(note.effectArg >> 4);
            base[11] = DTOH(note.effectArg & 15);
        }
        else {
            base[8] = ' ';
            base[9] = ' ';
            base[10] = ' ';
            base[11] = ' ';
        }
        base[12] = ' ';
        base[13] = '|';
    }
    string[60] = '\n';
    WriteConsoleA(stdo, string, 61, 0, 0);



    DWORD tmp;
    COORD coord;
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(stdo, &info);
    coord.Y = info.dwCursorPosition.Y - 1;
    coord.X = 9;

    for (size_t i = 0; i < 4; i++) {
        //uint32 c = ((getNote(i).sample % 10) + 1);
        FillConsoleOutputAttribute(stdo, getNote(i).sample % 8, 1, coord, &tmp);
        //FillConsoleOutputAttribute(stdo, c ^ FOREGROUND_INTENSITY, 1, coord, &tmp);
        coord.X++;
        FillConsoleOutputAttribute(stdo, (getNote(i).sample % 8) | FOREGROUND_INTENSITY, 1, coord, &tmp);
        //FillConsoleOutputAttribute(stdo, c, 1, coord, &tmp);
        coord.X += 13;
    }

    return;
    //Colorize
    //DWORD tmp;
    //COORD coord;
    //CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(stdo, &info);
    coord.Y = info.dwCursorPosition.Y - 1;
    coord.X = 0;

    //FillConsoleOutputAttribute(stdo, FOREGROUND_INTENSITY, 2, coord, &tmp);
    coord.X = 5;
    
    for (size_t i = 0; i < 4; i++) {
        FillConsoleOutputAttribute(stdo, FOREGROUND_BLUE | FOREGROUND_GREEN, 3, coord, &tmp);
        coord.X += 4;
        FillConsoleOutputAttribute(stdo, FOREGROUND_GREEN, 2, coord, &tmp);
        coord.X += 3;
        FillConsoleOutputAttribute(stdo, FOREGROUND_RED, 4, coord, &tmp);
        coord.X += 7;
    }
}