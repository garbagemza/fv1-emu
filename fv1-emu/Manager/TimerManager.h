#pragma once

struct Timer
{
	double t;
	unsigned int sample;
	unsigned int samplesPerSecond;

};
class TimerManager 
{
public:
	static Timer* createTimer(unsigned int samplesPerSecond);
	static void updateTimerWithSampleNumber(Timer* timer, unsigned int sampleNumber);
	static void updateTimerWithTimer(Timer* timer, Timer* withTimer);
};