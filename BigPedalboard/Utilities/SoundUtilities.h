#pragma once
struct Timer;

enum SignalType
{
	Sinusoidal,
	Triangle,
	Square,
	SawTooth,
	Noise
};


struct SignalGenerator
{
	SignalType type;
	double frequency;

};

class SoundUtilities {
public:
	static SignalGenerator* createSignalGenerator(SignalType type, double f);
	static short* generateSamplesWithSignalGenerator(SignalGenerator*, unsigned int, unsigned int);
	static double sampleWithTime(SignalGenerator* generator, Timer* timer);

	static double sign(double value);
};