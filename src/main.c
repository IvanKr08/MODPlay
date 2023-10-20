#include "utils.h"
#include "mod.h"
#include "console.h"
#include <wchar.h>

static WAVEFORMATEX soundFormat;
static HWAVEOUT hWaveOut;
static WAVEHDR outputHeader1, outputHeader2;
static LRSample buff1[A_SPB], buff2[A_SPB];

void fatal(bool val, cstr msg) {
    if (!val) {
        printS("FATAL: ");
        printS(msg);
        printC('\n');
        system("pause");
        TerminateProcess(GetCurrentProcess(), -1);
    }
}

void* memAlloc(uint32 count) {
    return HeapAlloc(GetProcessHeap(), 0, count);
}

void CALLBACK waveOutProc(HWAVEOUT hWO, uint32 uMsg, uint32 dwInstance, uint32 param1, uint32 param2) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    WAVEHDR* hdr = (LPWAVEHDR)param1;
    if (uMsg == WOM_DONE) {
        static int outputBufferIndex;
        
        if (outputBufferIndex == 1) {
            outputBufferIndex = 2;
            waveOutWrite(hWO, &outputHeader2, sizeof(WAVEHDR));
            fillBuffer(buff1, hWO);
        }
        else {
            outputBufferIndex = 1;
            waveOutWrite(hWO, &outputHeader1, sizeof(WAVEHDR));
            fillBuffer(buff2, hWO);
        }
    }
}

void initMME() {
    memset(buff1, 0x80, A_SPB << 1);
    memset(buff2, 0x80, A_SPB << 1);

    soundFormat.wFormatTag = WAVE_FORMAT_PCM;
    soundFormat.nChannels = 2;
    soundFormat.wBitsPerSample = 8;
    soundFormat.nSamplesPerSec = A_SR;
    soundFormat.nAvgBytesPerSec = A_SR * 2;
    soundFormat.nBlockAlign = 2;

    outputHeader1.dwBufferLength = A_BPB;
    outputHeader1.lpData = (LPSTR)buff1;
    outputHeader1.dwUser = 1;

    outputHeader2.dwBufferLength = A_BPB;
    outputHeader2.lpData = (LPSTR)buff2;
    outputHeader1.dwUser = 0;

    waveOutOpen(&hWaveOut, WAVE_MAPPER, &soundFormat, (DWORD_PTR)waveOutProc, 0, CALLBACK_FUNCTION);

    waveOutPrepareHeader(hWaveOut, &outputHeader1, sizeof(WAVEHDR));
    waveOutPrepareHeader(hWaveOut, &outputHeader2, sizeof(WAVEHDR));

    waveOutWrite(hWaveOut, &outputHeader1, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &outputHeader2, sizeof(WAVEHDR));

    Sleep(-1);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    initConsole();
    
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    wstr songn;

    if (argc > 1)
        songn = argv[1];
    else
#ifdef _DEBUG
        //songn = L"D:\\Code\\VSProjects\\Demo01\\NewRelease\\occ_san_geen.mod";
        //songn = L"F:\\Music\\MODULE\\MOD\\space_debris.mod";
        //songn = L"F:\\Music\\MODULE\\qTest.mod";
        songn = L"F:\\Music\\MODULE\\MOD\\GSLINGER.MOD";
#else
        songn = 0;
#endif
    SetConsoleTitleW(songn);
    loadSong(songn);
    //system("pause");
    initMME();
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
