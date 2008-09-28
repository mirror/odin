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
#include "CompressedRunLengthStream.h"
#include <string>
#include "OSException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

const unsigned longestTupelLength = 33; // 1+8+8+8+8

//////////////////////////////////////////////////////////////////////////////////////////////////////
// CBitArray class 
//////////////////////////////////////////////////////////////////////////////////////////////////////

const int BITSPERUNIT = 8;

CBitArray::CBitArray(void)
{
  fArray = NULL;
  fArraySize = 0;  
} 
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

CBitArray::~CBitArray()
{
  SetSize(0);
}


//---------------------------------------------------------------------------
// void wstring CBitArray::SetSize(unsigned Value)
//
// Resize the array.  Any new space added is zeroed out.  Old values are
// copied over.  I could have used SysReallocMem() instead of allocating
// a new buffer and copying the old contents every time, but even realloc
// will do a whole new alloc and copy contents over a lot of the time (it
// just does it behind the scenes), and my memcpy is way faster than its is.
//
void CBitArray::SetSize(unsigned newSize)
{
  BYTE *newMem = NULL;
  unsigned nNewMemSize, nOldMemSize;

  nNewMemSize = (newSize + BITSPERUNIT - 1) / BITSPERUNIT * sizeof(BYTE);
  nOldMemSize = (fArraySize + BITSPERUNIT - 1) / BITSPERUNIT * sizeof(BYTE);
  if (nNewMemSize != nOldMemSize) {
    if (nNewMemSize) {
      newMem = (BYTE *)malloc(nNewMemSize);
      if (nNewMemSize > nOldMemSize)
        memset(newMem + nOldMemSize, 0, nNewMemSize - nOldMemSize);
    }
    if (nOldMemSize) {
      if (nNewMemSize)
        memcpy(newMem, fArray, min(nNewMemSize, nOldMemSize));
      free(fArray);
    }
    fArray = newMem;
  } 
  fArraySize = newSize;
}

//---------------------------------------------------------------------------
// void wstring CBitArray::LoadBuffer(void *Buffer, unsigned BitCount)
//
// Load the bit array from an external buffer.
//
void CBitArray::LoadBuffer(void *bits, unsigned bitCount)
{
  unsigned nBufferBytes = (bitCount + 7) / 8;
  unsigned nNewMemSize = (bitCount + BITSPERUNIT - 1) / BITSPERUNIT * sizeof(BYTE);
  //BYTE nMask;

  if (fArray)
    free(fArray);
  fArray = (BYTE*)malloc(nNewMemSize);
  memcpy(fArray, bits, nBufferBytes);
  fArraySize = bitCount;

}

//---------------------------------------------------------------------------
//
// Set a bit in the array
//
// For a wstring method, "this" starts out in EAX, and the first two
// regular parameters are in EDX and ECX respectively.  This simplifies
// our assembly somewhat.  "Value" is actually in the low 8 bits of ECX
// (or CL) because bools are stored in one byte in Borland compilers.
//
void CBitArray::SetBit(unsigned index, bool value)
{
  unsigned byteIndex =index/BITSPERUNIT; 
  BYTE b = fArray[byteIndex];
  BYTE mask = 0x1;  // 0000 0001 
  if (value) {
    mask = mask << index % BITSPERUNIT;
    b |= mask;
  } else {
    mask = ~(mask << index % BITSPERUNIT);
    b &= mask;
  }
  fArray[byteIndex] = b;
 }

//---------------------------------------------------------------------------
// bool CBitArray::GetBit(unsigned Index)
//
// Retrieve a bit in the array.  
bool CBitArray::GetBit(unsigned index) const
{
  BYTE b = fArray[index/BITSPERUNIT];
  BYTE mask = 0x1; // 0000 0001;
  bool value;

  mask = mask << index % BITSPERUNIT;
  value = (b & mask) != 0;
  return value;
}

