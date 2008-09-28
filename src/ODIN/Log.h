#pragma once

#include <stdio.h>
#include <windows.h>

#define LOG_SETFILENAME(filename) gLog.SetFileName(filename)
#define LOG_SETLOGLEVEL_NOTHING gLog.SetLogLevel(CLoglevelNothing)

#define LOG_SETLOGLEVEL_ERROR gLog.SetLogLevel(CLoglevelError)
#define LOG_SETLOGLEVEL_INFO gLog.SetLogLevel(CLoglevelInfo)
#define LOG_SETLOGLEVEL_DEVELOPERINFO gLog.SetLogLevel(CLoglevelDebug)

#define LOGINFO gLog.Lock();\
	gLog.SetLineNumber(__LINE__);\
	gLog.SetSourceFileName(__FILE__);\
	gLog.LogInfo

#define LOGERROR gLog.Lock();\
	gLog.SetLineNumber(__LINE__);\
	gLog.SetSourceFileName(__FILE__);\
	gLog.LogError

#define LOGDEBUG gLog.Lock();\
	gLog.SetLineNumber(__LINE__);\
	gLog.SetSourceFileName(__FILE__);\
	gLog.LogDebug

typedef enum TLogLevels
{
	CLoglevelNothing,
	CLoglevelError,
	CLoglevelInfo,
	CLoglevelDebug
}TLogLevels;

class CLog
{
protected:
	CRITICAL_SECTION fcs;
	FILE *fFile;
  static const int fFileNameLen = MAX_PATH;
	TCHAR fFileName[fFileNameLen];
	TLogLevels fLogLevel;
  static const int fSourceFileLen = MAX_PATH;
	char fSourceFile[fSourceFileLen];
	int fLineNumber;
	void LogNow(const TCHAR *LoglevelName, const TCHAR *LogString);
	const TCHAR* LevelName(TLogLevels level);
  void LogWithLevel(TLogLevels level, const TCHAR *format, va_list arglist);
  void ReplaceCRLF(TCHAR *s);
public:
	CLog();
	~CLog();
	void Lock() {EnterCriticalSection(&fcs);};
	void SetLineNumber(int line) {fLineNumber = line;};
	void SetSourceFileName(char *filename);
	bool SetFileName(TCHAR *FileName);
	void SetLogLevel(TLogLevels LogLevel);
	void LogError(const TCHAR *format, ...);
	void LogInfo(const TCHAR *format, ...);
	void LogDebug(const TCHAR *format, ...);
};

extern CLog gLog;










