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
 
#include "stdafx.h"
#include "ImageStreamSimulator.h"
#include "RunLengthStreamSimulator.h"

// fixed size stream for reading only
CImageStreamSimulator::CImageStreamSimulator(unsigned __int64 size, bool isDrive)
{
  Init(forReading, isDrive);
  fSize = size;
}

// a variable sized stream for writing only
CImageStreamSimulator::CImageStreamSimulator(bool isDrive)
{
  Init(forWriting, isDrive);
}

// a stream that simulates run length values for allocated and free clusters
CImageStreamSimulator::CImageStreamSimulator(int* runLengths, int length, TOpenMode openMode, bool isDrive)
{
  Init(openMode, isDrive);
  fRunLengthReader = new CRunLengthStreamReaderSimulator(runLengths, length);
  for (int i=0; i<length; i++)
    fSize += runLengths[i] * fClusterSize;
}

void CImageStreamSimulator::Init(TOpenMode openMode, bool isDrive)
{
  fOpenMode = openMode;
  fIsDrive = isDrive;
  fClusterSize = 4096;
  fSize = 0;
  fPosition = 0;
  fRunLengthReader = NULL;
  srand( ::GetTickCount() );
}

CImageStreamSimulator::~CImageStreamSimulator()
{
  delete fRunLengthReader;
}

LPCWSTR CImageStreamSimulator::GetName() const
{
  return L"ImageStreamSimulator";
}

void CImageStreamSimulator::Open(LPCWSTR name, TOpenMode mode)
{
  fOpenMode = mode;
}

void CImageStreamSimulator::Close()
{
}

unsigned __int64 CImageStreamSimulator::GetPosition() const
{
  return fPosition;
}

void CImageStreamSimulator::Read(void * buffer, unsigned nLength, unsigned *nBytesRead)
{
  if (fPosition + nLength <= fSize)
    *nBytesRead = nLength;
  else if (fPosition < fSize)
    *nBytesRead = (unsigned)(fSize - fPosition);
  else
    *nBytesRead = 0;

  fPosition += *nBytesRead;

  for (unsigned i=0U; i<(*nBytesRead)/sizeof(int); i++)
   ((int*)buffer)[i] = rand();
  fCrc32.AddDataBlock((BYTE*)buffer, *nBytesRead);

}

void CImageStreamSimulator::Write(void *buffer, unsigned nLength, unsigned *nBytesWritten)
{
  fCrc32.AddDataBlock((BYTE*)buffer, nLength);
  *nBytesWritten  = nLength;
  fPosition += nLength;
}

void CImageStreamSimulator::Seek(__int64 offset, DWORD moveMethod)
{
  if (moveMethod == FILE_BEGIN)
  {
    fPosition = offset;
    fSeekPositions.push_back(offset);
  }
  else if (moveMethod == FILE_END)
    fPosition = fSize + offset;
  else if (moveMethod == FILE_CURRENT)
    fPosition += offset;

  if (fPosition < 0)
    fPosition = 0;

  if (fPosition > fSize) {
    if (fOpenMode == forReading)
      fPosition = fSize;
    else // mode = forWriting 
      fSize = fPosition;
  }
}

unsigned __int64 CImageStreamSimulator::GetSize() const
{
  return fSize;
}

unsigned __int64 CImageStreamSimulator::GetAllocatedBytes() const
{
  return 0;
}

bool CImageStreamSimulator::IsDrive() const // true if volume or harddisk, false if file
{
  return fIsDrive;
}

IRunLengthStreamReader* CImageStreamSimulator::GetRunLengthStreamReader() const
{
  return fRunLengthReader;
}

DWORD CImageStreamSimulator::GetCRC32()
{
  return fCrc32.GetResult();
}

void CImageStreamSimulator::SetCompletedInformation(DWORD crc32, unsigned __int64 processedBytes)
{
}

