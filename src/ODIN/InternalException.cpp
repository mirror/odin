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
#include "InternalException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

// error message strings (should later be moved to a separate non compiled file)
LPCWSTR EInternalException::sMessages[] = {
  L"Internal Error in retrieving volume bitmap, wrong buffer size", //  volumeBitmapBufferSizeError
  L"Could not read group descriptors for linux EXT2 partition", // ext2GroupDescrReadError
  L"Could not read EXT2 block group allocation bitmap", // ext2BlockGroupReadError
  L"Internal thread synchronization timeout", // threadSyncTimeout
  L"Input Type not set", // inputTypeNotSet, 
  L"Output Type not set", // outputTypeNotSet
  L"Failed to get memory chunk from buffer queue", // getChunkError
  L"Failed to put memory chunk into buffer queue", // writeChunkError
  L"Failed to read correct number of bytes from buffer", // wrongReadSize
  L"Failed to write correct number of bytes to buffer", // wrongWriteSize
  L"Internal overflow of fixed sized string table", // internalStringTableOverflow
  L"The file header information for this volume is too big for the file chunk size. Please use a bigger chunk size.", //chunkSizeTooSmall
  L"The maximum number of supported partitions of a hard disk is exceeded (internal buffer too small),", //maxPartitionNumberExceeded
  L"The partition format of this drive is not supported (only MBR format, no EFI)", // unsupportedPartitionFormat
};


void EInternalException::BuildMessageString()
{
  fMessage += sMessages[fErrorCode];
}
