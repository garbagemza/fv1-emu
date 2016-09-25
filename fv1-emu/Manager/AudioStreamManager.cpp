#include "AudioStreamManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctime>

/*
** Note that many of the older ISA sound cards on PCs do NOT support
** full duplex audio (simultaneous record and playback).
** And some only support full duplex at lower sample rates.
*/
#define SAMPLE_RATE         (44100)
#define PA_SAMPLE_TYPE      paFloat32
#define FRAMES_PER_BUFFER   (64)

typedef float SAMPLE;

int AudioStreamManager::numNoInputs = 0;
ISoundDelegate* AudioStreamManager::soundDelegate = NULL;
unsigned int AudioStreamManager::sampleNumber = 0;


/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
int AudioStreamManager::readStreamCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	SAMPLE *out = (SAMPLE*)outputBuffer;
	const SAMPLE *in = (const SAMPLE*)inputBuffer;
	unsigned int i;
	(void)timeInfo; /* Prevent unused variable warnings. */
	(void)statusFlags;
	(void)userData;

	if (inputBuffer == NULL)
	{
		for (i = 0; i< framesPerBuffer; i++)
		{
			*out++ = 0;  /* left - silent */
			*out++ = 0;  /* right - silent */
		}
		numNoInputs += 1;
	}
	else
	{
		//clock_t begin = clock();

		for (i = 0; i< framesPerBuffer; i++)
		{
			SAMPLE left = *in++;
			SAMPLE right = *in++;

			GetSampleResult result = soundDelegate->getSample(left, right, sampleNumber++);
			if (result == Abort) {
				return paAbort;
			}
			*out++ = left;
			*out++ = right;
		}

		//clock_t end = clock();
		//double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
		//printf("Elapsed time: %f seconds", elapsed_secs);

	}

	return paContinue;
}

/*******************************************************************/

int AudioStreamManager::StreamAudio(ISoundDelegate* delegate)
{
	PaStreamParameters inputParameters, outputParameters;
	PaError err;

	soundDelegate = delegate;

	err = Pa_Initialize();
	if (err != paNoError) goto error;

	inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
	if (inputParameters.device == paNoDevice) {
		fprintf(stderr, "Error: No default input device.\n");
		goto error;
	}
	inputParameters.channelCount = 2;       /* stereo input */
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	if (outputParameters.device == paNoDevice) {
		fprintf(stderr, "Error: No default output device.\n");
		goto error;
	}
	outputParameters.channelCount = 2;       /* stereo output */
	outputParameters.sampleFormat = PA_SAMPLE_TYPE;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	err = Pa_OpenStream(
		&audioStream,
		&inputParameters,
		&outputParameters,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		0, /* paClipOff, */  /* we won't output out of range samples so don't bother clipping them */
		readStreamCallback,
		NULL);
	if (err != paNoError) goto error;

	delegate->willBeginPlay();

	err = Pa_StartStream(audioStream);
	if (err != paNoError) goto error;	
	return 0;

error:
	Pa_Terminate();
	fprintf(stderr, "An error occured while using the portaudio stream\n");
	fprintf(stderr, "Error number: %d\n", err);
	fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
	delegate->didEndPlay();
	return -1;
}

int AudioStreamManager::StopAudio() {

	PaError err;

	err = Pa_CloseStream(audioStream);
	if (err != paNoError) goto error;
	soundDelegate->didEndPlay();

	printf("Finished. gNumNoInputs = %d\n", numNoInputs);
	Pa_Terminate();
	return 0;

error:
	Pa_Terminate();
	fprintf(stderr, "An error occured while using the portaudio stream\n");
	fprintf(stderr, "Error number: %d\n", err);
	fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
	soundDelegate->didEndPlay();
	return -1;
}
