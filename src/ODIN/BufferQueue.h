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
 
//----------------------------  $NoKeywords ---------------------------------
#pragma once
#ifndef BufferQueue_H
#define BufferQueue_H

//---------------------------------------------------------------------------

#include <list>
#include "sync.h"

//---------------------------------------------------------------------------
#define SIZE_NOT_SET 0xFFFFFFFF
//---------------------------------------------------------------------------
// CBufferChunk class - each instance of this class is one of a group of
// buffer chunks.  These chunks hold data as they pass from the read to the
// write threads.  Allocating chunks is done through the main CBufferQueue
// class.
// Memory used in these chunks is guaranteed to be alligned on a page
// boundary - thus on x86 it will be a multiple of any known disk sector
// size (as required when unbuffered file I/O is performed).
//
enum TBufferChunkState {csFree, csInUse};

class CBufferChunk {
  public:
    CBufferChunk(int nSize, int nIndex);
    ~CBufferChunk();
    
	void inline  Reset(void) { 
		fEOF = false; 
		fUsedSize = SIZE_NOT_SET; 
    fSeekPos = (unsigned __int64) -1;
	}

    unsigned GetMaxSize() {
		return fMaxSize;
	}

	void SetSize(unsigned newSize) {
		fUsedSize = newSize;
	}
	
	bool IsEOF() {
		return fEOF;
	}
	
	void SetEOF (bool eof) {
		fEOF = eof;
	}

    void *GetData() {
		return fData;
	}

  unsigned GetSize() { 
		return (fUsedSize!=SIZE_NOT_SET) ? fUsedSize : fMaxSize; 
	}

  void SetSeekPos(unsigned __int64 seekPos) { 
		fSeekPos = seekPos; 
	}

  unsigned __int64 GetSeekPos() { 
		return fSeekPos; 
	}

  bool HasSeekPos() {
    return fSeekPos != (unsigned __int64) -1;
  }

  bool IsEmpty() const {
    return fUsedSize == SIZE_NOT_SET;
  }
private:
    unsigned fMaxSize;
    unsigned fUsedSize;
    bool fEOF;
    BYTE *fData;
    unsigned __int64 fSeekPos;

};  // class CBufferChunk
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// CImageBuffer class - this class acts as a buffer in between two threads
//

class CSWMRG;
namespace ATL
{
  class CEvent;
};

using namespace ATL;
class CImageBuffer {
  public:
     CImageBuffer(LPCWSTR name=NULL);
     CImageBuffer(int ChunkSize, int ChunkCount, LPCWSTR name=NULL);
     ~CImageBuffer();

    CBufferChunk* GetChunk();

    void ReleaseChunk(CBufferChunk *Chunk);

  private:
    std::list<CBufferChunk*> fChunks;
    unsigned fChunkCount;         // Must be power of two
    CCriticalSection fCritSec;
    CSemaphore fSemaListHasElems;
    std::wstring fName;
};  // class CImageBuffer
//---------------------------------------------------------------------------
#endif
