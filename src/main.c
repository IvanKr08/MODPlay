#include "utils.h"
#include "mod.h"
#include "console.h"

static WAVEFORMATEX soundFormat;
static HWAVEOUT hWaveOut;
static WAVEHDR outputHeader1, outputHeader2;
static LRSample buff1[A_SPB], buff2[A_SPB];

void hangThread() {
    SuspendThread(GetCurrentThread());
}

void CALLBACK waveOutProc(HWAVEOUT hWO, UINT uMsg, uint32 dwInstance, uint32 param1, uint32 param2) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    WAVEHDR* hdr = param1;
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

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    //space_debris | occ_san_geen | fairlight | hymn_to_aurora | beyond_music | ELYSIUM | GSLINGER
    for (uint32 i = 0; i < A_SPB; i++) {
        buff1[i].l = 128;
        buff2[i].r = 128;
        buff1[i].r = 128;
        buff2[i].l = 128;
    }

    initConsole();
	//for (size_t i = 0; i < 16; i++)
	//{
	//	for (size_t j = 0; j < 7; j++)
	//	{
	//		for (size_t n = 0; n < 12; n++)
	//		{
	//			printFormat("%5i, ", 1, periods[(i * 84) + n + (j * 12)] >> 4);
	//		}
	//		printC('\n');
	//	}
	//	printC('\n');
	//}
    loadSong(L"C:\\ELYSIUM.mod");

    soundFormat.wFormatTag = WAVE_FORMAT_PCM;
    soundFormat.nChannels = 2;
    soundFormat.wBitsPerSample = 8;
    soundFormat.nSamplesPerSec = A_SR;
    soundFormat.nAvgBytesPerSec = A_SR * 2;
    soundFormat.nBlockAlign = 2;

    outputHeader1.dwBufferLength = A_BPB;
    outputHeader1.lpData = buff1;
    outputHeader1.dwUser = 1;

    outputHeader2.dwBufferLength = A_BPB;
    outputHeader2.lpData = buff2;
    outputHeader1.dwUser = 0;

    waveOutOpen(&hWaveOut, WAVE_MAPPER, &soundFormat, waveOutProc, 0, CALLBACK_FUNCTION);

    waveOutPrepareHeader(hWaveOut, &outputHeader1, sizeof(WAVEHDR));
    waveOutPrepareHeader(hWaveOut, &outputHeader2, sizeof(WAVEHDR));

    waveOutWrite(hWaveOut, &outputHeader1, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &outputHeader2, sizeof(WAVEHDR));
    
    hangThread();

    waveOutUnprepareHeader(hWaveOut, &outputHeader1, sizeof(WAVEHDR));
    waveOutUnprepareHeader(hWaveOut, &outputHeader2, sizeof(WAVEHDR));

    waveOutClose(hWaveOut);
}