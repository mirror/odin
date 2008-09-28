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
#ifndef __IRUNLENGTHSTREAMREADER_H__
#define __IRUNLENGTHSTREAMREADER_H__

// This interface is used to define the abstraction how the reader or writer of a volume get 
// the run length values of used or unsed clusters of a volume. Run Lengths are always read 
//  sequential. Therefore we need only two methods to read the next value and to check the end.
class IRunLengthStreamReader
{
public:
  // get next run length (how many adjacent cluster of a volume are used/unused). The method
  // starts with used clusters, then unused, then used and so on.
  virtual unsigned __int64 GetNextRunLength() = 0;

  // check if the end of all available run length values has been reached.
  virtual bool LastValueRead() = 0;
};

#endif