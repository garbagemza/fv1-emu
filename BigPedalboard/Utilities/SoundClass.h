///////////////////////////////////////////////////////////////////////////////
// Filename: soundclass.h
///////////////////////////////////////////////////////////////////////////////
#ifndef _SOUNDCLASS_H_
#define _SOUNDCLASS_H_

//The following libraries and headers are required for DirectSound to compile properly.

/////////////
// LINKING //
/////////////
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")


//////////////
// INCLUDES //
//////////////
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>

struct ISoundDelegate
{
public:
	virtual void willBeginPlay() = 0;
	virtual void getAudioChunk(LPVOID audioBuffer, DWORD sampleCount, DWORD &dwRetSamples) = 0;
	virtual void didEndPlay() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Class name: SoundClass
///////////////////////////////////////////////////////////////////////////////

class SoundClass
{
private:

	//The WaveHeaderType structure used here is for the.wav file format.When loading in.wav files I first read in the header to determine the required information for loading in the.wav audio data.If you are using a different format you will want to replace this header with the one required for your audio format.

	struct WaveHeaderType
	{
		char chunkId[4];
		unsigned long chunkSize;
		char format[4];
		char subChunkId[4];
		unsigned long subChunkSize;
		unsigned short audioFormat;
		unsigned short numChannels;
		unsigned long sampleRate;
		unsigned long bytesPerSecond;
		unsigned short blockAlign;
		unsigned short bitsPerSample;
		char dataChunkId[4];
		unsigned long dataSize;
	};


public:

	SoundClass();
	SoundClass(const SoundClass&);
	~SoundClass();

	bool Initialize(HWND);
	void Shutdown();

	void TimerCallback();

	//void SetCallback(LPGETAUDIOSAMPLES_PROGRESS Function_Callback, LPVOID lpData);
	void SetDelegate(ISoundDelegate* delegate);
	bool CreateSoundBuffer();
	bool Play();
	bool Stop();

private:
	bool InitializeDirectSound(HWND);
	void ShutdownDirectSound();

	bool LoadWaveFile(char*, IDirectSoundBuffer8**);

	void ShutdownWaveFile(IDirectSoundBuffer8**);

	bool PlayWaveFile();

	WAVEFORMATEX m_WFE;
	IDirectSoundBuffer* m_primaryBuffer;
	IDirectSound8* m_lpDS;
	IDirectSoundBuffer8* m_lpDSB;
	ISoundDelegate* m_SoundDelegate;

	HANDLE m_pHEvent[2];

	LPBYTE m_lpAudioBuf;
	LPVOID m_lpData;

	MMRESULT m_timerID;
	DWORD m_dwCircles1;
	DWORD m_dwCircles2;

};

#endif
