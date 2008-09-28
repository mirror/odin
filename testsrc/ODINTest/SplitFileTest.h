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

class CImageBuffer;

class SplitFileTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( SplitFileTest );
  /**/
  CPPUNIT_TEST( saveSplitFileTest );
  CPPUNIT_TEST( restoreSplitFileTest );
  CPPUNIT_TEST( askForUserFileTest );
  CPPUNIT_TEST( seekSplitFileTest );
  CPPUNIT_TEST( deleteSplitFileTest );
  /**/
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void saveSplitFileTest();
  void restoreSplitFileTest();
  void askForUserFileTest();
  void seekSplitFileTest();
  void deleteSplitFileTest();

private:
  void WaitUntilDone(HANDLE* threadHandleArray, int threadCount);
  void CheckFileSize(LPCWSTR fileName, unsigned __int64 expectedSize);

  CImageBuffer* fEmptyReaderQueue;
  CImageBuffer* fFilledReaderQueue;
  unsigned fClusterSize;
  std::wstring fFileNamePrefix;
  std::wstring fRenamedFileNamePrefix;
  static const unsigned sChunkSize;
  static const unsigned sStreamSize;
};