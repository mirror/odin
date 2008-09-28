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
#include "../zlib.1.2.3/zlib.h"
#include "../bzip2-1.0.5/bzlib.h"
#include "Compression.h"
#include <string>
#include "BufferQueue.h"
#include "CompressionThread.h"
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
 CCompressionThread::CCompressionThread(TCompressionFormat compressionFormat, 
   CImageBuffer *sourceQueueDecompressed, 
   CImageBuffer *targetQueueDecompressed,
   CImageBuffer *sourceQueueCompressed, 
   CImageBuffer *targetQueueCompressed
   ) : COdinThread(CREATE_SUSPENDED)      
{
  wstring ResourceName;

  this->fCompressionFormat = compressionFormat;
  fSourceQueueDecompressed = sourceQueueDecompressed;
  fTargetQueueDecompressed = targetQueueDecompressed;
  fSourceQueueCompressed = sourceQueueCompressed;
  fTargetQueueCompressed = targetQueueCompressed;
  fCompressionLevel = 6;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// The thread's main execution loop - keep this as simple as possible
//
DWORD CCompressionThread::Execute()
{
  SetName("CompressionThread");

  try {
    if (fCompressionFormat == compressionGZip)
      CompressLoopZlib();
    else
      CommpressLoopLibz2();

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

void  CCompressionThread::CompressLoopZlib()
{
  bool bEOF = false;
  z_stream  zStream;
  int flush, ret = Z_OK;
  CBufferChunk *readChunk = NULL;
  CBufferChunk *compressChunk = NULL;

  memset(&zStream, 0, sizeof(zStream));
  ret = deflateInit(&zStream, Z_DEFAULT_COMPRESSION);
  if (ret != BZ_OK) {
    THROWEX(EZLibCompressionException, ret);
  }

  while (ret != Z_STREAM_END) {
    if (zStream.avail_in == 0 && !bEOF) {
      if (readChunk)
        fTargetQueueDecompressed->ReleaseChunk(readChunk);
      readChunk = fSourceQueueDecompressed->GetChunk(); // may block
      bEOF = readChunk->IsEOF();
      zStream.next_in = (BYTE*)readChunk->GetData();
      zStream.avail_in = readChunk->GetSize();
    }

    if (zStream.avail_out == 0) {
      if (compressChunk) {
        compressChunk->SetSize(compressChunk->GetMaxSize());
        fTargetQueueCompressed->ReleaseChunk(compressChunk);
      }
      compressChunk = fSourceQueueCompressed->GetChunk();
      zStream.next_out = (BYTE*)compressChunk->GetData();
      zStream.avail_out = compressChunk->GetMaxSize();
    }

    flush = bEOF ? Z_FINISH : Z_NO_FLUSH;
    if (zStream.next_in > 0) {
      ret = deflate(&zStream, flush); 
      if (ret < 0 ) {
        ATLTRACE("Error in gzip compressing data, error code: %d\n", ret);
        THROWEX(EZLibCompressionException, ret);
      }
    }
  } 
  
  ret = deflateEnd(&zStream);
  if (ret < 0) {
    ATLTRACE("Error in termination of gzip compression, error code: %d\n", ret);
    THROWEX(EZLibCompressionException, ret);
  }

  if (readChunk != NULL)
    fTargetQueueDecompressed->ReleaseChunk(readChunk);
  if (compressChunk != NULL) {
    compressChunk->SetSize (compressChunk->GetMaxSize() - zStream.avail_out); 
    compressChunk->SetEOF(true);
    fTargetQueueCompressed->ReleaseChunk(compressChunk); 
  }
}

void CCompressionThread::CommpressLoopLibz2()
{
  bool bEOF = false;
  bz_stream  bzsStream;
  int action, ret = Z_OK;
  CBufferChunk *readChunk = NULL;
  CBufferChunk *compressChunk = NULL;

  memset(&bzsStream, 0, sizeof(bzsStream));
  ret = BZ2_bzCompressInit(&bzsStream, 9, 0, 0);
  if (ret != BZ_OK)
     THROWEX(EBZip2CompressionException, ret);

  while (ret != BZ_STREAM_END ) {
    if (bzsStream.avail_in == 0 && !bEOF) {
      if (readChunk)
        fTargetQueueDecompressed->ReleaseChunk(readChunk);
      readChunk = fSourceQueueDecompressed->GetChunk(); // may block
      bEOF = readChunk->IsEOF();
      bzsStream.next_in = (char*)readChunk->GetData();
      bzsStream.avail_in = readChunk->GetSize();
    }

    if (bzsStream.avail_out == 0) {
      if (compressChunk) {
        compressChunk->SetSize(compressChunk->GetMaxSize());// (zsStream.total_out);
        fTargetQueueCompressed->ReleaseChunk(compressChunk);
      }
      compressChunk = fSourceQueueCompressed->GetChunk();
      bzsStream.next_out = (char*)compressChunk->GetData();
      bzsStream.avail_out = compressChunk->GetMaxSize();
    }

    action = bEOF ? BZ_FINISH : BZ_RUN;
    if (bzsStream.next_in > 0) {
      ret = BZ2_bzCompress(&bzsStream, action);
      if (ret < 0) {
        ATLTRACE("Error in bzip2 compressing data, error code: %d\n", ret);
        THROWEX(EBZip2CompressionException, ret);
      }
    }
  } 
  
  ret = BZ2_bzCompressEnd(&bzsStream);
  if (ret < 0) {
    ATLTRACE("Error in termination of bzip2 compression, error code: %d\n", ret);
    THROWEX(EBZip2CompressionException, ret);
  }

  if (readChunk != NULL)
    fTargetQueueDecompressed->ReleaseChunk(readChunk);
  if (compressChunk != NULL) {
    compressChunk->SetSize (compressChunk->GetMaxSize() - bzsStream.avail_out);
    compressChunk->SetEOF(true);
    fTargetQueueCompressed->ReleaseChunk(compressChunk); 
  }
}
