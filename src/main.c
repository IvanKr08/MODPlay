#include <Windows.h>
#include "utils.h"
#include "fileio.h"
#include "console.h"

void dsInit();
void dsPlay();

void warn(cstr msg) {
    printFormat("WARN: %s\n", 1, msg);
}

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

void *memAlloc(size_t amount) {
    void *m = malloc(amount);
    fatal(!m, "malloc() error. (Out of memory?)");
    return m;
}

void  memFree(void *ptr) {
    if (ptr) free(ptr);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    timeBeginPeriod(1);
    consoleInit();

    dsInit();
    loadSongFile();

    system("pause");
    dsPlay();
    
    Sleep(-1);
}