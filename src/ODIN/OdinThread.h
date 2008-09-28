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
#ifndef __ODINTHREAD_H__
#define __ODINTHREAD_H__

#include <string>
#include "Thread.h"

class COdinThread : public CThread
{
  public:
    COdinThread(DWORD dwCreationFlags = 0)
      : CThread(dwCreationFlags) { 
		fFinished = fErrorFlag = false;
    fBytesProcessed = fCrc32 = 0;
	}

	__int64 GetBytesProcessed() {
		return fBytesProcessed;
	};
    
	DWORD GetCrc32() {
		return fCrc32;
	};

  bool HasFinished() {
		return fFinished;
	};
	
	bool GetErrorFlag() {
		return fErrorFlag;
	};
	
	LPCWSTR GetErrorMessage() {
		return fErrorMessage.c_str();
	};


  protected:

  __int64 fBytesProcessed;
  bool    fFinished;
  bool    fErrorFlag;
  std::wstring fErrorMessage;
  DWORD   fCrc32;
}; 
//---------------------------------------------------------------------------
#endif