//---------------------------------------------------------------------------
// unsigned CBitArray::FindSetBit(void)
//
// Find the first set bit in the bit array.  If there are none, then it
// returns the size of the array.
//
unsigned CBitArray::FindSetBit(void)
{
  unsigned nArraySize = (fArraySize + BITSPERUNIT - 1) / BITSPERUNIT;
  for (unsigned n = 0; n < nArraySize; n++)
    if (fArray[n]) {
      int nBit = n * sizeof(BYTE) * BITSPERUNIT;
      while (!GetBit(nBit))
        nBit++;
      return nBit;
    }
  return fArraySize;
}

//---------------------------------------------------------------------------
// unsigned CBitArray::GetRunLength(unsigned index, bool value)
//
// get the number of bits following index that are set (value=true)
// or not set (value=false).
//
unsigned CBitArray::GetRunLength(unsigned index, bool value) {
  if (value)
    return GetRunLengthSet(index);
  else
    return GetRunLengthNotSet(index);
}

unsigned CBitArray::GetRunLengthNotSet(unsigned index)
{
  unsigned nArraySize = fArraySize / BITSPERUNIT;
  unsigned start = (index+BITSPERUNIT-1) / BITSPERUNIT;
  int nBit=start*BITSPERUNIT;
  unsigned n;

  // from index to first byte boundary
  for (n = index; n < start*BITSPERUNIT; n++) {
      if (GetBit(n))
        return n-index;
  }
  // all bytes that are complete
  for (n = start; n < nArraySize; n++)
    if (fArray[n]) {
      nBit = n * sizeof(BYTE) * BITSPERUNIT;
      while (!GetBit(nBit++))
        ;
      return nBit-index-1;
    }
  // from last byte boundary until end of bit field
  for (n = nBit; n < fArraySize; n++) {
      if (GetBit(n))
        return n-index;
  }

  return fArraySize-index;
}

