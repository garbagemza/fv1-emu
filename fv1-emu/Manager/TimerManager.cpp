#include "stdafx.h"
#include "TimerManager.h"

Timer* TimerManager::createTimer(unsigned int samplesPerSecond)
{
	Timer* timer = new Timer();
	if (timer != NULL) {
		timer->samplesPerSecond = samplesPerSecond;
		timer->sample = 0;
		timer->t = 0.0;
	}
	return timer;
}

void TimerManager::updateTimerWithSampleNumber(Timer* timer, unsigned int sampleNumber)
{
	timer->sample = sampleNumber;
	timer->t = (double)sampleNumber / (double)timer->samplesPerSecond;
}