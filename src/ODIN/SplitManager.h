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
#include "ImageStream.h"
#include "SplitManagerCallback.h"

// CSplitmanager is a class that manages splitting image files into chunks of a predefined size
// Note: This implementation assumes that the chunk size is not smaller than the size of a chunk in the queue
//       (i.e. each read/write request spans at most two files)


class CSplitManager : public IFileImageStreamCallback {
public:
    // constructor for writing access to split files
  CSplitManager(LPCWSTR fileNamePrefix, unsigned __int64 chunkSize, CFileImageStream *stream, ISplitManagerCallback* provider);
    // constructor for reading access to split files
  CSplitManager(LPCWSTR fileNamePrefix, CFileImageStream *stream, unsigned __int64 totalSize, ISplitManagerCallback* provider);
  virtual ~CSplitManager();
  virtual bool PreReadEvent(void * buffer, unsigned length, unsigned *bytesRead);
  virtual bool PreWriteEvent(void *buffer, unsigned length, unsigned *bytesWritten);
  virtual bool PreSeekEvent(__int64 offset, DWORD moveMethod);

  unsigned GetFileNo() {
    // for testing
    return fFileNo;
  }
  
  unsigned GetFileCount() {
    return fFileCount;
  }

  void GetFileName(unsigned fileNo, std::wstring& fileName);

private:
  typedef enum {modeRead, modeWrite} TMode;

  void Init(LPCWSTR fileNamePrefix, CFileImageStream *stream);
  void SwitchFile (unsigned newFileNo);
  unsigned __int64 GetFileSize(LPCWSTR fileName);

  TMode fMode; // reading or writing
  CFileImageStream* fStream;
    // file image stream where we act as callback
  unsigned __int64 fSize;
    // total size of stream
  unsigned __int64 fChunkSize; 
    // size of each file chunk in bytes
  unsigned fFileNo;
    // index of actual file that is opened
  unsigned fFileCount;
    // number of total file chunks for image
  std::wstring fFileNamePrefix;
    // get file name or ask user for file
  ISplitManagerCallback* fCallback;
};