unsigned CBitArray::GetRunLengthSet(unsigned index)
{
  unsigned nArraySize = fArraySize / BITSPERUNIT;
  unsigned start = (index+BITSPERUNIT-1) / BITSPERUNIT;
  int nBit=start*BITSPERUNIT;
  unsigned n;

  // from index to first byte boundary
  for (n = index; n < start * BITSPERUNIT; n++) {
      if (!GetBit(n))
        return n-index;
  }
   // all bytes that are complete
  for (n = start; n < nArraySize; n++)
    if (fArray[n] != 255) {
      nBit = n * sizeof(BYTE) * BITSPERUNIT;
      while (GetBit(nBit++))
        ;
      return nBit-index-1;
    }
  // from last byte boundary until end of bit field
  for (n = nBit; n < fArraySize; n++) {
      if (!GetBit(n))
        return n-index;
  }

  return fArraySize-index;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
// class CompressedRunLengthStreamReader
//////////////////////////////////////////////////////////////////////////////////////////////////////

// This class is used to store the bitmaps of used clusters from a partition. As we are only 
// interested in contigoous sequences of clusters we use a run length encoding of clusters that
// are alternating free or used. Such a sequence might be number that can be stored in one, two,
// four or eight bytes. We use one byte as header containing the information how many bytes are used
// to store the next 4 run lengths. Bit 0 and 1 indicate length of first run length, bit 2 and 3 length
// of second run length, up to bit 6 and 7 of fourth value. 00 indicate a one byte length, 01 a 16 bit
// value, 10 a four byte value and 11 an 8 byte run length. Example (| indicates value boundary)
// 11001001 | 0xAABB | 0xCCDDEEFF | 0x77 | 0xAABBCCDDEEFF0000
// ^ header byte indicating that a 01 (2 Byte) then 10 (4 Byte) then 00 (1 Byte) then 11 (8 Byte) value 
// run length follows. The stream consists of a long sequence of such 4 variable length value symbols.
// At the end of the stream the last symbol is filled with zero length values.
// Thread synchronization:
// Writing of a cluster allocation bitmap always occurs single threaded. But when a volume is backuped
// the clusters are appended at the end of the image file in parallel to reading run length values at 
// the beginning of the file from different threads. To avoid introducing complicated thread synchronization
// objects the run length stream read just opens the file a second time to have its own file pointer. In this
// way both threads can seek individually.


CompressedRunLengthStreamReader::CompressedRunLengthStreamReader(HANDLE file, unsigned __int64 seekPos, DWORD length) {
  // initialize stream from file, seekPos indicates the position in the file where the stream starts
  // length is the length of the stream in the file
  Initialize(file, seekPos, length);
}

CompressedRunLengthStreamReader::CompressedRunLengthStreamReader(LPCWSTR filePath, unsigned __int64 seekPos, DWORD length) 
{
  fFileName = filePath;
  // initialize stream from file path, seekPos indicates the position in the file where the stream starts
  // length is the length of the stream in the file
  HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  CHECK_OS_EX_HANDLE_PARAM1(hFile, EWinException::fileOpenError, fFileName.c_str());

  Initialize(hFile, seekPos, length);
  fMustCloseFile = true;
}

CompressedRunLengthStreamReader::~CompressedRunLengthStreamReader() {
  BOOL ok;

  delete [] fBuffer;
  if (fMustCloseFile)
    ok = CloseHandle(fFile);
}

void CompressedRunLengthStreamReader::Initialize(HANDLE file, unsigned __int64 seekPos, DWORD length)
{
  fBuffer = new BYTE [cBufferLen];
  fBufferPos = fBufferEndPos = fBuffer;
  fEof = false;
  fFile = file;
  fStreamPos = seekPos;
  fStreamLength = length;
  fStreamBytesConsumed = 0;
  fTupelPos = 4;
  fLastTokenIndex = 3;
  fValueAvailable = true;
  fMustCloseFile = false;
  ReloadBuffer();
}

// get next run length value from stream
unsigned __int64 CompressedRunLengthStreamReader::GetNextRunLength() {

  if (3<fTupelPos && fLastTokenIndex == 3)
    ReadNextRunLength();
  ATLTRACE("load run length: %lu\n", fRunLength[fTupelPos]);
  return fRunLength[fTupelPos++];
}

// read next four values from stream  
void CompressedRunLengthStreamReader::ReadNextRunLength() {
  
  unsigned byteLen[4];
  
  if (fBufferPos + longestTupelLength > fBufferEndPos && !fEof) 
    ReloadBuffer();

  fTupelPos = 0;
  BYTE b = *fBufferPos++;
  byteLen[0] = b & 3;          // 00000011
  byteLen[1] = (b & 12) >> 2;  // 00001100
  byteLen[2] = (b & 48) >> 4;  // 00110000
  byteLen[3] = (b & 192) >> 6; // 11000000

  for (int i=0; i<4; i++) {
    if (byteLen[i] == 0) {         // 0 means 1 byte length
        fRunLength[i] = (unsigned __int64) (*(BYTE*) fBufferPos);
        ++fBufferPos;
    } else if (byteLen[i] == 1) {  // 1 means 2 byte length
        fRunLength[i] = (unsigned __int64) (*(WORD*) fBufferPos);
        fBufferPos += 2;
    } else if (byteLen[i] == 2) {  // 2 means 4 byte length
        fRunLength[i] = (unsigned __int64) (*(DWORD*) fBufferPos);
        fBufferPos += 4;
    } else if (byteLen[i] == 3) {  // 3 means 8 byte length
        fRunLength[i] = (unsigned __int64) (*(unsigned __int64*) fBufferPos);
        fBufferPos += 8;
    } else
        ATLASSERT(false); // wrong length calucalated or corrupted file
  }
  
  // When last block is read eliminate padded zero run lengths at the end
  if (fEof && fBufferPos >= fBufferEndPos) {
    while (fRunLength[fLastTokenIndex] == 0)
      fLastTokenIndex--;
    fValueAvailable = false;
  }
}

// read next chunk from file into buffer
// This function resets the current file pointer. Because it is relatively rarely called
// and calls may interfer with other read calls in the same file it is quite handy
// to restore the file pointer to the state when this function was called.
void CompressedRunLengthStreamReader::ReloadBuffer() {

  DWORD len = (DWORD)(fBufferEndPos - fBufferPos);
  DWORD sizeRead, sizeToRead;
  BOOL ok;
  LARGE_INTEGER wantedStreamPos, newStreamPos;

  // save current file pointer:
  LARGE_INTEGER beforePos;
  wantedStreamPos.QuadPart = 0LL;
  ok = SetFilePointerEx(fFile, wantedStreamPos, &beforePos, FILE_CURRENT);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, fFileName.c_str());

  // reload buffer
  memcpy(fBuffer, fBufferPos, len);

  BYTE* readPos = fBuffer + len;
  sizeToRead = cBufferLen - len;
  if (fStreamBytesConsumed + sizeToRead > fStreamLength) {
    fEof = true;
    sizeToRead = fStreamLength - fStreamBytesConsumed;
  }
  wantedStreamPos.QuadPart = fStreamPos + fStreamBytesConsumed;
  ok = SetFilePointerEx(fFile, wantedStreamPos , &newStreamPos, FILE_BEGIN);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, fFileName.c_str());
  ok = ReadFile(fFile, readPos, sizeToRead, &sizeRead, NULL);
  CHECK_OS_EX_PARAM1(ok, EWinException::readFileError, fFileName.c_str());
  fStreamBytesConsumed += sizeRead;
  fBufferEndPos = readPos + sizeRead;
  fBufferPos = fBuffer;

  // restore previous file pointer
  ok = SetFilePointerEx(fFile, beforePos, NULL, FILE_BEGIN);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, fFileName.c_str());
}
  
