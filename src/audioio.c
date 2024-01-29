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
static DWORD			   dsBufferStep, dsNotifyCount, dsThreadID;
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
	dsNotifyCount	= 10;
	dsBufferStep	= A_BUFFERSIZE / dsNotifyCount;
	dsNotifyEvents	= memAlloc(sizeof(DSBPOSITIONNOTIFY) * dsNotifyCount);
	dsNotifyHandles = memAlloc(sizeof(HANDLE) * dsNotifyCount);

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



//printC('\n');
//printI(pos);
//printI(bytes);
//printI(size1);
//printI(size2);
//printC('\n');

//continue;

//eventNum = 
//DWORD eventNum, size;
//LPVOID buff, buff1, buff2;
//IDirectSoundBuffer_Lock(dsSBBuffer, dsBufferStep * eventNum, dsBufferStep, &buff, &size, 0, 0, 0);
//if (eventNum == 7) {
//	printI(dsBufferStep * eventNum);
//	printI(dsBufferStep);
//	printI(dsNotifyEvents[eventNum].dwOffset);
//}
//fillBuffer((LRSample*)buff, size);
//IDirectSoundBuffer_Unlock(dsSBBuffer, buff, size, 0, 0);













//-----------------------------------------------------------
// Play an audio stream on the default audio rendering
// device. The PlayAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data to the
// rendering device. The inner loop runs every 1/2 second.
//-----------------------------------------------------------
/*
#define INITGUID
#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <float.h>
#include <initguid.h>
#include <Mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>
#include <winerror.h>

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

HRESULT PlayAudioStream(MyAudioSource* pMySource)
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioClient* pAudioClient = NULL;
	IAudioRenderClient* pRenderClient = NULL;
	WAVEFORMATEX* pwfx = NULL;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	UINT32 numFramesPadding;
	BYTE* pData;
	DWORD flags = 0;

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr);

	hr = CoCreateInstance(
		&CLSID_MMDeviceEnumerator, (*) NULL,
		CLSCTX_ALL, &IID_IMMDeviceEnumerator, (*)
		(void**)&deviceEnumerator
	);

		hr = pEnumerator->GetDefaultAudioEndpoint(
			eRender, eConsole, &pDevice);
		EXIT_ON_ERROR(hr);

		hr = pDevice->Activate(
			IID_IAudioClient, CLSCTX_ALL,
			NULL, (void**)&pAudioClient);
		EXIT_ON_ERROR(hr);

		hr = pAudioClient->GetMixFormat(&pwfx);
		EXIT_ON_ERROR(hr);

		hr = pAudioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			0,
			hnsRequestedDuration,
			0,
			pwfx,
			NULL);
		EXIT_ON_ERROR(hr);

		// Tell the audio source which format to use.
		hr = pMySource->SetFormat(pwfx);
		EXIT_ON_ERROR(hr);

		// Get the actual size of the allocated buffer.
		hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr)

		hr = pAudioClient->GetService(
			IID_IAudioRenderClient,
			(void**)&pRenderClient);
	EXIT_ON_ERROR(hr)

		// Grab the entire buffer for the initial fill operation.
		hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
	EXIT_ON_ERROR(hr)

		// Load the initial data into the shared buffer.
		hr = pMySource->LoadData(bufferFrameCount, pData, &flags);
	EXIT_ON_ERROR(hr)

		hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
	EXIT_ON_ERROR(hr)

		// Calculate the actual duration of the allocated buffer.
		hnsActualDuration = (double)REFTIMES_PER_SEC *
		bufferFrameCount / pwfx->nSamplesPerSec;

	hr = pAudioClient->Start();  // Start playing.
	EXIT_ON_ERROR(hr)

		// Each loop fills about half of the shared buffer.
		while (flags != AUDCLNT_BUFFERFLAGS_SILENT)
		{
			// Sleep for half the buffer duration.
			Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

			// See how much buffer space is available.
			hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
			EXIT_ON_ERROR(hr)

				numFramesAvailable = bufferFrameCount - numFramesPadding;

			// Grab all the available space in the shared buffer.
			hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
			EXIT_ON_ERROR(hr)

				// Get next 1/2-second of data from the audio source.
				hr = pMySource->LoadData(numFramesAvailable, pData, &flags);
			EXIT_ON_ERROR(hr)

				hr = pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
			EXIT_ON_ERROR(hr)
		}

	// Wait for last data in buffer to play before stopping.
	Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

	hr = pAudioClient->Stop();  // Stop playing.
	EXIT_ON_ERROR(hr)

		Exit:
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pRenderClient)

		return hr;
}*/