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
#include <string>
#include "IImageStream.h"
#include "BufferQueue.h"
#include "WriteThread.h"
#include "Exception.h"
#include "IRunLengthStreamReader.h"
#include "crc32.h"
#include "InternalException.h"

using namespace std;

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Write thread constructor
//
CWriteThread::CWriteThread(IImageStream *writeStore, CImageBuffer *sourceQueue, CImageBuffer *targetQueue, bool verifyOnly)
  : COdinThread(CREATE_SUSPENDED)
{
  this->fWriteStore  = writeStore;
  fSourceQueue = sourceQueue;
  fTargetQueue = targetQueue;
  fRunLengthReader = NULL;
  fVerifyOnly = verifyOnly;
} 
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Main execute method for the thread.
//
DWORD  CWriteThread:: Execute()
{
  SetName("WriteThread");
  try {
    if (fVerifyOnly) {
      WriteLoopVerify();
    } else {
      if (NULL != fRunLengthReader)
        WriteLoopRunLength(); 
      else
        WriteLoopSimple();
    }
    return S_OK;
  } catch (Exception &e) {
    fErrorFlag = true;
    fErrorMessage = e.GetMessage();
	  fFinished = true;
    return E_FAIL;
  } 
} 

//---------------------------------------------------------------------------

void CWriteThread::WriteLoopRunLength()
{
  unsigned __int64 runLength;
  unsigned __int64 seekPos = 0;
  unsigned __int64 bytesToReadForReadRunLength;
  unsigned bytesToRead, bytesRead, remainingBufferSize, bufferBytesUsed;
  BYTE* buffer;
  CCRC32 crc32;
  unsigned dbgNoUsedClustersTotal = 0;
  unsigned dbgBytesReadRunLength = 0;
  
  // CompressedRunLengthStreamReader allocMapReader(fAllocMapFileHandle, (DWORD) fAllocMapOffset, (DWORD) fAllocMapLen);
  CBufferChunk *chunk = fSourceQueue->GetChunk(); // may block
  buffer = (BYTE*)chunk->GetData();
  bufferBytesUsed = 0;
  if (!chunk)
    THROW_INT_EXC(EInternalException::getChunkError); 

  remainingBufferSize = chunk->GetSize();

  while (!fRunLengthReader->LastValueRead()) {
    // read run length of used clusters
    runLength = fRunLengthReader->GetNextRunLength();
    dbgNoUsedClustersTotal+=(unsigned)runLength;
    ATLTRACE("Write thread: Reading run length of used clusters, size: %d\n", (DWORD) runLength);
    bytesToReadForReadRunLength = fClusterSize * runLength;
    //ATLTRACE("  size in bytes is: %d\n", (DWORD) bytesToReadForReadRunLength);
        
    while (bytesToReadForReadRunLength > 0) {
      if (bytesToReadForReadRunLength  < remainingBufferSize) {
         // actual run length is shorter than what we have in buffer, write rest
         bytesToRead = (unsigned) bytesToReadForReadRunLength;
         runLength = 0;
       } else {
         // actual run length is longer than what we have in buffer, write total buffer
         bytesToRead = remainingBufferSize;
         runLength -= remainingBufferSize / fClusterSize;
       }
       fWriteStore->Write(buffer, bytesToRead, &bytesRead);
       fBytesProcessed += bytesRead;
       if (bytesToRead != bytesRead) {
          THROW_INT_EXC(EInternalException::wrongWriteSize); 
       }
       //ATLTRACE("First Bytes of run length are: %d, %d, %d, %d, %d\n",
       //  (unsigned) buffer[0], (unsigned) buffer[1], (unsigned) buffer[2],(unsigned) buffer[3], (unsigned) buffer[4]);
       crc32.AddDataBlock(buffer, bytesRead);
       seekPos += bytesRead;
       buffer += bytesRead;
       dbgBytesReadRunLength += bytesRead;
       bufferBytesUsed += bytesRead;
       remainingBufferSize -= bytesRead;
       bytesToReadForReadRunLength -= bytesRead;
       chunk->SetSize(bufferBytesUsed);
       if (remainingBufferSize <= 0) {
         // write current block, because it is full and get a new block.
         //ATLTRACE("  Buffer is full, releasing buffer, bytes written: %d\n", (DWORD) fBytesProcessed);
         //ATLTRACE("  write thread CRC32 for this run length is: %u\n", crc32.GetResult());
         bool eof = chunk->IsEOF();
         chunk->Reset();
         fTargetQueue->ReleaseChunk(chunk);
         if (!eof) {
          chunk = fSourceQueue->GetChunk(); // may block
          remainingBufferSize = chunk->GetSize();
          buffer = (BYTE*)chunk->GetData();
          bufferBytesUsed = 0;
         }
       }
    } // inner while
    
    //ATLTRACE("write thread CRC32 for this run length is: %u\n", crc32.GetResult());
    //ATLTRACE("write thread bytes read for this run length is: %u\n", dbgBytesReadRunLength);
    dbgBytesReadRunLength = 0;
    // read run length of free clusters
    runLength = fRunLengthReader->GetNextRunLength();
    ATLTRACE("Skipping run length of free clusters, size: %d\n", (DWORD) runLength);
    //ATLTRACE("  number of total clusters up to now: %u\n", dbgNoUsedClustersTotal);
    seekPos += fClusterSize * runLength; // skip free clusters
    //ATLTRACE("write thread: set seek position: %d\n", (DWORD) seekPos);

    fWriteStore->Seek(seekPos, FILE_BEGIN); // set seek position for write thread
  } // outer while
  ATLTRACE("Write thread: Number of written bytes in total: %u\n", dbgNoUsedClustersTotal * fClusterSize);
  fCrc32 = crc32.GetResult();
  ATLTRACE("Write thread CRC32 is: %u\n", crc32.GetResult());
  fWriteStore->SetCompletedInformation(fCrc32, fBytesProcessed); 
}

