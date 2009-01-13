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
#include "FileFormatException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

// error message strings (should later be moved to a separate non compiled file)
LPCWSTR EFileFormatException::sMessages[] = {
  L"The file header indicates an unsupported file format", //  magicByteError
  L"The offset in the file header has an illegal value", // wrongFileOffsetError
  L"The length of the comment in the file header is wrong", // wrongCommentLength
  L"The length of the checksum in the file header is wrong", // wrongChecksumLength
  L"The version of this file format is not supported", // majorVersionError
  L"The method to calculate the checksum is unknown", // wrongChecksumMethod, 
  L"The compression method is unknown", // wrongCompressionMethod,
  L"The method to store information about cluster usage is unknown", // wrongVolumeEncodingMethod,
  L"The file has an unexpected file size.", // wrongFileSizeError
};


void EFileFormatException::BuildMessageString()
{
  fMessage += sMessages[fErrorCode];
}