//////////////////////////////////////////////////////////////////////////////////////////////////////
// class CompressedRunLengthStreamWriter
//////////////////////////////////////////////////////////////////////////////////////////////////////

CompressedRunLengthStreamWriter::CompressedRunLengthStreamWriter(HANDLE hFile, unsigned extraClustersAtBegin) {
  fFile = hFile;
  fBitValue = true;
  fBuffer = new BYTE [cBufferLen];
  fBufferPos = fBuffer;
  fBufferEndPos = fBuffer + cBufferLen;
  fTupelPos = 3;
  fHeaderByte = fBufferPos;
  (*fHeaderByte) = 0;
  fLastRunLength = 0; 
  fCountBitsSet = 0;
  fCountBitsCleared = 0;
  if (extraClustersAtBegin > 0) {
    EncodeAndStoreRunLength(extraClustersAtBegin);
    fLastRunLength = extraClustersAtBegin;
  }
}

CompressedRunLengthStreamWriter::~CompressedRunLengthStreamWriter() {
  delete [] fBuffer;
}

void CompressedRunLengthStreamWriter::AddBuffer(void* buffer, unsigned length) {
  
  CBitArray bitArray;
  unsigned __int64 runLength = 0;

  bitArray.LoadBuffer(buffer, length);
  bool newBitValue = bitArray.GetBit(0);

  // if it is the first block and this starts with 0 add a zero run length because we always
  // start with a run length of 1
  if (fLastRunLength == 0 && !newBitValue)
    EncodeAndStoreRunLength(0);

  // check if we need to enhance the last run length or a bit value change occured
  if (newBitValue == fBitValue && fLastRunLength != 0) {
    // add new run length to current run length
    runLength = bitArray.GetRunLength((unsigned) 0, fBitValue);
    EnhanceLastRunLength(runLength);
    fBitValue = !fBitValue;
  } else {
    fBitValue = newBitValue;
  }

  for (unsigned __int64 i=runLength; i < length; ) {
      runLength = bitArray.GetRunLength((unsigned) i, fBitValue);
      EncodeAndStoreRunLength(runLength);
      fBitValue = !fBitValue;
      i+=runLength;
  }
  // save last values:
  fBitValue = !fBitValue;
  fLastRunLength = runLength;
}