//---------------------------------------------------------------------------

void CWriteThread::WriteLoopSimple()
{
  bool bEOF = false;
  // bool bWriteSuccess;
  unsigned nWriteCount;
  unsigned nBytesWritten;
  unsigned dbgRunLength = 0;
  bool bTerminated = FALSE;
  CCRC32 crc32;

  while (!bEOF) {
      CBufferChunk *ReadChunk = fSourceQueue->GetChunk();
      nWriteCount = ReadChunk->IsEmpty() ? 0 : ReadChunk->GetSize();
	    bEOF = ReadChunk->IsEOF();

      fWriteStore->Write(ReadChunk->GetData(), nWriteCount, &nBytesWritten);
      fBytesProcessed += nBytesWritten;
      if (nBytesWritten != nWriteCount) {
        // An error occured while writing - handle this
        THROW_INT_EXC(EInternalException::wrongWriteSize); 
      }  // else if (!nBytesRead)
      dbgRunLength += nBytesWritten;
      crc32.AddDataBlock((BYTE*)(ReadChunk->GetData()), nWriteCount);
      //ATLTRACE("Write thread number of bytes written so far: %u\n", (DWORD) fBytesProcessed);
      //ATLTRACE("Write thread CRC32 so far is: %u\n", crc32.GetResult());
      
      ReadChunk->Reset();
      fTargetQueue->ReleaseChunk(ReadChunk);
    }  // while (!EOF)
    ATLTRACE("Write thread number of bytes written totally: %u\n", (DWORD) fBytesProcessed);
    ATLTRACE("Write thread CRC32 is: %u\n", crc32.GetResult());
    unsigned __int64 fileSeekPos = fWriteStore->GetPosition();
    ATLTRACE("Write thread: final seek position in file is: %u\n", (unsigned) fileSeekPos);
    fCrc32 = crc32.GetResult();
    fWriteStore->SetCompletedInformation(fCrc32, fBytesProcessed); 
}

//---------------------------------------------------------------------------

void CWriteThread::WriteLoopVerify()
{
  bool bEOF = false;
  unsigned nWriteCount;
  unsigned dbgRunLength = 0;
  bool bTerminated = FALSE;
  CCRC32 crc32;

  while (!bEOF) {
      CBufferChunk *ReadChunk = fSourceQueue->GetChunk();
      nWriteCount = ReadChunk->IsEmpty() ? 0 : ReadChunk->GetSize();
	    bEOF = ReadChunk->IsEOF();
      fBytesProcessed += nWriteCount;
      dbgRunLength += nWriteCount;
      crc32.AddDataBlock((BYTE*)(ReadChunk->GetData()), nWriteCount);      
      ReadChunk->Reset();
      fTargetQueue->ReleaseChunk(ReadChunk);
    }  // while (!EOF)
    ATLTRACE("Write thread number of bytes written totally: %u\n", (DWORD) fBytesProcessed);
    fCrc32 = crc32.GetResult();
    ATLTRACE("Write thread CRC32 is: %u\n", fCrc32);
    if (fWriteStore)
      fWriteStore->SetCompletedInformation(fCrc32, fBytesProcessed); 
}

//---------------------------------------------------------------------------

void CWriteThread::SetAllocationMapReaderInfo(IRunLengthStreamReader* runLengthReader, DWORD clusterSize) {
  fRunLengthReader = runLengthReader;
  fClusterSize = clusterSize;
}
