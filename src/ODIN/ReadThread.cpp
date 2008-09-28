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
#include "ReadThread.h"
#include "IRunLengthStreamReader.h"
#include "crc32.h"
#include "Exception.h"
#include "InternalException.h"

using namespace std;

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//
 CReadThread::CReadThread(IImageStream *fReadStore, CImageBuffer *sourceQueue,  CImageBuffer *targetQueue, bool verifyOnly)
   : COdinThread(CREATE_SUSPENDED )
{
  this->fReadStore   = fReadStore;
  this->fSourceQueue = sourceQueue;
  this->fTargetQueue = targetQueue;
  //fAllocMapOffset = 0;
  //fAllocMapLen = 0;
  fVolumeDataOffset = 0;
  fRunLengthReader = NULL;
  fVerifyOnly = verifyOnly;
} 

//---------------------------------------------------------------------------
// Off with the thread's head!  Execute time - read the image.
// We support three different input types now - normal input for files and
// partitions, gzip chunk input, and bzip2 chunk input.  Since we want to
// keep each read loop as tight as possible, we have different read loops
// for each type.  We decide here which one to use and send control there.
//
DWORD CReadThread::Execute()
{
  SetName("ReadThread");
  ATLTRACE("CReadThread created,  thread: %d, name: Read-Thread\n", GetCurrentThreadId());
  try {
    if (fVerifyOnly) {
      ReadLoopVerify();
    } else {
      if (NULL == fRunLengthReader)
        ReadLoopSimple();
      else
        ReadLoopCombined(); 
    }
    fFinished = true;
    return S_OK;
  } catch (Exception &e) {
    fErrorFlag = true;
    fErrorMessage = e.GetMessage();
	  fFinished = true;
    return E_FAIL;
  } 
} 

//---------------------------------------------------------------------------

// A modified read loop that uses the stored run lengths to only store clusters 
// that are in use. This function can only be used if we read from a partition.
// the cluster allocation table is read from the beginning of the file.
void  CReadThread::ReadLoopCombined(void) // ReadLoopFromPartition(void)
{
  unsigned __int64 runLength;
  unsigned __int64 seekPos = 0;
  unsigned __int64 bytesToReadForReadRunLength;
  unsigned bytesToRead, bytesRead, remainingBufferSize, bufferBytesUsed;
  BYTE* buffer;
  CCRC32 crc32, crc32Source;
  unsigned dbgNoUsedClustersTotal = 0;
  unsigned dbgBytesReadRunLength = 0;
  // CompressedRunLengthStreamReader allocMapReader(fAllocMapFileName.c_str(), (DWORD) fAllocMapOffset, (DWORD) fAllocMapLen);
  ATLASSERT( fRunLengthReader != NULL);
  CBufferChunk *writeChunk = fSourceQueue->GetChunk(); // may block
  buffer = (BYTE*)writeChunk->GetData();
  bufferBytesUsed = 0;
  if (!writeChunk)
    THROW_INT_EXC(EInternalException::getChunkError); 

  remainingBufferSize = writeChunk->GetMaxSize();
  if (!fReadStore->IsDrive()) {
    fReadStore->Seek(fVolumeDataOffset, FILE_BEGIN);
  }

  while (!fRunLengthReader->LastValueRead()) {
    // read run length of used clusters
    runLength = fRunLengthReader->GetNextRunLength();
    dbgNoUsedClustersTotal+=(unsigned)runLength;
    ATLTRACE("Reading run length of used clusters, size: %d\n", (DWORD) runLength);
    bytesToReadForReadRunLength = fClusterSize * runLength;
    //ATLTRACE("  size in bytes is: %d\n", (DWORD) bytesToReadForReadRunLength);
        
    while (bytesToReadForReadRunLength > 0) {

      if (bytesToReadForReadRunLength  < remainingBufferSize) {
         bytesToRead = (unsigned) bytesToReadForReadRunLength;
         runLength = 0;
       } else {
         bytesToRead = remainingBufferSize;
         runLength -= remainingBufferSize / fClusterSize;
       }
       fReadStore->Read(buffer, bytesToRead, &bytesRead);
       if (bytesToRead != bytesRead) {
         THROW_INT_EXC(EInternalException::wrongReadSize); 
       }
       //ATLTRACE("First Bytes of run length are: %d, %d, %d, %d, %d\n",
       //  (unsigned) buffer[0], (unsigned) buffer[1], (unsigned) buffer[2],(unsigned) buffer[3], (unsigned) buffer[4]);
       crc32.AddDataBlock(buffer, bytesRead);
       seekPos += bytesRead;
       buffer += bytesRead;
       bufferBytesUsed += bytesRead;
       remainingBufferSize -= bytesRead;
       bytesToReadForReadRunLength -= bytesRead;
       writeChunk->SetSize(bufferBytesUsed);
       fBytesProcessed += bytesRead;
       //if (bytesToReadForReadRunLength == 0) {
       //  ATLTRACE("last part read!!\n");
       //}
       if (remainingBufferSize <= 0) {
         // release current block, because it is full and get a new block.
         //ATLTRACE("Buffer is full, releasing buffer, bytes written: %d\n", (DWORD) dbgBytesReadTotal);
         fTargetQueue->ReleaseChunk(writeChunk);
         writeChunk = fSourceQueue->GetChunk(); // may block
         remainingBufferSize = writeChunk->GetSize();
         buffer = (BYTE*)writeChunk->GetData();
         bufferBytesUsed = 0;
         //ATLTRACE("  Read thread: Number of read bytes so far: %u\n", fBytesProcessed);
         //ATLTRACE("  Read thread CRC32 is: %u\n", crc32.GetResult());
       }
    } // inner while
    
    //ATLTRACE("  read thread CRC32 for this run length is: %u\n", crc32.GetResult());
    dbgBytesReadRunLength = 0;
    // read run length of free clusters
    runLength = fRunLengthReader->GetNextRunLength();
    ATLTRACE("Skipping run length of free clusters, size: %d\n", (DWORD) runLength);
    //ATLTRACE("  number of total clusters up to now: %u\n", dbgNoUsedClustersTotal);
    seekPos += fClusterSize * runLength; // skip free clusters
    // ATLTRACE("read thread: set seek position: %d\n", (DWORD) seekPos);

    if (fReadStore->IsDrive()) { 
      fReadStore->Seek(seekPos, FILE_BEGIN);
    } 
  } // outer while

  writeChunk->SetEOF(true);  
  ATLTRACE("Number of read bytes in total: %u\n", dbgNoUsedClustersTotal * fClusterSize);
  fTargetQueue->ReleaseChunk(writeChunk);
  ATLTRACE("Read thread: Number of read bytes in total: %u\n", fBytesProcessed);
  fCrc32 = crc32.GetResult();
  ATLTRACE("Read thread CRC32 is: %u\n", fCrc32);
}

