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
#ifndef __RUNLENGTHSTREAMREADER_H__
#define __RUNLENGTHSTREAMREADER_H__

#include "..\..\src\ODIN\IRunLengthStreamReader.h"

// This class is a simple simulator of a runlength stream reader
// It is constructed with a list of run length valuse and just
// returns value by value of this list in the interface.

class CRunLengthStreamReaderSimulator : public IRunLengthStreamReader {

public:
  CRunLengthStreamReaderSimulator(int* runLength, int length);
  virtual ~CRunLengthStreamReaderSimulator();

  virtual unsigned __int64 GetNextRunLength();
  virtual bool LastValueRead();

private:
  int fIndex;
  int fLength;
  int* fLengthValues;
};

#endif
