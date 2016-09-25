#pragma once
#include "portaudio.h"

enum GetSampleResult {
	Continue,
	Abort
};

struct ISoundDelegate
{
public:
	// called when the output device will begin playing data
	virtual void willBeginPlay() = 0;

	// called every sample read
	virtual GetSampleResult getSample(float& left, float& right, unsigned int sampleNumber) = 0;

	// called when the reproduction is aborted.
	virtual void didEndPlay() = 0;
};


class AudioStreamManager {
public:
	int StreamAudio(ISoundDelegate* delegate);
	int StopAudio();

private:
	static int readStreamCallback(const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData);

	static ISoundDelegate* soundDelegate;
	static unsigned int sampleNumber;
	static int numNoInputs;

	PaStream *audioStream = 0;
};