//---------------------------------------------------------------------------
// Simple read loop reading the complete input buffer after buffer
void  CReadThread::ReadLoopSimple(void)
{
  bool bEOF = false;
  bool bSkipUnallocated = true; 
  unsigned nBytesRead;
  CCRC32 crc32;
  if (!fReadStore->IsDrive()) {
    fReadStore->Seek(fVolumeDataOffset, FILE_BEGIN);
  }

  while (!bEOF ) {
    CBufferChunk *chunk = fSourceQueue->GetChunk(); // may block
    fReadStore->Read(chunk->GetData(), chunk->GetSize(), &nBytesRead);

    // If we didn't get as much data as we expected, then we're at the end of the file.  Set the EOF marker
    // in the buffer chunk so the write thread knows this is the last.
    if (chunk->GetSize() != nBytesRead) {
	    chunk->SetEOF(true);
      bEOF = true;
    }  
    chunk->SetSize(nBytesRead);
    crc32.AddDataBlock((BYTE*)(chunk->GetData()), nBytesRead);
    fBytesProcessed += nBytesRead;
    //ATLTRACE("  Read thread: Number of read bytes for current block: %u\n", fBytesProcessed);  
    //ATLTRACE("  Read thread CRC32 for this block is: %u\n", crc32.GetResult());
    fTargetQueue->ReleaseChunk(chunk);
  } 
  fCrc32 = crc32.GetResult();
  ATLTRACE("Read thread: Number of read bytes in total: %u\n", fBytesProcessed);  
  ATLTRACE("Read thread CRC32 is: %u\n", fCrc32);
}

//---------------------------------------------------------------------------

void CReadThread::ReadLoopVerify()
{
  ReadLoopSimple(); // we do nothing but a simple read
}

//---------------------------------------------------------------------------

void CReadThread::SetAllocationMapReaderInfo(IRunLengthStreamReader* runLengthReader, DWORD clusterSize) {
  fRunLengthReader = runLengthReader;
  fClusterSize = clusterSize;
}

