#include "stdafx.h"
#include "log.h"
#include <time.h>
#include <crtdbg.h>

#define LOG_STDOUT

CLog gLog;

CLog::CLog()
{
	fFileName[0] = _T('\0');
	fLogLevel = CLoglevelNothing;
	fFile = NULL;
	InitializeCriticalSection(&fcs);
}

CLog::~CLog()
{
	if (fFile != NULL)
	{
		fclose(fFile);
	}
	DeleteCriticalSection(&fcs);
}

bool CLog::SetFileName(TCHAR *FileName)
{
	if (fFile != NULL)
	{
		fclose(fFile);
	}
	//we wanna save the logfile the same place as this process is...
	TCHAR file[MAX_PATH];
	int r = GetModuleFileName(NULL, file, MAX_PATH);
	_ASSERTE(r != 0);
	if (r == 0)
	{
		return false;
	}
	//now we have the filename and path, let's remove the filename so we only have the path...
	TCHAR *mid = file + r;
	while (*(--mid) != _T('\\') && mid > file);
	if (mid <= file)
	{
		return false;
	}
	*(++mid) = '\0';
	_tcscpy_s(fFileName, fFileNameLen, file);
	_tcscat_s(fFileName, fFileNameLen, FileName);
	//open the logfile...
	errno_t res = _tfopen_s(&fFile, fFileName, _T("ab"));
	_ASSERTE(fFile != NULL);
	if (res)
	{
		return false;
	}

	return true;
}

void CLog::SetSourceFileName(char *filename)
{
	//strip the path from the filename...
	char *mid = filename + strlen(filename);
	while (mid > filename)
	{
		if (*(--mid) == _T('\\'))
		{
			mid++;
			break;
		}
	}
	//store the filename...
	strcpy_s(fSourceFile, fSourceFileLen, mid);
}

void CLog::SetLogLevel(TLogLevels LogLevel)
{
	fLogLevel = LogLevel;
}

void CLog::LogNow(const TCHAR *LoglevelName, const TCHAR *LogString)
{
	if (fFile == NULL)
	{
		_ASSERTE(!"Filename is not set...");
		return;
	}

	//get the current date and time, and format it to the format we wanna use...
	time_t now;
	time(&now);
	struct tm tmnow;
  errno_t res = localtime_s(&tmnow, &now);
	TCHAR strnow[25];
	_tcsftime(strnow, 24, _T("%Y-%m-%d %H:%M:%S"), &tmnow);

#ifdef _UNICODE
	if (fLogLevel == CLoglevelDebug)
	{
		fwprintf(fFile, _T("%s\t%s\t%S, %d\t%s\r\n"), strnow, LoglevelName, fSourceFile, fLineNumber, LogString);
	}
	else
	{
		fwprintf(fFile, _T("%s\t%s\t%s\r\n"), strnow, LoglevelName, LogString);
	}
#else
	if (fLogLevel == CLoglevelDebug)
	{
		fprintf(fFile, _T("%s\t%s\t%s, %d\t%s\r\n"), strnow, LoglevelName, fSourceFile, fLineNumber, LogString);
	}
	else
	{
		fprintf(fFile, _T("%s\t%s\t%s\r\n"), strnow, LoglevelName, LogString);
	}
#endif
#ifdef LOG_TRACE
	TCHAR mid[1025] = {0};
	wsprintf(mid, _T("%s\r\n"), LogString);
	OutputDebugString(mid);
#endif
#ifdef LOG_STDOUT
	TCHAR mid2[1025] = {0};
	wsprintf(mid2, _T("%s\r\n"), LogString);
	_tprintf(mid2);
#endif
}

void CLog::ReplaceCRLF(TCHAR *s)
{
	TCHAR *mid = s;
	while (*mid != _T('\0'))
	{
		switch (*mid)
		{
		case _T('\r'):
			*mid = _T('|');
			break;
		case _T('\n'):
			*mid = _T('|');
			break;
		}
		mid++;
	}
}

const TCHAR* CLog::LevelName(TLogLevels level)
{
  switch (level)
  {
    case CLoglevelNothing:
      return _T("");
    case CLoglevelError:
      return _T("Error");
    case CLoglevelInfo:
      return _T("Info");
    case CLoglevelDebug:
      return _T("Debug");
    default:
      return _T("Unknown");
  }
}

void CLog::LogWithLevel(TLogLevels level, const TCHAR *format, va_list arglist)
{
	if (fLogLevel <= level)
	{
		//never corrupt the last error value...
		DWORD LastError = GetLastError();
		//do the actual logging...
		TCHAR mid[1025] = {0}; //the wvsprintf function never puts more than 1024 bytes in a string...
		wvsprintf(mid, format, arglist);
		ReplaceCRLF(mid);
		LogNow(LevelName(level), mid);
		SetLastError(LastError);
	}
	LeaveCriticalSection(&fcs);
}

void CLog::LogError(const TCHAR *format, ...)
{
	va_list args;
	va_start(args, format);
  LogWithLevel(CLoglevelError, format, args);
	va_end(args);
}

void CLog::LogInfo(const TCHAR *format, ...)
{
	va_list args;
	va_start(args, format);
  LogWithLevel(CLoglevelInfo, format, args);
	va_end(args);
}

void CLog::LogDebug(const TCHAR *format, ...)
{
	va_list args;
	va_start(args, format);
  LogWithLevel(CLoglevelDebug, format, args);
	va_end(args);
}



















