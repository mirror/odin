// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <stdio.h>
#include <tchar.h>

// Change these values to use different versions
//#define WINVER		0x0400
//#define _WIN32_WINNT	0x0400
#define WINVER		0x0500
#define _WIN32_WINNT 0x0502
#define _WIN32_IE	0x0502
#define _RICHEDIT_VER	0x0200

// This project was generated for VC++ 2005/8 Express and ATL 3.0 from Platform SDK.
// Comment out this line to build the project with different versions of VC++ and ATL.
#define _WTL_SUPPORT_SDK_ATL3

// Support for VS2005 Express & SDK ATL
#ifdef _WTL_SUPPORT_SDK_ATL3
  #define _CRT_SECURE_NO_DEPRECATE
  #pragma conform(forScope, off)
  #pragma comment(linker, "/NODEFAULTLIB:atlthunk.lib")
#endif // _WTL_SUPPORT_SDK_ATL3

#include <atlbase.h>

// Support for VS2005 Express & SDK ATL
#ifdef _WTL_SUPPORT_SDK_ATL3
  namespace ATL
  {
	inline void * __stdcall __AllocStdCallThunk()
	{
		return ::HeapAlloc(::GetProcessHeap(), 0, sizeof(_stdcallthunk));
	}

	inline void __stdcall __FreeStdCallThunk(void *p)
	{
		::HeapFree(::GetProcessHeap(), 0, p);
	}
  };
#endif // _WTL_SUPPORT_SDK_ATL3


#ifdef _WTL_SUPPORT_SDK_ATL3
#include <atlapp.h>
extern CAppModule _Module;
#endif // _WTL_SUPPORT_SDK_ATL3
#include <atlbase.h>
#include <atlwin.h>

// to track memory allocations:
#include "..\..\src\ODIN\DebugMem.h"

