#define COBJMACROS

#include <Windows.h>
#include <initguid.h>
#include <dsound.h>
#include <mmdeviceapi.h>
#include "console.h"
#include "utils.h"
#include "mod.h"

#pragma region Utils
static void fillFormat(WAVEFORMATEX *sound) {
	sound->wFormatTag  		= WAVE_FORMAT_PCM;
	sound->nChannels	    = A_CHANNELS;
	sound->wBitsPerSample	= MUL8(A_SAMPLEDEPTH);
	sound->nSamplesPerSec   = A_SAMPLERATE;
	sound->nAvgBytesPerSec  = A_SAMPLERATE * A_SAMPLESIZE;
	sound->nBlockAlign	    = A_SAMPLESIZE;
	sound->cbSize			= 0;
}
#pragma endregion

#pragma region MME
static HWAVEOUT	mmeOut;
static WAVEHDR	mmeH1,			 mmeH2;
static LRSample	mmeBuff1[A_BUFFERSAMPLES], mmeBuff2[A_BUFFERSAMPLES];

static void CALLBACK mmeThreadProc(HWAVEOUT hWO, uint32 uMsg, uint32 dwInstance, uint32 param1, uint32 param2) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	//WAVEHDR* hdr = (LPWAVEHDR)param1;

	if (uMsg == WOM_DONE) {
		static int outputBufferIndex;

		if (outputBufferIndex == 1) {
			outputBufferIndex = 2;
			waveOutWrite(hWO, &mmeH2, sizeof(WAVEHDR));
			fillBuffer(mmeBuff1, A_BUFFERSAMPLES);
		}
		else {
			outputBufferIndex = 1;
			waveOutWrite(hWO, &mmeH1, sizeof(WAVEHDR));
			fillBuffer(mmeBuff2, A_BUFFERSAMPLES);
		}
	}
}

void mmeInit() {
	memset(mmeBuff1, 0x80, MUL2(A_BUFFERSAMPLES));
	memset(mmeBuff2, 0x80, MUL2(A_BUFFERSAMPLES));

	WAVEFORMATEX soundFormat;
	fillFormat(&soundFormat);

	mmeH1.dwBufferLength = A_BUFFERSIZE;
	mmeH1.lpData = (LPSTR)mmeBuff1;
	mmeH1.dwUser = 1;

	mmeH2.dwBufferLength = A_BUFFERSIZE;
	mmeH2.lpData = (LPSTR)mmeBuff2;
	mmeH1.dwUser = 0;

	waveOutOpen(&mmeOut, WAVE_MAPPER, &soundFormat, (DWORD_PTR)mmeThreadProc, 0, CALLBACK_FUNCTION);

	waveOutPrepareHeader(mmeOut, &mmeH1, sizeof(WAVEHDR));
	waveOutPrepareHeader(mmeOut, &mmeH2, sizeof(WAVEHDR));

	waveOutWrite(mmeOut, &mmeH1, sizeof(WAVEHDR));
	waveOutWrite(mmeOut, &mmeH2, sizeof(WAVEHDR));
}
#pragma endregion

#pragma region DirectSound
static IDirectSound	      *dsOut;
static IDirectSoundBuffer *dsPBBuffer, *dsSBBuffer;
static IDirectSoundNotify* dsNotify;
static DWORD			   dsBufferStep, dsNotifyCount = 10, dsThreadID;
static HANDLE			   dsThread, *dsNotifyHandles;
static DSBPOSITIONNOTIFY  *dsNotifyEvents;

static DWORD WINAPI dsThreadProc(LPVOID lpParam) {
	static DWORD pos = 0;
	DWORD  playPos, totalBytes, size1, size2;
	LPVOID buff1, buff2;

	while (1) {
		WaitForMultipleObjects(dsNotifyCount, dsNotifyHandles, 0, -1);

		//WTF!? Thanks to OpenMPT (Чтение чужих программ ГОРАЗДО эффективнее книг...)
		IDirectSoundBuffer_GetCurrentPosition(dsSBBuffer, &playPos, 0);
		totalBytes = ((playPos - pos + A_BUFFERSIZE) % A_BUFFERSIZE) & ~(A_SAMPLESIZE - 1);
		IDirectSoundBuffer_Lock(dsSBBuffer, pos, totalBytes, &buff1, &size1, &buff2, &size2, 0);
		
		if (size1) fillBuffer(buff1, size1 / A_SAMPLESIZE);
		if (size2) fillBuffer(buff2, size2 / A_SAMPLESIZE);
		IDirectSoundBuffer_Unlock(dsSBBuffer, buff1, size1, buff2, size2);

		pos = (pos + size1 + size2) % A_BUFFERSIZE;
	}
}

