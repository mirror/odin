/******************************************************************************

    ODIN - Open Disk Imager in a Nutshell

    Copyright (C) 2008

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>

    For more information and the latest version of the source code see
    <http://sourceforge.net/projects/odin-win>

******************************************************************************/
 
#pragma once
#ifndef __THREAD_H__
#define __THREAD_H__

class CThread
{

public:
	CThread(DWORD dwCreationFlags = 0)
	{
    m_hThread = Create(ThreadProc, this, dwCreationFlags, NULL, 0, &m_dwThreadId);
	}

  CThread(HANDLE hThread, DWORD dwThreadId = 0)
		: m_hThread(hThread), m_dwThreadId(dwThreadId)
	{
		if ( m_hThread != NULL && m_dwThreadId == 0 )
			m_dwThreadId = ::GetThreadId(m_hThread);
	}

	virtual ~CThread()
	{
		if ( m_hThread != NULL )
			CloseHandle(m_hThread);
	}

	virtual DWORD Execute() = 0;

    DWORD GetThreadId() const
	{
		return m_dwThreadId;
	}

	HANDLE GetHandle() const
	{
		return m_hThread;
	}

	int GetPriority() const
	{
		ATLASSERT( m_hThread != NULL );
		return GetThreadPriority(m_hThread);
	}

	BOOL SetPriority(int nPriority)
	{
		ATLASSERT( m_hThread != NULL );
		return SetThreadPriority(m_hThread, nPriority);
	}

	DWORD GetExitCode() const
	{
		ATLASSERT( m_hThread != NULL );
		DWORD dwExitCode = 0;
		if ( GetExitCodeThread(m_hThread, &dwExitCode) )
			return dwExitCode;
		else
			return (DWORD) -1;
	}

	BOOL GetThreadTimes(LPFILETIME pCreationTime, LPFILETIME pExitTime, LPFILETIME pKernelTime,
		LPFILETIME pUserTime) const
	{
		ATLASSERT( m_hThread != NULL );
		return ::GetThreadTimes(m_hThread, pCreationTime, pExitTime, pKernelTime, pUserTime);
	}
	
	DWORD Resume()
	{
		ATLASSERT( m_hThread != NULL );
		return ResumeThread(m_hThread);
	}

	DWORD Suspend()
	{
		ATLASSERT( m_hThread != NULL );
		return SuspendThread(m_hThread);
	}

	BOOL Terminate(DWORD dwExitCode = 0)
	{
		ATLASSERT( m_hThread != NULL );
		return TerminateThread(m_hThread, dwExitCode);
	}

	void Exit(DWORD dwExitCode = 0)
	{
		// Make sure this is only called from the thread that this object represents
		ATLASSERT( m_dwThreadId == ::GetCurrentThreadId() );
		_endthreadex(dwExitCode);
	}


	DWORD WaitForThread(DWORD dwWaitMilliseconds = INFINITE) const
	{
		ATLASSERT( m_hThread != NULL );
    return WaitForSingleObject(m_hThread, dwWaitMilliseconds);
	}

  // Function to set the thread's name for debugging purposes.  This is done
  // by filling in a special structure and firing off an exception.  A
  // compatible debugger knows to look in exceptions of this type and look for
  // the thread's name.Strange but works
  // Note: This method must be called from inside the new thread, i.e. from the
  // Execute method

  void SetName(LPCSTR threadName)
  {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = -1;
    info.dwFlags = 0;

    __try {
      RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (const ULONG_PTR*)&info );
    }
    __except (EXCEPTION_CONTINUE_EXECUTION) {}
}

protected:
	HANDLE m_hThread;
	DWORD m_dwThreadId;

private:
	CThread(const CThread& otherThread)
  {} // do not do this

  typedef struct tagTHREADNAME_INFO {
    DWORD dwType;     // must be 0x1000
    LPCSTR szName;    // pointer to name (in user addr space)
    DWORD dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags;    // reserved for future use, must be zero
  } THREADNAME_INFO;

	static HANDLE Create(unsigned (__stdcall *pThreadProc )( void * ), LPVOID pParam = NULL,
		DWORD dwCreationFlags = 0, LPSECURITY_ATTRIBUTES pSecurityAttr = NULL,
		DWORD dwStackSize = 0, DWORD* pdwThreadId = NULL)
	{
		DWORD dwThreadId = 0;
		HANDLE hThread = (HANDLE) _beginthreadex(pSecurityAttr, dwStackSize,
			pThreadProc, pParam, dwCreationFlags, (unsigned*) &dwThreadId);
    
    if (pdwThreadId != NULL)
      *pdwThreadId = dwThreadId;
    return hThread;
	}
  
  static unsigned __stdcall ThreadProc( void *p )
  {
    CThread* pThreadObj = (CThread*) p;
    return pThreadObj->Execute();
  }
};

#endif
