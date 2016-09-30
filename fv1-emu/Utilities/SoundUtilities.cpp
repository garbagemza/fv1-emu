#include <Windows.h>
#include "SoundUtilities.h"
#include "..\Manager\TimerManager.h"

#define _USE_MATH_DEFINES
#include <math.h>

SignalGenerator* SoundUtilities::createSignalGenerator(SignalType type, double f)
{
	SignalGenerator* generator = new SignalGenerator();
	if (generator != NULL) {
		generator->type = type;
		generator->frequency = f;
	}

	return generator;
}

short* SoundUtilities::generateSamplesWithSignalGenerator(SignalGenerator* generator, unsigned int begin, unsigned int end)
{
	unsigned int dataSize = end - begin;
	short* data = new short[dataSize];
	if (data != NULL) {
		// Copy the wave data into the buffer.
		for (unsigned int sample = 0; sample < dataSize; sample++) {
			double t = (double)sample * 2.0 / (double)(44100 * 2);
			double h = 8000.0 * sin(2.0 * M_PI * generator->frequency * t);
			data[sample] = (short)h;
		}
		return data;
	}
	return NULL;
}

double SoundUtilities::sampleWithTime(SignalGenerator* generator, Timer* timer)
{
	double w = 2.0 * M_PI * generator->frequency;
	double period =  1.0 / generator->frequency;

	double sinf = sin(w * timer->t);

	switch (generator->type)
	{
	case Sinusoidal:
		return sinf;
	case Square:
		return sign(sinf);
	case Triangle:
	{
		double amplitude = (4.0 * 1.0) / period;
		double y = (abs(fmod(timer->t, period) - (period / 2.0))) - (period / 4.0);
		return amplitude * y;
	}
	case SawTooth:
	{
		double tp = timer->t / period;
		return 2.0 * (tp - floor((1.0 / 2.0) + tp));
	}
	case Noise:
	{
		// white noise
		return fmod(rand(), 2.0) - 0.5;
	}
	default:
		break;
	}
	return sinf;
}

double SoundUtilities::sign(double value)
{
	if (value < 0.0) {
		return -1.0;
	}
	else {
		return 1.0;
	}
}