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
#ifndef __COMPRESSEDRUNLENGHTSTREAM_H__
#define __COMPRESSEDRUNLENGHTSTREAM_H__

#include <string>
#include <vector>
#include "IRunLengthStreamReader.h"

//---------------------------------------------------------------------------
// CBitArray class
//

class CBitArray {
  protected:
    unsigned  fArraySize;
    BYTE *fArray;
  public:
    void  SetSize(unsigned NewSize);
    void  SetBit(unsigned Index, bool Value);
    bool  GetBit(unsigned Index) const;

    CBitArray(void);
    CBitArray(void *Bits, unsigned BitCount);

     ~CBitArray();
    void  LoadBuffer(void *Bits, unsigned BitCount);
    unsigned  FindSetBit(void);
    unsigned GetRunLength(unsigned index, bool value);
 
    bool inline  HasSetBit(void) { 
      return (FindSetBit() < fArraySize); 
    }

	  bool operator [] (unsigned index) const {
		  return GetBit(index);
	}

  unsigned GetSize()
	{
		return fArraySize;
	}
private:
   unsigned GetRunLengthSet(unsigned index);
   unsigned GetRunLengthNotSet(unsigned index);

  friend class BitArrayTest;
};  // class CBitArray


class CompressedRunLengthStreamReader: public IRunLengthStreamReader {
public:
  CompressedRunLengthStreamReader(HANDLE file, unsigned __int64 seekPos, DWORD length);
  CompressedRunLengthStreamReader(LPCWSTR filePath, unsigned __int64 seekPos, DWORD length);
  ~CompressedRunLengthStreamReader();

  unsigned __int64 GetNextRunLength();

  bool LastValueRead() {
    return (!fValueAvailable) && fTupelPos > fLastTokenIndex;
  }

private:
  void Initialize(HANDLE file, unsigned __int64 seekPos, DWORD length);
  void ReadNextRunLength();
  // read next chunk from file into buffer
  void ReloadBuffer();

private:
  DWORD fTupelPos; // position in current tupel (0..3)

  unsigned __int64 fRunLength[4]; // run length values of current tupel
  bool fEof;      // end of stream was reached
  bool fValueAvailable; // last run length was read
  unsigned fLastTokenIndex; // last valid index fRunLength after last token was read;

  static const unsigned cBufferLen = 65536;
  BYTE *fBuffer;
  BYTE *fBufferPos;
  BYTE *fBufferEndPos;

  HANDLE fFile;
  unsigned __int64 fStreamPos;
  DWORD fStreamBytesConsumed;
  DWORD fStreamLength;
  bool fMustCloseFile;
  std::wstring fFileName;
};

class CompressedRunLengthStreamWriter {

public:
  CompressedRunLengthStreamWriter(HANDLE hFile, unsigned extraClustersAtBegin = 0);
  ~CompressedRunLengthStreamWriter();

  void AddBuffer(void* buffer, unsigned length);
  void Flush();
  unsigned __int64 Get1Count() {
    return fCountBitsSet;
  }
  unsigned __int64 Get0Count() {
    return fCountBitsCleared;
  }


private:
  void EncodeAndStoreRunLength(unsigned __int64 runLength);
  void EnhanceLastRunLength(unsigned __int64 runLength);
  void WriteBuffer();
  void WriteHeaderByte();

  static const unsigned cBufferLen = 65536;
  bool fBitValue; // current run length for either a sequence of 1s or 0s
  BYTE *fBuffer;
  BYTE *fBufferPos;
  BYTE *fBufferEndPos;
  HANDLE fFile;
  DWORD fTupelPos; // position in current tupel (0..3)
  BYTE* fHeaderByte;
  unsigned __int64 fLastRunLength;    // run length last read
  unsigned __int64 fCountBitsSet;     // total number of 1 bits
  unsigned __int64 fCountBitsCleared; // total number of 0 bits
  friend class RLTest;
};

#if 0
class CBitArrayTest {
public:
  static void BitArrayTest();
};

class RLTest {
public:
  static void Test();
private:
  static void SimpleTest();
  static void OverflowTest();
  static void ContinueSequenceTest();
  static void ReaderTest1();
  static void ReaderTest2();
  static void ReaderTest3();
  static void ReaderTest4();
};
#endif

#endif