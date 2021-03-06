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
 
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

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

// There is a warning when compiling with VS2008 and later, to avoid define...
#define _CRT_NON_CONFORMING_SWPRINTFS

// Support for VS2005 Express & SDK ATL
#ifdef _WTL_SUPPORT_SDK_ATL3
  #define _CRT_SECURE_NO_DEPRECATE
  #pragma conform(forScope, off)
  #pragma comment(linker, "/NODEFAULTLIB:atlthunk.lib")
#endif // _WTL_SUPPORT_SDK_ATL3

#include <atlbase.h>

// Support for VS2005 Express & SDK ATL
#ifdef _WTL_SUPPORT_SDK_ATL3
// !!!!!!!!!!!!!
// for 64-Bit change the definitions AllocStdCallThunk and FreeStdCallThunk (now in stdafx.h):
// see: http://www.zabkat.com/blog/26Oct08-x64-development-with-VS6-and-ATL3.htm
// All that's left is a slight but important change to ATLBASE.H so that thunks will run from executable memory avoiding DEP faults, which are not tolerated in 64 bit windows versions. You must comment out the definitions of AllocStdCallThunk and FreeStdCallThunk and replace them with:
// !!!!!!!!!!!!!
namespace ATL
  {
	inline void * __stdcall __AllocStdCallThunk()
	{
	  // return ::HeapAlloc(::GetProcessHeap(), 0, sizeof(_stdcallthunk));
      // change to be compatible with 64-Bit see comment above
      return ::VirtualAlloc(0, sizeof(_stdcallthunk), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	}

	inline void __stdcall __FreeStdCallThunk(void *p)
	{
	  // ::HeapFree(::GetProcessHeap(), 0, p);
      // change to be compatible with 64-Bit
      // change to be compatible with 64-Bit see comment above
      ::VirtualFree(p, 0, MEM_RELEASE);
	}
  };
#endif // _WTL_SUPPORT_SDK_ATL3

#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// to track memory allocations:
#include "DebugMem.h"
