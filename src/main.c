#include <Windows.h>
#include "utils.h"
#include "fileio.h"
#include "console.h"

void mmeInit();
void dsInit();
void dsPlay();

void fatal(bool val, cstr msg) {
    if (val) {
        printFormat("FATAL: %s\n", 1, msg);
        SetConsoleTitleA(msg);
        system("pause");
        exit(-1);
    }
}

void fatalCode(uint32 val, cstr msg) {
    if (val) {
        printFormat("FATAL: %s (Error: 0x%X)\n", 2, msg, val);
        SetConsoleTitleA(msg);
        system("pause");
        exit(-1);
    }
}

static HANDLE heap;
void* memAlloc(uint32 amount) { return HeapAlloc(heap, 0, amount); }
void  memFree (void *ptr)     { HeapFree(heap, 0, ptr); }

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    heap = GetProcessHeap();

    timeBeginPeriod(1);
    consoleInit();

    //=== External procedures
    //void printFormat(char *format, uint32 count, ...);
    //void printS(char *string);

    int i = 0, s = 0, k = 0, a[6] = {-3, 6, -1, -6, 8, 4};
    double sr = 0.0;
    const int aSize = sizeof(a) / sizeof(*a);

    printS("Table 1:\ni   ");
    for (int j = 0; j < aSize; j++) printFormat("%5i", 1, j + 1);
    printS("\na[i]");
    for (int j = 0; j < aSize; j++) printFormat("%5i", 1, a[j]);

    printS("\n\nTable 2:\n  s    i    k    sr\n");
    for (; i < aSize; i++) {
        printFormat("%3i  %3i  %3i  %3.2F\n", 4, s, i, k, sr);
        if (a[i] > 0) {
            k++;
            s += a[i];
        }
    }
    sr = (double)s / k;
    printFormat("%3i  %3i  %3i  %3.2F\n", 4, s, i, k, sr);

    dsInit();
    loadSongFile();
    dsPlay();
    
    Sleep(-1);
}
//printFormat("sr=%-5.2F\n", 1, sr);