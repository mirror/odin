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
#ifndef __CMDLINEEXCPETION_H__
#define __CMDLINEEXCPETION_H__

#include "Exception.h"

#define THROW_CMD_EXC(error) throw ECmdLineException((error), __WFILE__, __LINE__)

// special exception classes for handling Windows errors 
class ECmdLineException : public Exception 
{
  public:
  
  typedef enum ExceptionCode {noCode, noSource, noTarget, noOperation, wrongCompression, unknownOption,
    wrongSource, wrongTarget, wrongIndex, backupParamError, restoreParamError, verifyParamError};

  ECmdLineException(enum ExceptionCode errCode)
    : Exception(CmdLineException) { 
      Init(errCode);
    }

  ECmdLineException(enum ExceptionCode errCode, LPCWSTR file, int line)
    : Exception(CmdLineException, file, line) {
      Init(errCode);
  }


  int GetErrorCode() {
   return fErrorCode;
  }

  private:

  // methods:
  void Init(enum ExceptionCode errCode);

  // members:
  enum ExceptionCode fErrorCode;
};

#endif
