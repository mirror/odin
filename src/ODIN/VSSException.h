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
#ifndef __VSSEXCPETION_H__
#define __VSSEXCPETION_H__

#include "Exception.h"

#define THROW_COM_EXC_PARAM1(hr, exCode, param1) throw EVSSException((hr), __WFILE__, __LINE__, (exCode), (param1))

#define CHECK_COM_ERROR(hr, exCode, param1) { \
  if (!SUCCEEDED(hr)) { \
    THROW_COM_EXC_PARAM1(hr, exCode, param1); \
  } \
}


// special exception classes for handling Windows errors 
class EVSSException : public Exception 
{
  public:
  
  typedef enum ExceptionCode {noError, comError, vssError};

  EVSSException(HRESULT hr)
    : Exception(VSSException)
    { 
      Init(hr, noError, NULL, 0);
    }

  EVSSException(HRESULT hr, LPCWSTR file, int line) 
    : Exception(VSSException, file, line)
    { 
      Init(hr, noError, NULL, 0);
    }

  EVSSException(HRESULT hr, ExceptionCode exCode)
    : Exception(VSSException)
    { 
       Init(hr, exCode, NULL, 0);
    }

  EVSSException(HRESULT hr, LPCWSTR file, int line, ExceptionCode exCode)
    : Exception(VSSException, file, line)
    { 
      Init(hr, exCode, NULL, 0);
    }

 
  EVSSException(HRESULT hr, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t* param1)
    : Exception(VSSException, file, line)
  { 
    const wchar_t* params[1];
    params[0] = param1;
    Init(hr, exCode, params, 1);
  }

  EVSSException(HRESULT hr, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t* param1, const wchar_t* param2) 
    : Exception(VSSException, file, line)
  { 
    const wchar_t* params[2];
    params[0] = param1;
    params[1] = param2;
    Init(hr, exCode, params, 2);
  }

  EVSSException(HRESULT hr, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t* param1, const wchar_t* param2, const wchar_t* param3)
    : Exception(VSSException, file, line)
  { 
    const wchar_t* params[3];
    params[0] = param1;
    params[1] = param2;
    params[2] = param3;
    Init(hr, exCode, params, 3);
  }

  EVSSException(HRESULT hr, LPCWSTR file, int line, ExceptionCode exCode, const wchar_t** params, int len)
    : Exception(VSSException, file, line)
  { 
    Init(hr, exCode, params, len);
  }

  void AppendVSSMessage();

    int GetErrorCode() {
     return (int) fErrorCode;
  }

  private:

  void Init(HRESULT hr, ExceptionCode exCode, const wchar_t** params, int len) {
      fErrorCode = hr;
      if (noError != exCode) {
        fMessage +=sMessages[exCode];
        fMessage += L". ";
      }
      ExpandParameters(params, len);
      AppendVSSMessage();
  }

  const wchar_t* GetVSSMessageForErrorCode();
  
  HRESULT fErrorCode;

  typedef struct {long code; const wchar_t* msg;} ErrorMessage;
  static LPCWSTR sMessages[];
  static ErrorMessage sVSSErrorMessages[];

};

#endif
  // __VSSEXCPETION_H__
