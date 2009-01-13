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
#ifndef __INTERNALEXCEPTION_H__
#define __INTERNALEXCEPTION_H__

#include "Exception.h"


// special exception classes for handling compression and decompression errors in libz and bzio2 lib.
#define THROW_INT_EXC(error) throw EInternalException((error), __WFILE__, __LINE__)

class EInternalException : public Exception 
{
  public:
  typedef enum ExceptionCode {volumeBitmapBufferSizeError, ext2GroupDescrReadError, ext2BlockGroupReadError,
    threadSyncTimeout, inputTypeNotSet, outputTypeNotSet, getChunkError, writeChunkError, wrongReadSize,
    wrongWriteSize, internalStringTableOverflow, chunkSizeTooSmall, maxPartitionNumberExceeded,
    unsupportedPartitionFormat,
  };
  
  EInternalException(int errCode) : 
    Exception(InternalException)
    { 
      fErrorCode = errCode;
      BuildMessageString();
    }

  EInternalException(int errCode, LPCWSTR file, int line) : 
    Exception(InternalException, file, line)
    { 
      fErrorCode = errCode;
      BuildMessageString();
    }

  int GetErrorCode() const
  {
    return fErrorCode;
  }

  virtual void BuildMessageString();  

private:
  int fErrorCode;
  static LPCWSTR sMessages[];
};

#endif
