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
#include "BufferQueue.h"
#include "DecompressionThread.h"
#include "../zlib.1.2.3/zlib.h"
#include "../bzip2-1.0.5/bzlib.h"
#include "CompressionException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

using namespace std;
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Constructor
//
 CDecompressionThread::CDecompressionThread(TCompressionFormat compressionFormat, 
      CImageBuffer *sourceQueueCompressed, 
      CImageBuffer *targetQueueCompressed,
      CImageBuffer *sourceQueueDecompressed, 
      CImageBuffer *targetQueueDecompressed
      ) : COdinThread(CREATE_SUSPENDED)
{
  fSourceQueueCompressed = sourceQueueCompressed;
  fTargetQueueCompressed = targetQueueCompressed;
  fSourceQueueDecompressed = sourceQueueDecompressed;
  fTargetQueueDecompressed = targetQueueDecompressed;
  this->fCompressionFormat = compressionFormat;
}
//---------------------------------------------------------------------------


DWORD  CDecompressionThread::Execute()
{
  ATLTRACE("CDecompressionThread created,  thread: %d, name: Decompression-Thread\n", GetCurrentThreadId());
  SetName("DecompressionThread");
  try {
    if (compressionGZip == fCompressionFormat)
      DecompressLoopZlib();
    else
      DecompressLoopLibz2();

    fFinished = true;
  } catch (Exception &e) {
    fErrorFlag = true;
    fErrorMessage = L"Compression thread encountered exception: \"";
  	fErrorMessage += e.GetMessage();
	  fErrorMessage += L"\"";
    fFinished = true;
	  return -1;
  }
  return 0;
}

//---------------------------------------------------------------------------

void CDecompressionThread::DecompressLoopZlib()
{
  bool bEOF = false;
  z_stream  zsStream;
  int ret = Z_OK;
  CBufferChunk *readChunk = NULL;
  CBufferChunk *decompressChunk = NULL;

  memset(&zsStream, 0, sizeof(zsStream));
  ret = inflateInit(&zsStream);
  if (ret != Z_OK)
     THROWEX(EZLibCompressionException, ret);

  while (ret != Z_STREAM_END) {
    if (zsStream.avail_in == 0) {
      if (readChunk)
        fTargetQueueCompressed->ReleaseChunk(readChunk);
      readChunk = fSourceQueueCompressed->GetChunk(); // may block
      bEOF = readChunk->IsEOF();
      zsStream.next_in = (BYTE*)readChunk->GetData();
      zsStream.avail_in = readChunk->GetSize();
    }

    if (zsStream.avail_out == 0) {
      if (decompressChunk) {
        decompressChunk->SetSize(decompressChunk->GetMaxSize());// (zsStream.total_out);
        fTargetQueueDecompressed->ReleaseChunk(decompressChunk);
      }
      decompressChunk = fSourceQueueDecompressed->GetChunk();
      zsStream.next_out = (BYTE*)decompressChunk->GetData();
      zsStream.avail_out = decompressChunk->GetMaxSize();
    }

    ret = inflate(&zsStream, Z_NO_FLUSH); 
    if (ret < 0) {
      ATLTRACE("Error in gzip compressing data, error code: %d\n", ret);
      THROWEX(EZLibCompressionException, ret);
    }

    if (fCancel) {
      if (decompressChunk)
        fTargetQueueCompressed->ReleaseChunk(decompressChunk);
      if (readChunk)
        fTargetQueueDecompressed->ReleaseChunk(readChunk);
      inflateEnd(&zsStream);
      Terminate(-1);  // terminate thread after releasing buffer and before acquiring next one
    }
  } 
  
  ret = inflateEnd(&zsStream);
  if (ret < 0) {
    ATLTRACE("Error in termination of gzip compression, error code: %d\n", ret);
    THROWEX(EZLibCompressionException, ret);
  }

  if (readChunk != NULL)
    fTargetQueueCompressed->ReleaseChunk(readChunk);
  if (decompressChunk != NULL) {
    decompressChunk->SetSize (decompressChunk->GetMaxSize() - zsStream.avail_out); // zsStream.total_out);
    decompressChunk->SetEOF(true);
    fTargetQueueDecompressed->ReleaseChunk(decompressChunk); 
  }
}

void CDecompressionThread::DecompressLoopLibz2()
{
  bool bEOF = false;
  bz_stream  bzStream;
  int ret = Z_OK;
  CBufferChunk *readChunk = NULL;
  CBufferChunk *decompressChunk = NULL;

  memset(&bzStream, 0, sizeof(bzStream));
  ret = BZ2_bzDecompressInit(&bzStream, 0, 0);
  if (ret != BZ_OK)
    THROWEX(EBZip2CompressionException, ret);

  while (ret != BZ_STREAM_END ) {
    if (bzStream.avail_in == 0) {
      if (readChunk)
        fTargetQueueCompressed->ReleaseChunk(readChunk);
      readChunk = fSourceQueueCompressed->GetChunk(); // may block
      bEOF = readChunk->IsEOF();
      bzStream.next_in = (char*)readChunk->GetData();
      bzStream.avail_in = readChunk->GetSize();
    }

    if (bzStream.avail_out == 0) {
      if (decompressChunk) {
        decompressChunk->SetSize(decompressChunk->GetMaxSize());// (bzStream.total_out);
        fTargetQueueDecompressed->ReleaseChunk(decompressChunk);
      }
      decompressChunk = fSourceQueueDecompressed->GetChunk();
      bzStream.next_out = (char*)decompressChunk->GetData();
      bzStream.avail_out = decompressChunk->GetMaxSize();
    }

   ret = BZ2_bzDecompress(&bzStream); 
   if (ret < 0)
     THROWEX(EBZip2CompressionException, ret);

    if (fCancel) {
      BZ2_bzDecompressEnd(&bzStream);
      if (decompressChunk)
        fTargetQueueCompressed->ReleaseChunk(decompressChunk);
      if (readChunk)
        fTargetQueueDecompressed->ReleaseChunk(readChunk);
      Terminate(-1);  // terminate thread after releasing buffer and before acquiring next one
    }
  } 
  
  ret = BZ2_bzDecompressEnd(&bzStream);
  if (ret < 0) {
    ATLTRACE("Error in termination of bzip2 decompression, error code: %d\n", ret);
    THROWEX(EBZip2CompressionException, ret);
  }

  if (readChunk != NULL)
    fTargetQueueCompressed->ReleaseChunk(readChunk);
  if (decompressChunk != NULL) {
    decompressChunk->SetSize (decompressChunk->GetMaxSize() - bzStream.avail_out); // bzStream.total_out);
    decompressChunk->SetEOF(true);
    fTargetQueueDecompressed->ReleaseChunk(decompressChunk); 
  }
}


