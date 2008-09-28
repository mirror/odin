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
#include "..\..\src\ODIN\Compression.h"

class CImageBuffer;

class ImageTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( ImageTest );
  /**/
  CPPUNIT_TEST( saveRestoreImageTestSimple );
  CPPUNIT_TEST( verifyTestSimple );
  CPPUNIT_TEST( saveImageTestRunLengthStandard );
  CPPUNIT_TEST( restoreImageTestRunLengthStandard );
  CPPUNIT_TEST( saveImageTestRunLengthShort );
  CPPUNIT_TEST( restoreImageTestRunLengthShort );
  CPPUNIT_TEST( saveImageTestRunLengthLong );
  CPPUNIT_TEST( restoreImageTestRunLengthLong );
  CPPUNIT_TEST( saveImageTestRunLengthSpecial );
  CPPUNIT_TEST( restoreImageTestRunLengthSpecial );
  /**/
  CPPUNIT_TEST( saveCompressedImageGzipTest );
  CPPUNIT_TEST( saveCompressedImageBzip2Test );
  /**/
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void saveRestoreImageTestSimple();
  void verifyTestSimple();
  void saveImageTestSimple();
  void restoreImageTestSimple();
  void saveImageTestRunLengthStandard();
  void restoreImageTestRunLengthStandard();
  void saveImageTestRunLengthShort();
  void restoreImageTestRunLengthShort();
  void saveImageTestRunLengthLong();
  void restoreImageTestRunLengthLong();
  void saveImageTestRunLengthSpecial();
  void restoreImageTestRunLengthSpecial();

  void saveCompressedImageGzipTest();
  void saveCompressedImageBzip2Test();


private:
  void WaitUntilDone(HANDLE* threadHandleArray, int threadCount);
  void runSimpleSaveRestore(bool verifyOnly);
  void saveCompressed(TCompressionFormat compressionType);
  void saveImageTestRunLength(int* runLengthArray, int len);
  void restoreImageTestRunLength(int* runLengthArray, int len);
  void LogSeekPositions();

  CImageBuffer* fEmptyReaderQueue;
  CImageBuffer* fFilledReaderQueue;
  CImageBuffer* fEmptyCompDecompQueue;
  CImageBuffer* fFilledCompDecompQueue;
  unsigned fClusterSize;
  static unsigned sSavedCrc32;
};