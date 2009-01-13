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
#include "..\..\src\ODIN\Config.h"

class PartitionInfoMgrTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( PartitionInfoMgrTest );
  CPPUNIT_TEST( ValidFileHeaderTest );
  CPPUNIT_TEST( SupportedVersionTest );
  CPPUNIT_TEST( CreateFileHeaderTest );
  CPPUNIT_TEST( ReadWriteFileHeaderTest );
  CPPUNIT_TEST( ReadWriteFileHeaderExceptionTest );
  CPPUNIT_TEST( ReadWriteDiskTest );
  CPPUNIT_TEST_SUITE_END();

public:
  PartitionInfoMgrTest();
  void setUp();
  void tearDown();

  void CreateFileHeaderTest();
  void ValidFileHeaderTest();
  void SupportedVersionTest();
  void ReadWriteFileHeaderTest();
  void ReadWriteFileHeaderExceptionTest();
  void ReadWriteDiskTest ();
  void ReadDiskTest();

private:
  void TestEquality(BYTE* partInfoBuffer, BYTE* readBuffer);
  BYTE* PreparePartitionInfo();

  std::wstring fFileName;
  DECLARE_SECTION()
  DECLARE_ENTRY(bool, fEnableTest)
  DECLARE_ENTRY(std::wstring, fImageDisk) // path to directory containing sample files to backup

};
