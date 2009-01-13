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
#ifndef __OSEXCPETION_H__
#define __OSEXCPETION_H__

#include "Exception.h"

#define THROW_OS_EXC(error) throw EWinException((error), __WFILE__, __LINE__)
#define THROW_OS_EXC_INFO(error, exCode) throw EWinException((error), __WFILE__, __LINE__, (exCode))
#define THROW_OS_EXC_PARAM1(error, exCode, param1) throw EWinException((error), __WFILE__, __LINE__, (exCode), (param1))
#define THROW_KERNEL_EXC_PARAM1(error, exCode, param1) throw EWinException((error), __WFILE__, __LINE__, (exCode), (param1))

#define CHECK_OS_EX(res) { \
  if (res == 0) { \
    int ret = ::GetLastError(); \
    THROW_OS_EXC(ret); \
  } \
}
#define CHECK_OS_EX_INFO(res, exCode) { \
  if (res == 0) { \
    int ret = ::GetLastError(); \
    THROW_OS_EXC_INFO(ret, exCode); \
  } \
}
#define CHECK_OS_EX_PARAM1(res, exCode, param1) { \
  if (res == 0) { \
    int ret = ::GetLastError(); \
    THROW_OS_EXC_PARAM1(ret, exCode, param1); \
  } \
}
#define CHECK_OS_EX_HANDLE(handle) { \
  if (handle == INVALID_HANDLE_VALUE) { \
    int ret = ::GetLastError(); \
    THROW_OS_EXC(ret); \
  } \
}
#define CHECK_OS_EX_HANDLE_INFO(handle, exCode) { \
  if (handle == INVALID_HANDLE_VALUE) { \
    int ret = ::GetLastError(); \
    THROW_OS_EXC_INFO(ret, exCode); \
  } \
}
#define CHECK_OS_EX_HANDLE_PARAM1(handle, exCode, param1) { \
  if (handle == INVALID_HANDLE_VALUE) { \
    int ret = ::GetLastError(); \
    THROW_OS_EXC_PARAM1(ret, exCode, param1); \
  } \
}
#define CHECK_KERNEL_EX_HANDLE_PARAM1(ntStatus, exCode, param1) { \
  if (ntStatus != 0) { \
    THROW_KERNEL_EXC_PARAM1(ntStatus, exCode, param1); \
  } \
}

// special exception classes for handling Windows errors 
class EWinException : public Exception 
{
  public:
  
  typedef enum ExceptionCode {noCode, testError, fileOpenError, volumeOpenError, ioControlError, closeHandleError,
    readFileError, writeFileError, readVolumeError, writeVolumeError, seekError, generalFileError};

  EWinException(int winRetCode)
    : Exception(OSException)
    { 
      Init(winRetCode, noCode, NULL, 0);
    }

  EWinException(int winRetCode, LPCWSTR file, int line) 
    : Exception(OSException, file, line)
    { 
      Init(winRetCode, noCode, NULL, 0);
    }

  EWinException(int winRetCode, ExceptionCode exCode)
    : Exception(OSException)
    { 
       Init(winRetCode, exCode, NULL, 0);
    }

  EWinException(int winRetCode, LPCWSTR file, int line, ExceptionCode exCode)
    : Exception(OSException, file, line)
    { 
      Init(winRetCode, exCode, NULL, 0);
    }

 
  EWinException(int winRetCode, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t* param1)
    : Exception(OSException, file, line)
  { 
    const wchar_t* params[1];
    params[0] = param1;
    Init(winRetCode, exCode, params, 1);
  }

  EWinException(int winRetCode, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t* param1, const wchar_t* param2) 
    : Exception(OSException, file, line)
  { 
    const wchar_t* params[2];
    params[0] = param1;
    params[1] = param2;
    Init(winRetCode, exCode, params, 2);
  }

  EWinException(int winRetCode, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t* param1, const wchar_t* param2, const wchar_t* param3)
    : Exception(OSException, file, line)
  { 
    const wchar_t* params[3];
    params[0] = param1;
    params[1] = param2;
    params[2] = param3;
    Init(winRetCode, exCode, params, 3);
  }

  EWinException(int winRetCode, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t** params, int len)
    : Exception(OSException, file, line)
  { 
    Init(winRetCode, exCode, params, len);
  }

  // with NtStatus for kernel errors
  EWinException(long ntStatus, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t* param1)
    : Exception(OSException, file, line)
  { 
    const wchar_t* params[1];
    params[0] = param1;
    InitKernel(ntStatus, exCode, params, 1);
  }

  virtual void AppendWindowsMessage();
  void AppendNtStatusMessage();
  void AppendVSSMessage();

  int GetErrorCode() {
   return fErrorCode;
  }

  private:

  void Init(int winRetCode, ExceptionCode exCode, const wchar_t** params, int len) {
      fErrorCode = winRetCode;
      if (noCode != exCode) {
        fMessage +=sMessages[exCode];
        fMessage += L". ";
      }
      ExpandParameters(params, len);
      AppendWindowsMessage();
  }

  void InitKernel(long ntStatus, ExceptionCode exCode, const wchar_t** params, int len) {
      fErrorCode = ntStatus;
      if (noCode != exCode) {
        fMessage +=sMessages[exCode];
        fMessage += L". ";
      }
      ExpandParameters(params, len);
      AppendNtStatusMessage();
  }

  
  int fErrorCode;

  static LPCWSTR sMessages[];

};

#endif