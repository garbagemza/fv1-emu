#pragma once

#include <ostream>
#include <sstream>

#define SCLogLevelDebug		1
#define SCLogLevelVerbose	2
#define SCLogLevelInfo		3
#define SCLogLevelWarning	4
#define SCLogLevelError		5

#define SCLogInfo(...) SCLog::mainInstance()->log(SCLogLevelInfo, __VA_ARGS__);
#define SCLogError(...) SCLog::mainInstance()->log(SCLogLevelError, __VA_ARGS__);
#define SCLogFunction() SCLogInfo(__FUNCTIONW__);


class SCLog
{
public:
	static SCLog* mainInstance();

	void setupLogging();
	void setupLoggingLevel(int logLevel);
	const char* rollingLogFile();

	
	void log(int severity, const wchar_t* format, ...);
	
private:
	// move to private eventually
	SCLog();
	const char* file;
	int logLevel;
	static SCLog* _instance;
};