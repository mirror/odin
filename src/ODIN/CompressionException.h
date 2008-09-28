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
#ifndef __COMPRESSIONEXCEPTION_H__
#define __COMPRESSIONEXCEPTION_H__

#include "Exception.h"

// special exception classes for handling compression and decompression errors in libz and bzio2 lib.
class ECompressionException : public Exception 
{
  public:
  //typedef enum ExceptionCode {errNo, streamError, dataError, memError, bufError, versionError};
  
  ECompressionException(int errCode, int firstCode, int lastCode) : 
    Exception(CompressionException), fFirstCode(firstCode), fLastCode(lastCode)
    { 
      fErrorCode = errCode;
    }

  ECompressionException(int errCode, LPCWSTR file, int line, int firstCode, int lastCode) : 
    Exception(CompressionException, file, line), fFirstCode(firstCode), fLastCode(lastCode)
    { 
      fErrorCode = errCode;
    }

protected:
  int fErrorCode;
  const int fFirstCode;
  const int fLastCode;
};

class EZLibCompressionException : public ECompressionException 
{
  public:
  
  EZLibCompressionException(int errCode) : 
    ECompressionException(errCode, -6, -1)
    { 
      BuildMessageString();
    }

  EZLibCompressionException(int errCode, LPCWSTR file, int line) : 
    ECompressionException(errCode, file, line, -6, -1)
    { 
      BuildMessageString();
    }

  virtual void BuildMessageString();  

private:
  static LPCWSTR sMessages[];
};

class EBZip2CompressionException : public ECompressionException 
{
  public:
  
  EBZip2CompressionException(int errCode) : 
    ECompressionException(errCode, -9, -1)
    { 
      BuildMessageString();
    }

  EBZip2CompressionException(int errCode, LPCWSTR file, int line) : 
    ECompressionException(errCode, file, line, -9, -1)
    { 
      BuildMessageString();
    }

  virtual void BuildMessageString();  

private:
  static LPCWSTR sMessages[];
};

void exceptionTest();

#endif

