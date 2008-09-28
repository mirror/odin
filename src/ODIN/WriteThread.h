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

#ifndef WriteThread_H
#define WriteThread_H
//---------------------------------------------------------------------------
#include "OdinThread.h"

class CImageBuffer;
class IImageStream;
class CBufferChunk;
class IRunLengthStreamReader;

//---------------------------------------------------------------------------
class CWriteThread : public COdinThread
{
  public:
    CWriteThread(IImageStream *writeStore, CImageBuffer *sourceQueue, CImageBuffer* targetQueue, bool verifyOnly);
    virtual DWORD Execute();
    
    // void SetAllocationMapReaderInfo(HANDLE hFile, unsigned __int64 offBegin, unsigned __int64 length, DWORD clusterSize);
    void SetAllocationMapReaderInfo(IRunLengthStreamReader* runLengthReader, DWORD clusterSize);
 
  protected:
    CImageBuffer *fSourceQueue;
    CImageBuffer *fTargetQueue;
    IImageStream  *fWriteStore;

    // HANDLE fAllocMapFileHandle;       // handle of file where file allocation table map is stored
    //unsigned __int64 fAllocMapOffset; // offset in file where file allocation table map 
    //unsigned __int64 fAllocMapLen;    // length of file allocation map table
    DWORD fClusterSize;               // size each bit in allocation bitmap represents
    IRunLengthStreamReader* fRunLengthReader;
    bool fVerifyOnly;                   // check only checksum of a stored image

  private:
    void WriteLoopRunLength();
    void WriteLoopSimple();
    void WriteLoopVerify();
}; 
//---------------------------------------------------------------------------
#endif
