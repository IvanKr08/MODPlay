#include <Windows.h>
#include <wchar.h>
#include "utils.h"
#include "mod.h"
#include "console.h"

/*
#include <winnt.h>
#ifndef NT_ERROR
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)
#endif

    NTSYSAPI
        NTSTATUS
        NTAPI
        NtSetTimerResolution(
            _In_ ULONG                DesiredResolution,
            _In_ BOOLEAN              SetResolution,
            _Out_ PULONG              CurrentResolution);

    NTSYSAPI
        NTSTATUS
        NTAPI
        NtQueryTimerResolution(
            _Out_ PULONG              MaximumResolution,
            _Out_ PULONG              MinimumResolution,
            _Out_ PULONG              CurrentResolution);

#pragma comment (lib, "ntdll.lib")
*/

void MMEInit();
void DSInit();
void DSPlay();

void fatal(bool val, cstr msg) {
    if (val) {
        printFormat("FATAL: %s\n", 1, msg);
        system("pause");
        TerminateProcess(GetCurrentProcess(), -1);
    }
}

void fatalCode(uint32 val, cstr msg) {
    if (val) {
        printFormat("FATAL: %s (Error: 0x%X)\n", 2, msg, val);
        system("pause");
        TerminateProcess(GetCurrentProcess(), -1);
    }
}

void* memAlloc(uint32 count) {
    return HeapAlloc(GetProcessHeap(), 0, count);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    initConsole();
    DSInit();
    timeBeginPeriod(1);
    ULONG curRes;

    int argc;
    wstr songn, *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc > 1)
        songn = argv[1];
    else
#ifdef _DEBUG
        //songn = L"D:\\Code\\VSProjects\\Demo01\\NewRelease\\occ_san_geen.mod";
        songn = L"F:\\Music\\MODULE\\MOD\\space_debris.mod";
        //songn = L"F:\\Music\\MODULE\\MOD\\hymn_to_aurora.mod";
        //songn = L"F:\\Music\\MODULE\\qTest.mod";
        //songn = L"F:\\Music\\MODULE\\MOD\\GSLINGER.MOD";
        //songn = L"F:\\Music\\MODULE\\MOD\\emax-doz.mod";
        //songn = L"F:\\Music\\MODULE\\MOD\\SHADOWRU.MOD";
        //songn = L"F:\\Music\\MODULE\\MOD\\fairlight.MOD";
#else
        songn = 0;
#endif

    SetConsoleTitleW(songn);
    loadSong(songn);
    DSPlay();
    Sleep(-1);
}















/*
hangThread();

waveOutUnprepareHeader(hWaveOut, &outputHeader1, sizeof(WAVEHDR));
waveOutUnprepareHeader(hWaveOut, &outputHeader2, sizeof(WAVEHDR));

waveOutClose(hWaveOut);
*/
/*
    void CenterText(HDC hDC, int x, int y, LPWSTR szFace, LPWSTR szMessage, int point)
    {
        HFONT hFont = CreateFontW(-point * GetDeviceCaps(hDC, LOGPIXELSY) / 72,
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
            PROOF_QUALITY, VARIABLE_PITCH, szFace);

        HGDIOBJ hOld = SelectObject(hDC, hFont);

        SetTextAlign(hDC, TA_CENTER | TA_BASELINE);

        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(0, 0, 0xFF));
        TextOutW(hDC, x, y, szMessage, lstrlenW(szMessage));

        SelectObject(hDC, hOld);
        DeleteObject(hFont);
    }

    WCHAR szMessage[] = L"123Hello, World";
    WCHAR szFace[] = L"123Times New Roman";

    uint8 arr[1920 * 1080];
    extern uint16 notes[];
    int _fltused = 0;
WriteConsoleW(GetStdHandle((DWORD)-11), argv[1], lstrlenW(argv[1]), &fuck, NULL);*/
/*
LARGE_INTEGER res;
QueryPerformanceFrequency(&res);

HDC hDC = GetDC(NULL);

CenterText(hDC, GetSystemMetrics(SM_CXSCREEN) / 2,
    GetSystemMetrics(SM_CYSCREEN) / 2,
    szFace, szMessage, 72);

ReleaseDC(NULL, hDC);
Sleep(5000);

initConsole();
while (1) {
    LARGE_INTEGER start;
    uint32 c = 0;
    QueryPerformanceCounter(&start);
    while (1) {
        LARGE_INTEGER n;
        QueryPerformanceCounter(&n);
        if (n.QuadPart > (start.QuadPart + res.QuadPart)) {
            printI(c);
            break;
        }
        if (!(c % 1000000)) {
            //printC('H');
        }
        arr[c % (1920 * 1080)] = (uint8)(c * 16 + (c & 564) - (c / 2) + (c ^ 468)) ^ (~c);
        c++;
    }
}
    goto lSkip;
for (size_t i = 0; i < 16; i++)
{
    for (size_t j = 0; j < 7; j++)
    {
        for (size_t n = 0; n < 12; n++)
        {
            printFormat("%4i, ", 1, notes[(i * 84) + n + (j * 12)] >> 4);
        }
        printC('\n');
    }
    printC('\n');
}
lSkip:    //space_debris | occ_san_geen | fairlight | hymn_to_aurora | beyond_music | ELYSIUM | GSLINGER | enigma
*/
//int *main() {

// /*(void (WINAPI*)(HWAVEOUT, UINT, uint32, uint32, uint32))*/
