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
#ifndef ReadThread_H
#define ReadThread_H

//---------------------------------------------------------------------------
#include "OdinThread.h"

class CImageBuffer;
class IImageStream;
class CBufferChunk;
class CompressedRunLengthStreamReader;

//---------------------------------------------------------------------------

class CReadThread : public COdinThread
{
  public:
    CReadThread(IImageStream *ReadStore, CImageBuffer *sourceQueue,  CImageBuffer *targetQueue, bool verifyOnly);
    virtual DWORD Execute();

    void SetAllocationMapReaderInfo(IRunLengthStreamReader* runLengthReader, DWORD clusterSize);

    void SetVolumeDataOffset(unsigned __int64 volumeDataOffset) {
      fVolumeDataOffset = volumeDataOffset;
    }

protected:
    CImageBuffer *fSourceQueue;
    CImageBuffer *fTargetQueue;
    IImageStream  *fReadStore;
    std::wstring fAllocMapFileName; // name of file where file allocation table map is stored
    unsigned __int64 fVolumeDataOffset;   // offset of volume data in image file
    HANDLE fAllocMapFileHandle;       // handle of file where file allocation table map is stored
    // unsigned __int64 fAllocMapOffset; // offset in file where file allocation table map 
    // unsigned __int64 fAllocMapLen;    // length of file allocation map table
    DWORD fClusterSize;               // size each bit in allocation bitmap represents
    IRunLengthStreamReader* fRunLengthReader; // interface to get run length of (un)allocated clusters
    bool fVerifyOnly;                   // check only checksum of a stored image

    void ReadLoopCombined(void);
    void ReadLoopSimple(void);
    void ReadLoopVerify();
}; 
//---------------------------------------------------------------------------
#endif