void CompressedRunLengthStreamWriter::Flush() {
  for (int i=fTupelPos+1; i<4; i++) {
    EncodeAndStoreRunLength(0);
  }
//  fBufferPos--; // next header byte is already assigned
  WriteBuffer();
}

void CompressedRunLengthStreamWriter::EnhanceLastRunLength(unsigned __int64 runLength) {
  
  BYTE fieldWidthTag = (*fHeaderByte) & 0xC0 ;
  // reset pointer of output buffer before the last token
  if (fieldWidthTag == 0xC0) { // 11000000
    fBufferPos -= 8;
  } else if (fieldWidthTag == 0x80) { // 10000000
    fBufferPos -= 4;
  } else if (fieldWidthTag == 0x40) { // 01000000
    fBufferPos -= 2;
  } else { // 00000000
    fBufferPos -= 1;
  }

  runLength += fLastRunLength;
  (*fHeaderByte) <<= 2;
  --fTupelPos;
  if ((fTupelPos & 1) == 0)
    fCountBitsCleared -= fLastRunLength;
  else
    fCountBitsSet -= fLastRunLength;
  return EncodeAndStoreRunLength(runLength);
}

void CompressedRunLengthStreamWriter::EncodeAndStoreRunLength(unsigned __int64 runLength) {
  
  ATLTRACE("store run length: %lu\n", runLength);
  if ((fTupelPos & 1) == 0)
    fCountBitsCleared += runLength;
  else
    fCountBitsSet+=runLength;

  ++fTupelPos;
  if (fTupelPos == 4) {
    if (fBufferPos + longestTupelLength > fBufferEndPos) {
      WriteBuffer();
      fTupelPos = 0;
      *fHeaderByte = 0;
    }

    fTupelPos = 0;
    fHeaderByte = fBufferPos++;
    (*fHeaderByte) = 0;
  }

  (*fHeaderByte) >>= 2;
  if (runLength & 0xFFFFFFFF00000000) { // requires 64 bit to store
    memcpy(fBufferPos, &runLength, 8);
    fBufferPos += 8;
    (*fHeaderByte) |= 0xC0; // set highest 2 bits to 11
  } else if (runLength & 0x00000000FFFF0000) { // requires 32 bit to store
    memcpy(fBufferPos, (DWORD*)(&runLength), 4);
    fBufferPos += 4;
    (*fHeaderByte) |= 0x80; // set highest 2 bits to 10
  } else if (runLength & 0x000000000000FF00) { // requires 16 bit to store
    memcpy(fBufferPos, (WORD*)(&runLength), 2);
    fBufferPos += 2;
    (*fHeaderByte) |= 0x40; // set highest 2 bits to 01
  } else { // if (runLength & 0x00000000000000FF) { // requires 8 bit to store
    *fBufferPos = (BYTE) runLength;
    fBufferPos += 1;
    // obsolete (*fHeaderByte) |= 0x00; // set highest 2 bits to 00
  }

}

void CompressedRunLengthStreamWriter::WriteBuffer() {
  DWORD sizeWritten;
  DWORD len = (DWORD)(fBufferPos - fBuffer);
  BOOL ok;

  ok = WriteFile(fFile, fBuffer, len, &sizeWritten, NULL);
  CHECK_OS_EX_PARAM1(ok, EWinException::writeFileError, L"");

  fBufferPos = fBuffer;
  fBufferEndPos = fBufferPos + cBufferLen;
}
  
void CompressedRunLengthStreamWriter::WriteHeaderByte() {
  // encode word length of stored 4 words by using the following encoding

}
