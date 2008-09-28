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
#ifndef CompressionThread_H
#define CompressionThread_H
//---------------------------------------------------------------------------

#include "OdinThread.h"
#include "Compression.h"
//---------------------------------------------------------------------------

class CImageBuffer;
class CBufferChunk;

//---------------------------------------------------------------------------
class CCompressionThread : public COdinThread
{
  public:
   CCompressionThread(TCompressionFormat compressionFormat, 
     CImageBuffer *sourceQueueDecompressed, 
     CImageBuffer *targetQueueDecompressed,
     CImageBuffer *sourceQueueCompressed, 
     CImageBuffer *targetQueueCompressed);

  protected:
    TCompressionFormat fCompressionFormat;
    int fCompressionLevel;
    CImageBuffer *fSourceQueueDecompressed, *fTargetQueueDecompressed;
    CImageBuffer *fSourceQueueCompressed, *fTargetQueueCompressed;

    virtual DWORD Execute();

  private:
    void CompressLoopZlib();
    void CommpressLoopLibz2();

};
//---------------------------------------------------------------------------
#endif
 