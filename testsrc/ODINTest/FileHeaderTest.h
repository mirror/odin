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

#include "cppunit/extensions/HelperMacros.h"
#include "..\..\src\ODIN\FileHeader.h"

class FileHeaderTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( FileHeaderTest );
  CPPUNIT_TEST( WriteHeaderTest );
  CPPUNIT_TEST( ReadHeaderTest );
  CPPUNIT_TEST( ValidFileHeaderTest );
  CPPUNIT_TEST( SupportedVersionTest );
  CPPUNIT_TEST( SupportedChecksumMethodTest );
  CPPUNIT_TEST( SupportedCompressionFormatTest );
  CPPUNIT_TEST( SupportedVolumeEncodingFormatTest) ;
CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void WriteHeaderTest();
  void ReadHeaderTest();
  void ValidFileHeaderTest();
  void SupportedVersionTest();
  void SupportedChecksumMethodTest();
  void SupportedCompressionFormatTest();
  void SupportedVolumeEncodingFormatTest();

private:
  unsigned Read(void * buffer, unsigned nLength);
  unsigned Write(void *buffer, unsigned nLength);
  unsigned __int64 Seek(__int64 Offset, DWORD MoveMethod);
  void WriteAllocationBitmap();
  void WriteComment();
  void WriteCrc32Checksum(DWORD crc32, unsigned __int64 offset, unsigned length);
  void WriteVolumeData();
  void ReadAndCheckBuffer(unsigned __int64 offset, unsigned length, BYTE* reference, unsigned refLength);

  // variables:
  CImageFileHeader fImageHeader;
  HANDLE fHandle;
  static int sCounter;

};
