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
#include "..\..\src\ODIN\IImageStream.h"
#include "..\..\src\ODIN\crc32.h"
#include <vector>

class CImageStreamSimulator : public IImageStream
{

public:
  // fixed size stream for reading only
  CImageStreamSimulator(unsigned __int64 size, bool isDrive); 
  
  // a variable sized stream for writing only
  CImageStreamSimulator(bool isDrive); 

  // a stream that simulates run length values for allocated and free clusters
  CImageStreamSimulator(int* runLengths, int length, TOpenMode openMode, bool isDrive);
  virtual ~CImageStreamSimulator();

  virtual LPCWSTR GetName() const;
  virtual void Open(LPCWSTR name, TOpenMode mode);
  virtual void Close();
  virtual unsigned __int64 GetPosition() const;
  virtual void Read(void * buffer, unsigned nLength, unsigned *nBytesRead);
  virtual void Write(void *buffer, unsigned nLength, unsigned *nBytesWritten);
  virtual void Seek(__int64 Offset, DWORD MoveMethod);
  virtual unsigned __int64 GetSize() const;
  virtual unsigned __int64 GetAllocatedBytes() const;
  virtual bool IsDrive() const;  // true if volume or harddisk, false if file
  virtual IRunLengthStreamReader* GetRunLengthStreamReader() const;
  virtual void SetCompletedInformation(DWORD crc32, unsigned __int64 processedBytes);

  DWORD GetCRC32();
  
  void SetClusterSize(unsigned newSize) {
    fClusterSize = newSize;
  }
  
  unsigned GetClusterSize() const {
    return fClusterSize;
  }
  
  const std::vector<unsigned __int64>& GetSeekPositions() {
    return fSeekPositions;
  }

private:
  void Init(TOpenMode openMode, bool isDrive);

  unsigned __int64 fSize;
  unsigned __int64 fPosition;
  TOpenMode fOpenMode;
  IRunLengthStreamReader* fRunLengthReader;
  unsigned fClusterSize;
  bool fIsDrive;
  CCRC32 fCrc32;
  std::vector<unsigned __int64> fSeekPositions;
};
