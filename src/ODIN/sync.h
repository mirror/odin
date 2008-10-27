#pragma once
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
 

#ifdef _WTL_SUPPORT_SDK_ATL3
class CCriticalSection :
	public CRITICAL_SECTION
{
public:
	CCriticalSection()
	{
		__try
		{
			::InitializeCriticalSection( this );
		}
		__except( STATUS_NO_MEMORY == GetExceptionCode() )
		{
			RaiseException( STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL );
		}
	}

	explicit CCriticalSection( ULONG nSpinCount )
	{
		__try
		{
			BOOL bRet = ::InitializeCriticalSectionAndSpinCount( this, nSpinCount );
			if (!bRet)
			{
				RaiseException( STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL );
			}
		} __except(STATUS_NO_MEMORY == GetExceptionCode()) {
			RaiseException( STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL );
		}
	}

	~CCriticalSection() throw()
	{
		::DeleteCriticalSection( this );
	}

	// Acquire the critical section
	void Enter()
	{
		::EnterCriticalSection( this );
	}

	// Release the critical section
	void Leave() throw()
	{
		::LeaveCriticalSection( this );
	}

	// Set the spin count for the critical section
	ULONG SetSpinCount( ULONG nSpinCount ) throw()
	{
		return( ::SetCriticalSectionSpinCount( this, nSpinCount ) );
	}

	BOOL TryEnter() throw()
	{
		return( ::TryEnterCriticalSection( this ) );
	}
};
class CHandle
{
public:
	CHandle() throw()
		: m_h( NULL )
	{
	}

	CHandle( CHandle& h ) throw()
		: m_h( NULL )
	{
		Attach( h.Detach() );
	}
	explicit CHandle( HANDLE h ) throw()
		: m_h( h )
	{
	}

	~CHandle() throw()
	{
		if( m_h != NULL )
		{
			Close();
		}
	}

	CHandle& operator=( CHandle& h ) throw()
	{
		if( this != &h )
		{
			if( m_h != NULL )
			{
				Close();
			}
			Attach( h.Detach() );
		}
		return( *this );
	}

	operator HANDLE() const throw()
	{
		return( m_h );
	}

	void Attach( HANDLE h ) throw()
	{
		m_h = h;  // Take ownership
	}

	HANDLE Detach() throw()
	{
		HANDLE h;

		h = m_h;  // Release ownership
		m_h = NULL;

		return( h );
	}

	void Close() throw()
	{
	}

public:
	HANDLE m_h;
};

class CSemaphore :
	public CHandle
{
public:
	CSemaphore() throw()
	{
	}

	CSemaphore( CSemaphore& h ) throw()
	{
	}

	CSemaphore( LONG nInitialCount, LONG nMaxCount )
	{
		BOOL bSuccess;

		bSuccess = Create( NULL, nInitialCount, nMaxCount, NULL );
		if( !bSuccess )
		{
			DWORD dwError = ::GetLastError();
			RaiseException( HRESULT_FROM_WIN32( dwError ), EXCEPTION_NONCONTINUABLE, 0, NULL );
		}
	}

	CSemaphore( LPSECURITY_ATTRIBUTES pSecurity, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName )
	{
		BOOL bSuccess;

		bSuccess = Create( pSecurity, nInitialCount, nMaxCount, pszName );
		if( !bSuccess )
		{
			DWORD dwError = ::GetLastError();
			RaiseException( HRESULT_FROM_WIN32( dwError ), EXCEPTION_NONCONTINUABLE, 0, NULL );
		}
	}
	
	explicit CSemaphore( HANDLE h ) throw()
		: CHandle( h )
	{
	}
	
	BOOL Create( LPSECURITY_ATTRIBUTES pSecurity, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName ) throw()
	{
		ATLASSERT( m_h == NULL );

		m_h = ::CreateSemaphore( pSecurity, nInitialCount, nMaxCount, pszName );
		return( m_h != NULL );
	}
	
	BOOL Open( DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName ) throw()
	{
		ATLASSERT( m_h == NULL );

		m_h = ::OpenSemaphore( dwAccess, bInheritHandle, pszName );
		return( m_h != NULL );
	}
	
	BOOL Release( LONG nReleaseCount = 1, LONG* pnOldCount = NULL ) throw()
	{
		ATLASSERT( m_h != NULL );
		return( ::ReleaseSemaphore( m_h, nReleaseCount, pnOldCount ) );
	}
};

#else // _WTL_SUPPORT_SDK_ATL3
#include <atlsync.h>

#endif  //_WTL_SUPPORT_SDK_ATL3