static void dsSetupNotify() {
	dsBufferStep = A_BUFFERSIZE / dsNotifyCount;
	ARRALLOC(dsNotifyEvents, dsNotifyCount);
	ARRALLOC(dsNotifyHandles, dsNotifyCount);

	for (uint32 i = 0, offset = 0; i < dsNotifyCount; i++) {
		offset += dsBufferStep;

		dsNotifyHandles[i] = CreateEventA(0, 0, 0, 0);

		dsNotifyEvents[i].hEventNotify = dsNotifyHandles[i];
		dsNotifyEvents[i].dwOffset = offset - 1;
	}
	dsNotifyEvents[dsNotifyCount - 1].dwOffset = A_BUFFERSIZE - 1;

	fatalCode(
		IDirectSoundNotify_SetNotificationPositions(dsNotify, dsNotifyCount, dsNotifyEvents),
		"Failed to set notification positions.");
}

void dsInit() {
	fatalCode(
		CoInitialize(0),
		"Failed to initialize COM.");
	

	fatalCode(
		DirectSoundCreate(0, &dsOut, 0),
		"Failed to create audio context.");

	fatalCode(
		IDirectSound_SetCooperativeLevel(dsOut, GetConsoleWindow(), DSSCL_PRIORITY),
		"Failed to set cooperative level.");

	DSBUFFERDESC dsPBDesc;
	memset(&dsPBDesc, 0, sizeof(dsPBDesc));
	dsPBDesc.dwSize		   = sizeof(dsPBDesc);
	dsPBDesc.dwFlags	   = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	fatalCode(
		IDirectSound_CreateSoundBuffer(dsOut, &dsPBDesc, &dsPBBuffer, 0),
		"Failed to create primary buffer.");

	WAVEFORMATEX soundFormat;
	fillFormat(&soundFormat);
	fatalCode(
		IDirectSoundBuffer_SetFormat(dsPBBuffer, &soundFormat),
		"Failed to set wave format.");

	fatalCode(
		IDirectSoundBuffer_Play(dsPBBuffer, 0, 0, DSBPLAY_LOOPING),
		"Failed to play primary buffer.");

	DSBUFFERDESC dsSBDesc;
	memset(&dsSBDesc, 0, sizeof(dsSBDesc));
	dsSBDesc.dwSize		   = sizeof(dsSBDesc);
	dsSBDesc.dwBufferBytes = A_BUFFERSIZE;
	dsSBDesc.lpwfxFormat   = &soundFormat;
	dsSBDesc.dwFlags       = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	fatalCode(
		IDirectSound_CreateSoundBuffer(dsOut, &dsSBDesc, &dsSBBuffer, 0),
		"Failed to create secondary buffer.");

	fatalCode(
		IDirectSoundBuffer_QueryInterface(dsSBBuffer, &IID_IDirectSoundNotify, (void**)&dsNotify),
		"Failed to query SoundNotify.");

	fatal(
		(dsThread = CreateThread(0, 0, dsThreadProc, 0, CREATE_SUSPENDED, &dsThreadID)) == INVALID_HANDLE_VALUE,
		"Failed to create sound thread.");

	dsSetupNotify();

	printFormat("Using: \t\tDirectSound\nSamplerate: \t%i\nChannels: \t%i\nSample depth: \t%i\nBuffer size: \t%i\n\n", 4, A_SAMPLERATE, A_CHANNELS, A_SAMPLEDEPTH * 8, A_BUFFERSIZE);
}

void dsPlay() {
	DWORD size;
	LPVOID buff;
	IDirectSoundBuffer_Lock(dsSBBuffer, 0, A_BUFFERSIZE, &buff, &size, 0, 0, DSBLOCK_ENTIREBUFFER);
	fillBuffer(buff, size / A_SAMPLESIZE);
	IDirectSoundBuffer_Unlock(dsSBBuffer, buff, size, 0, 0);
	IDirectSoundBuffer_Play(dsSBBuffer, 0, 0, DSBPLAY_LOOPING);

	ResumeThread(dsThread);
}
#pragma endregion