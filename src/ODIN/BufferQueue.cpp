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
#include <list>
#include <sstream>
#include <string>
#include <math.h>
#include "sync.h"
#include "BufferQueue.h"
#include "InternalException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

using namespace std;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// CBufferChunk class
//

//---------------------------------------------------------------------------
// CBufferChunk constructor
//
CBufferChunk::CBufferChunk(int nSize, int nIndex) {

  fData = new BYTE[nSize];
  fMaxSize = nSize;
  fUsedSize = SIZE_NOT_SET;
  fEOF = false;
  fSeekPos = (unsigned __int64) -1;

}  
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// CBufferChunk destructor
//
CBufferChunk::~CBufferChunk() {
  delete [] fData;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// CImageBuffer class
//
// A thread safe queue managing memory chunks

//---------------------------------------------------------------------------
// CImageBuffer constructor
//
CImageBuffer::CImageBuffer(LPCWSTR name) {
  if (name)
    fName = name;
  fChunkCount = 0;
  fSemaListHasElems.Create(NULL, 0, 9999, NULL);
}

CImageBuffer::CImageBuffer(int size, int count, LPCWSTR name) {

  if (name)
    fName = name;
  fChunkCount = count;
  // Create all the buffer chunks
  for (unsigned n = 0; n < fChunkCount; n++) {
    fChunks.push_back( new CBufferChunk(size, n));
  }  // for (unsigned n = 0; n < nChunkCount; n++)
  fSemaListHasElems.Create(NULL, count, count, NULL);
}

//---------------------------------------------------------------------------
// Destructor - free up all the resources please
//
 CImageBuffer::~CImageBuffer()
{
  fCritSec.Enter();
  list <CBufferChunk*>::iterator it = fChunks.begin();

  while(it != fChunks.end())
	{
		delete *it;
		it++;
	}
  fChunks.clear();
  fCritSec.Leave();
}  //  CImageBuffer::~CImageBuffer()
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Request a chunk, if no chunk is available, wait until a chunk is available
//
CBufferChunk *  CImageBuffer::GetChunk() 
{
  CBufferChunk *chunk = NULL;
  //ATLTRACE("CImageBuffer::GetChunk() begin, thread: %d, name: %S, size is: %d\n", GetCurrentThreadId(), fName.c_str(), fChunks.size());
  DWORD res = WaitForSingleObject(fSemaListHasElems.m_h, 1000000);
  if ( res == WAIT_TIMEOUT)
    THROW_INT_EXC(EInternalException::threadSyncTimeout);
  fCritSec.Enter();
  chunk = fChunks.front();
  fChunks.pop_front();
  fCritSec.Leave();
  //ATLTRACE("CImageBuffer::GetChunk() end,  thread: %d, name: %S, size is: %d\n", GetCurrentThreadId(), fName.c_str(), fChunks.size());
  return chunk;
} 
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Release a chunk
//
void  CImageBuffer::ReleaseChunk(CBufferChunk *chunk) 
{
  //ATLTRACE("CImageBuffer::ReleaseChunk() begin,  thread: %d, name: %S, size is: %d\n", GetCurrentThreadId(), fName.c_str(), fChunks.size());
  fCritSec.Enter();
  fChunks.push_back(chunk);
  fCritSec.Leave();
  fSemaListHasElems.Release(); 
  //ATLTRACE("CImageBuffer::ReleaseChunk() end,  thread: %d, name: %S, size is: %d\n", GetCurrentThreadId(), fName.c_str(), fChunks.size());
}
//---------------------------------------------------------------------------

