///////////////////////////////////////////////////////////////////////////////
// Filename: soundclass.cpp
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "soundclass.h"


void CALLBACK TimerProcess(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	SoundClass* pDDS = (SoundClass *)dwUser;
	pDDS->TimerCallback();
}

//Use the class constructor to initialize the private member variables that are used inside the sound class.

SoundClass::SoundClass()
{
	m_lpDS = 0;
	m_primaryBuffer = 0;
	m_lpDSB = 0;
	m_SoundDelegate = 0;
	m_pHEvent[0] = CreateEvent(NULL, FALSE, FALSE, _T("Direct_Sound_Buffer_Notify_0"));
	m_pHEvent[1] = CreateEvent(NULL, FALSE, FALSE, _T("Direct_Sound_Buffer_Notify_1"));

}


SoundClass::SoundClass(const SoundClass& other)
{
}


SoundClass::~SoundClass()
{
}


bool SoundClass::Initialize(HWND hwnd)
{
	bool result;

	//First initialize the DirectSound API as well as the primary buffer.Once that is initialized then the LoadWaveFile function can be called which will load in the.wav audio file and initialize the secondary buffer with the audio information from the.wav file.After loading is complete then PlayWaveFile is called which then plays the.wav file once.

	// Initialize direct sound and the primary sound buffer.
	result = InitializeDirectSound(hwnd);
	if (!result)
	{
		return false;
	}

	// Load a wave audio file onto a secondary buffer.
	result = LoadWaveFile("C:/Users/Mauro/Desktop/sound01.wav", &m_lpDSB);
	if (!result)
	{
		return false;
	}

	// Play the wave file now that it has been loaded.
	result = PlayWaveFile();
	if (!result)
	{
		return false;
	}

	return true;
}

//The Shutdown function first releases the secondary buffer which held the.wav file audio data using the ShutdownWaveFile function.Once that completes this function then called ShutdownDirectSound which releases the primary buffer and the DirectSound interface.

void SoundClass::Shutdown()
{
	// Release the secondary buffer.
	ShutdownWaveFile(&m_lpDSB);

	// Shutdown the Direct Sound API.
	ShutdownDirectSound();

	return;
}

//InitializeDirectSound handles getting an interface pointer to DirectSound and the default primary sound buffer.Note that you can query the system for all the sound devices and then grab the pointer to the primary sound buffer for a specific device, however I've kept this tutorial simple and just grabbed the pointer to the primary buffer of the default sound device.

