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
#include "FileHeader.h"
#include "OSException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

using namespace std;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// class CImageFileHeader
// class for definition and manipulation of a common image file header

// {1D4D7B73-FA01-40e1-B094-5267D8FA0BE7}
const GUID CImageFileHeader::sMagicFileHeaderGUID = 
  { 0x1d4d7b73, 0xfa01, 0x40e1, { 0xb0, 0x94, 0x52, 0x67, 0xd8, 0xfa, 0xb, 0xe7 } };
const WORD CImageFileHeader::sVerMajor = 1;
const WORD CImageFileHeader::sVerMinor = 0;

CImageFileHeader::CImageFileHeader()
{
  memset(&fHeader, 0, sizeof(fHeader));
  fHeader.guid = sMagicFileHeaderGUID;
  fHeader.versionMajor = sVerMajor;
  fHeader.versionMinor = sVerMinor;
  fHeader.compressionScheme = noVolumeBitmap;
  fHeader.verifyScheme = verifyNone;
}

void CImageFileHeader::WriteHeaderToFile(HANDLE hFileOut)
{    
  DWORD sizeWritten;
  LARGE_INTEGER pos, curPos;
  pos.QuadPart = 0LL;
  BOOL ok = SetFilePointerEx(hFileOut, pos, &curPos, FILE_CURRENT);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, L"");
  ok = SetFilePointerEx(hFileOut, pos, NULL, FILE_BEGIN);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, L"");
  ok = WriteFile(hFileOut, &fHeader, sizeof(fHeader), &sizeWritten, NULL);
  CHECK_OS_EX_PARAM1(ok, EWinException::writeFileError, L"");
  ok = SetFilePointerEx(hFileOut, curPos, NULL, FILE_BEGIN);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, L"");
}

void CImageFileHeader::ReadHeaderFromFile(HANDLE hFileIn)
{
  DWORD sizeRead;
  LARGE_INTEGER pos, curPos;
  pos.QuadPart = 0LL;
  BOOL ok = SetFilePointerEx(hFileIn, pos, &curPos, FILE_BEGIN);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, L"");
  ok = ReadFile(hFileIn, &fHeader, sizeof(fHeader), &sizeRead, NULL);
  CHECK_OS_EX_PARAM1(ok, EWinException::readFileError, L"");
  ok = SetFilePointerEx(hFileIn, curPos, NULL, FILE_BEGIN);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, L"");
}

bool CImageFileHeader::IsValidFileHeader() const {
  return fHeader.guid == sMagicFileHeaderGUID ? true : false;
}

bool CImageFileHeader::IsSupportedVersion() const {
  return fHeader.versionMajor == sVerMajor; // we assume that all 1.x versions are supported
}

bool CImageFileHeader::IsSupportedChecksumMethod() const {
  return fHeader.verifyScheme >= verifyNone && fHeader.verifyScheme <= verifyCRC32;
}

bool CImageFileHeader::IsSupportedCompressionFormat() const {
  return fHeader.compressionScheme >= noCompression && fHeader.compressionScheme <= compressionBZIP2;
}

bool CImageFileHeader::IsSupportedVolumeEncodingFormat() const {
  return fHeader.volumeBitmapEncodingScheme >= noVolumeBitmap && fHeader.volumeBitmapEncodingScheme <= simpleCompressedRunLength;
}

void CImageFileHeader::SetVolumeBitmapInfo(VolumeEncodingFormat format, unsigned __int64 offset, unsigned __int64 length)
{
  fHeader.volumeBitmapEncodingScheme = format;
  fHeader.volumeBitmapOffset = offset; 
  fHeader.volumeBitmapLength = length;
}

void CImageFileHeader::SetImageFileVerificationInfo(VerifyFormat format, DWORD length)
{
  fHeader.verifyScheme = format;
  fHeader.verifyLength = length;
  fHeader.verifyOffset = sizeof(fHeader) + fHeader.volumeBitmapLength ; 
    // Note: this assumes that verifcation info is written after cluster bitmap
}

void CImageFileHeader::SetCompressionFormat(TCompressionFormat compFormat)
{
  fHeader.compressionScheme = compFormat;
}

void CImageFileHeader::SetVolumeSize(unsigned __int64 volSize)
{
  fHeader.volumeSize = volSize;
}

