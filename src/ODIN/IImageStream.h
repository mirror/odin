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
#ifndef __IMAGESTREAM_H__
#define __IMAGESTREAM_H__

/////////////////////////////////////////////////////////////////////////////////////
// The image stream classes are a simple abstraction for reading and writing disk
// or volume images. The common functionality is that you can read, write and
// and seek and that you deal with memory buffers. Subclasses deal with disk devices
// or files or network connections
/////////////////////////////////////////////////////////////////////////////////////
class IRunLengthStreamReader;

class IImageStream // interface class: no fields, no methods
{
public:
  typedef enum TOpenMode {forReading, forWriting};
  virtual ~IImageStream() {}
  virtual LPCWSTR GetName() const = 0;
  virtual void Open(LPCWSTR name, TOpenMode mode) = 0;
  virtual void Close() = 0;
  virtual unsigned __int64 GetPosition() const = 0;
  virtual void Read(void * buffer, unsigned nLength, unsigned *nBytesRead) = 0;
  virtual void Write(void *buffer, unsigned nLength, unsigned *nBytesWritten) = 0;
  virtual void Seek(__int64 offset, DWORD moveMethod) = 0;
  virtual unsigned __int64 GetSize() const = 0;
  virtual unsigned __int64 GetAllocatedBytes() const = 0;
  virtual bool IsDrive() const = 0;  // true if volume or harddisk, false if file
  virtual IRunLengthStreamReader* GetRunLengthStreamReader() const = 0;
  virtual void SetCompletedInformation(DWORD crc32, unsigned __int64 processedBytes) = 0;
};

#endif
