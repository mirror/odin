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
#ifndef __FILEFORMATEXCEPTION_H__
#define __FILEFORMATEXCEPTION_H__

#include "Exception.h"


// special exception classes for handling compression and decompression errors in libz and bzio2 lib.
#define THROW_FILEFORMAT_EXC(error) throw EFileFormatException((error), __WFILE__, __LINE__)

class EFileFormatException : public Exception 
{
  public:
  typedef enum ExceptionCode {magicByteError, wrongFileOffsetError, wrongCommentLength,
    wrongChecksumLength, majorVersionError, wrongChecksumMethod, wrongCompressionMethod,
    wrongVolumeEncodingMethod, wrongFileSizeError,
  };
  
  EFileFormatException(int errCode) : 
    Exception(FileFormatException)
    { 
      fErrorCode = errCode;
      BuildMessageString();
    }

  EFileFormatException(int errCode, LPCWSTR file, int line) : 
    Exception(FileFormatException, file, line)
    { 
      fErrorCode = errCode;
      BuildMessageString();
    }

  virtual void BuildMessageString();  

  int GetErrorCode() const {
    return fErrorCode;
  }

private:
  int fErrorCode;
  static LPCWSTR sMessages[];
};

#endif