bool SoundClass::InitializeDirectSound(HWND hwnd)
{
	HRESULT result;
	DSBUFFERDESC bufferDesc;
	WAVEFORMATEX waveFormat;


	// Initialize the direct sound interface pointer for the default sound device.
	result = DirectSoundCreate8(NULL, &m_lpDS, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Set the cooperative level to priority so the format of the primary sound buffer can be modified.
	result = m_lpDS->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
	if (FAILED(result))
	{
		return false;
	}

	//We have to setup the description of how we want to access the primary buffer.The dwFlags are the important part of this structure.In this case we just want to setup a primary buffer description with the capability of adjusting its volume.There are other capabilities you can grab but we are keeping it simple for now.

	// Setup the primary buffer description.
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	bufferDesc.dwBufferBytes = 0;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = NULL;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// Get control of the primary sound buffer on the default sound device.
	result = m_lpDS->CreateSoundBuffer(&bufferDesc, &m_primaryBuffer, NULL);
	if (FAILED(result))
	{
		return false;
	}

	//Now that we have control of the primary buffer on the default sound device we want to change its format to our desired audio file format.Here I have decided we want high quality sound so we will set it to uncompressed CD audio quality.

	// Setup the format of the primary sound bufffer.
	// In this case it is a .WAV file recorded at 44,100 samples per second in 16-bit stereo (cd audio format).
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 1;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// Set the primary buffer to be the wave format specified.
	result = m_primaryBuffer->SetFormat(&waveFormat);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

//The ShutdownDirectSound function handles releasing the primary buffer and DirectSound interfaces.

void SoundClass::ShutdownDirectSound()
{
	// Release the primary sound buffer pointer.
	if (m_primaryBuffer)
	{
		m_primaryBuffer->Release();
		m_primaryBuffer = 0;
	}

	// Release the direct sound interface pointer.
	if (m_lpDS)
	{
		m_lpDS->Release();
		m_lpDS = 0;
	}

	return;
}

//void SoundClass::SetCallback(LPGETAUDIOSAMPLES_PROGRESS Function_Callback, LPVOID lpData)
//{
//	m_lpGETAUDIOSAMPLES = Function_Callback;
//	m_lpData = lpData;
//}

void SoundClass::SetDelegate(ISoundDelegate* delegate)
{
	m_SoundDelegate = delegate;
}

//initializes main waveformat
bool SoundClass::CreateSoundBuffer()
{
	DSBUFFERDESC bufferDesc;
	HRESULT result;
	IDirectSoundBuffer* tempBuffer;

	// Set the wave format of secondary buffer that this wave file will be loaded onto.
	m_WFE.wFormatTag = WAVE_FORMAT_PCM;
	m_WFE.nSamplesPerSec = 44100;
	m_WFE.wBitsPerSample = 16;
	m_WFE.nChannels = 1;
	m_WFE.nBlockAlign = (m_WFE.wBitsPerSample / 8) * m_WFE.nChannels;
	m_WFE.nAvgBytesPerSec = m_WFE.nSamplesPerSec * m_WFE.nBlockAlign;
	m_WFE.cbSize = 0;

	// Set the buffer description of the secondary sound buffer that the wave file will be loaded onto.
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS;
	bufferDesc.dwBufferBytes = m_WFE.nSamplesPerSec * 2; // 2 seconds
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &m_WFE;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// Create a temporary sound buffer with the specific buffer settings.
	result = m_lpDS->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Test the buffer format against the direct sound 8 interface and create the secondary buffer.
	result = tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&m_lpDSB);
	if (FAILED(result))
	{
		return false;
	}
	// Release the temporary buffer.
	tempBuffer->Release();
	tempBuffer = 0;

	//Query DirectSoundNotify
	LPDIRECTSOUNDNOTIFY lpDSBNotify;
	result = m_lpDSB->QueryInterface(IID_IDirectSoundNotify8, (LPVOID *)&lpDSBNotify);
	if(FAILED(result)) {
		return false;
	}

	//Set Direct Sound Buffer Notify Position
	DSBPOSITIONNOTIFY pPosNotify[2];
	pPosNotify[0].dwOffset = 44100 / 2 - 1;
	pPosNotify[1].dwOffset = 3 * 44100 / 2 - 1;
	pPosNotify[0].hEventNotify = m_pHEvent[0];
	pPosNotify[1].hEventNotify = m_pHEvent[1];

	result = lpDSBNotify->SetNotificationPositions(2, pPosNotify);
	if (FAILED(result)) {
		return false;
	}

	//New audio buffer
	if (NULL != m_lpAudioBuf) {
		delete[]m_lpAudioBuf;
		m_lpAudioBuf = NULL;
	}
	m_lpAudioBuf = new BYTE[m_WFE.nAvgBytesPerSec];

	//Init Audio Buffer
	memset(m_lpAudioBuf, 0, m_WFE.nAvgBytesPerSec);

	return true;
}

bool SoundClass::Play()
{
	HRESULT result;

	// Set position at the beginning of the sound buffer.
	result = m_lpDSB->SetCurrentPosition(0);
	if (FAILED(result))
	{
		return false;
	}

	if (m_SoundDelegate != NULL) {
		m_SoundDelegate->willBeginPlay();
	}

	if (0 == m_dwCircles1) {

		//Get audio data by callback function
		DWORD dwRetSamples = 0, dwRetBytes = 0;
		
		if (m_SoundDelegate != NULL) {
			m_SoundDelegate->getAudioChunk(m_lpAudioBuf, m_WFE.nSamplesPerSec, dwRetSamples); // 1 second
		}

		//m_lpGETAUDIOSAMPLES(m_lpAudioBuf, m_WFE.nSamplesPerSec, dwRetSamples, m_lpData); // 1 second
		dwRetBytes = dwRetSamples * m_WFE.nBlockAlign;

		//Write the audio data to DirectSoundBuffer
		LPVOID lpvAudio1 = NULL, lpvAudio2 = NULL;
		DWORD dwBytesAudio1 = 0, dwBytesAudio2 = 0;

		//Lock DirectSoundBuffer
		HRESULT hr = m_lpDSB->Lock(0, m_WFE.nAvgBytesPerSec, &lpvAudio1, &dwBytesAudio1, &lpvAudio2, &dwBytesAudio2, 0);
		if (FAILED(hr)) {
			return false;
		}

		//Init lpvAudio1
		if (NULL != lpvAudio1) {
			memset(lpvAudio1, 0, dwBytesAudio1);
		}

		//Init lpvAudio2
		if (NULL != lpvAudio2) {
			memset(lpvAudio2, 0, dwBytesAudio2);
		}

		//Copy Audio Buffer to DirectSoundBuffer
		if (NULL == lpvAudio2) {
			memcpy(lpvAudio1, m_lpAudioBuf, dwRetBytes);
		}
		else {
			memcpy(lpvAudio1, m_lpAudioBuf, dwBytesAudio1);
			memcpy(lpvAudio2, m_lpAudioBuf + dwBytesAudio1, dwBytesAudio2);
		}

		//Unlock DirectSoundBuffer
		m_lpDSB->Unlock(lpvAudio1, dwBytesAudio1, lpvAudio2, dwBytesAudio2);
	}


	// Set volume of the buffer to 100%.
	result = m_lpDSB->SetVolume(DSBVOLUME_MAX);
	if (FAILED(result))
	{
		return false;
	}

	// Play the contents of the secondary sound buffer.
	result = m_lpDSB->Play(0, 0, DSBPLAY_LOOPING);
	if (FAILED(result))
	{
		return false;
	}

	//timeSetEvent
	m_timerID = timeSetEvent(200, 100, TimerProcess, (DWORD)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);

	return true;
}

bool SoundClass::Stop()
{
	if (NULL != m_lpDSB) {

		m_lpDSB->Stop();
		timeKillEvent(m_timerID);

		//Empty the buffer
		LPVOID lpvAudio1 = NULL;
		DWORD dwBytesAudio1 = 0;
		HRESULT hr = m_lpDSB->Lock(0, 0, &lpvAudio1, &dwBytesAudio1, NULL, NULL, DSBLOCK_ENTIREBUFFER);
		if (FAILED(hr)) {
			return false;
		}
		memset(lpvAudio1, 0, dwBytesAudio1);
		m_lpDSB->Unlock(lpvAudio1, dwBytesAudio1, NULL, NULL);

		//Move the current play position to begin
		m_lpDSB->SetCurrentPosition(0);

		//Reset Event
		ResetEvent(m_pHEvent[0]);
		ResetEvent(m_pHEvent[1]);

		//Set Circles1 and Circles2 0
		m_dwCircles1 = 0;
		m_dwCircles2 = 0;
	}
}


//The LoadWaveFile function is what handles loading in a.wav audio file and then copies the data onto a new secondary buffer.If you are looking to do different formats you would replace this function or write a similar one.

bool SoundClass::LoadWaveFile(char* filename, IDirectSoundBuffer8** secondaryBuffer)
{
	int error;
	FILE* filePtr;
	unsigned int count;
	WaveHeaderType waveFileHeader;
	WAVEFORMATEX waveFormat;
	DSBUFFERDESC bufferDesc;
	HRESULT result;
	IDirectSoundBuffer* tempBuffer;
	unsigned char* waveData;
	unsigned char *bufferPtr;
	unsigned long bufferSize;

	//To start first open the.wav file and read in the header of the file.The header will contain all the information about the audio file so we can use that to create a secondary buffer to accommodate the audio data.The audio file header also tells us where the data begins and how big it is.You will notice I check for all the needed tags to ensure the audio file is not corrupt and is the proper wave file format containing RIFF, WAVE, fmt, data, and WAVE_FORMAT_PCM tags.I also do a couple other checks to ensure it is a 44.1KHz stereo 16bit audio file.If it is mono, 22.1 KHZ, 8bit, or anything else then it will fail ensuring we are only loading the exact format we want.

		// Open the wave file in binary.
		error = fopen_s(&filePtr, filename, "rb");
	if (error != 0)
	{
		return false;
	}

	// Read in the wave file header.
	count = fread(&waveFileHeader, sizeof(waveFileHeader), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Check that the chunk ID is the RIFF format.
	if ((waveFileHeader.chunkId[0] != 'R') || (waveFileHeader.chunkId[1] != 'I') ||
		(waveFileHeader.chunkId[2] != 'F') || (waveFileHeader.chunkId[3] != 'F'))
	{
		return false;
	}

	// Check that the file format is the WAVE format.
	if ((waveFileHeader.format[0] != 'W') || (waveFileHeader.format[1] != 'A') ||
		(waveFileHeader.format[2] != 'V') || (waveFileHeader.format[3] != 'E'))
	{
		return false;
	}

	// Check that the sub chunk ID is the fmt format.
	if ((waveFileHeader.subChunkId[0] != 'f') || (waveFileHeader.subChunkId[1] != 'm') ||
		(waveFileHeader.subChunkId[2] != 't') || (waveFileHeader.subChunkId[3] != ' '))
	{
		return false;
	}

	// Check that the audio format is WAVE_FORMAT_PCM.
	if (waveFileHeader.audioFormat != WAVE_FORMAT_PCM)
	{
		return false;
	}

	// Check that the wave file was recorded in stereo format.
	if (waveFileHeader.numChannels != 2)
	{
		return false;
	}

	// Check that the wave file was recorded at a sample rate of 44.1 KHz.
	if (waveFileHeader.sampleRate != 44100)
	{
		return false;
	}

	// Ensure that the wave file was recorded in 16 bit format.
	if (waveFileHeader.bitsPerSample != 16)
	{
		return false;
	}

	// Check for the data chunk header.
	if ((waveFileHeader.dataChunkId[0] != 'd') || (waveFileHeader.dataChunkId[1] != 'a') ||
		(waveFileHeader.dataChunkId[2] != 't') || (waveFileHeader.dataChunkId[3] != 'a'))
	{
		return false;
	}

	//Now that the wave header file has been verified we can setup the secondary buffer we will load the audio data onto.We have to first set the wave format and buffer description of the secondary buffer similar to how we did for the primary buffer.There are some changes though since this is secondary and not primary in terms of the dwFlags and dwBufferBytes.

	// Set the wave format of secondary buffer that this wave file will be loaded onto.
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 2;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// Set the buffer description of the secondary sound buffer that the wave file will be loaded onto.
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &waveFormat;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	//Now the way to create a secondary buffer is fairly strange.First step is that you create a temporary IDirectSoundBuffer with the sound buffer description you setup for the secondary buffer.If this succeeds then you can use that temporary buffer to create a IDirectSoundBuffer8 secondary buffer by calling QueryInterface with the IID_IDirectSoundBuffer8 parameter.If this succeeds then you can release the temporary buffer and the secondary buffer is ready for use.

	// Create a temporary sound buffer with the specific buffer settings.
	result = m_lpDS->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Test the buffer format against the direct sound 8 interface and create the secondary buffer.
	result = tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&*secondaryBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the temporary buffer.
	tempBuffer->Release();
	tempBuffer = 0;

	//Now that the secondary buffer is ready we can load in the wave data from the audio file.First I load it into a memory buffer so I can check and modify the data if I need to.Once the data is in memory you then lock the secondary buffer, copy the data to it using a memcpy, and then unlock it.This secondary buffer is now ready for use.Note that locking the secondary buffer can actually take in two pointers and two positions to write to.This is because it is a circular buffer and if you start by writing to the middle of it you will need the size of the buffer from that point so that you don't write outside the bounds of it. This is useful for streaming audio and such. In this tutorial we create a buffer that is the same size as the audio file and write from the beginning to make things simple.

		// Move to the beginning of the wave data which starts at the end of the data chunk header.
		fseek(filePtr, sizeof(WaveHeaderType), SEEK_SET);

	// Create a temporary buffer to hold the wave file data.
	waveData = new unsigned char[waveFileHeader.dataSize];
	if (!waveData)
	{
		return false;
	}

	// Read in the wave file data into the newly created buffer.
	count = fread(waveData, 1, waveFileHeader.dataSize, filePtr);
	if (count != waveFileHeader.dataSize)
	{
		return false;
	}

	// Close the file once done reading.
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	// Lock the secondary buffer to write wave data into it.
	result = (*secondaryBuffer)->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0);
	if (FAILED(result))
	{
		return false;
	}

	// Copy the wave data into the buffer.
	memcpy(bufferPtr, waveData, waveFileHeader.dataSize);

	// Unlock the secondary buffer after the data has been written to it.
	result = (*secondaryBuffer)->Unlock((void*)bufferPtr, bufferSize, NULL, 0);
	if (FAILED(result))
	{
		return false;
	}

	// Release the wave data since it was copied into the secondary buffer.
	delete[] waveData;
	waveData = 0;

	return true;
}

//ShutdownWaveFile just does a release of the secondary buffer.

void SoundClass::ShutdownWaveFile(IDirectSoundBuffer8** secondaryBuffer)
{
	// Release the secondary sound buffer.
	if (*secondaryBuffer)
	{
		(*secondaryBuffer)->Release();
		*secondaryBuffer = 0;
	}

	return;
}

//The PlayWaveFile function will play the audio file stored in the secondary buffer.The moment you use the Play function it will automatically mix the audio into the primary buffer and start it playing if it wasn't already. Also note that we set the position to start playing at the beginning of the secondary sound buffer otherwise it will continue from where it last stopped playing. And since we set the capabilities of the buffer to allow us to control the sound we set the volume to maximum here.

bool SoundClass::PlayWaveFile()
{
	HRESULT result;


	// Set position at the beginning of the sound buffer.
	result = m_lpDSB->SetCurrentPosition(0);
	if (FAILED(result))
	{
		return false;
	}

	// Set volume of the buffer to 100%.
	result = m_lpDSB->SetVolume(DSBVOLUME_MAX);
	if (FAILED(result))
	{
		return false;
	}

	// Play the contents of the secondary sound buffer.
	result = m_lpDSB->Play(0, 0, 0);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

void SoundClass::TimerCallback()
{
	LPVOID lpvAudio1 = NULL, lpvAudio2 = NULL;
	DWORD dwBytesAudio1 = 0, dwBytesAudio2 = 0;
	DWORD dwRetSamples = 0, dwRetBytes = 0;

	HRESULT hr = WaitForMultipleObjects(2, m_pHEvent, FALSE, 0);
	if (WAIT_OBJECT_0 == hr) {

		m_dwCircles1++;

		//Lock DirectSoundBuffer Second Part
		HRESULT hr = m_lpDSB->Lock(m_WFE.nAvgBytesPerSec, m_WFE.nAvgBytesPerSec, &lpvAudio1, &dwBytesAudio1, &lpvAudio2, &dwBytesAudio2, 0);
		if (FAILED(hr)) {
			return;
		}
	}
	else if (WAIT_OBJECT_0 + 1 == hr) {

		m_dwCircles2++;

		//Lock DirectSoundBuffer First Part
		HRESULT hr = m_lpDSB->Lock(0, m_WFE.nAvgBytesPerSec, &lpvAudio1, &dwBytesAudio1, &lpvAudio2, &dwBytesAudio2, 0);
		if (FAILED(hr)) {
			return;
		}
	}
	else {
		return;
	}

	//Get 1 Second Audio Buffer 
	if (m_SoundDelegate != NULL) {
		m_SoundDelegate->getAudioChunk(m_lpAudioBuf, m_WFE.nSamplesPerSec, dwRetSamples);
	}
	dwRetBytes = dwRetSamples * m_WFE.nBlockAlign;

	//If near the end of the audio data
	if (dwRetSamples < m_WFE.nSamplesPerSec) {
		DWORD dwRetBytes = dwRetSamples*m_WFE.nBlockAlign;
		memset(m_lpAudioBuf + dwRetBytes, 0, m_WFE.nAvgBytesPerSec - dwRetBytes);
	}

	//Copy AudioBuffer to DirectSoundBuffer
	if (NULL == lpvAudio2) {
		memcpy(lpvAudio1, m_lpAudioBuf, dwBytesAudio1);
	}
	else {
		memcpy(lpvAudio1, m_lpAudioBuf, dwBytesAudio1);
		memcpy(lpvAudio2, m_lpAudioBuf + dwBytesAudio1, dwBytesAudio2);
	}

	//Unlock DirectSoundBuffer
	m_lpDSB->Unlock(lpvAudio1, dwBytesAudio1, lpvAudio2, dwBytesAudio2);

}