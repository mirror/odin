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

class CompressedRunLengthStreamTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( CompressedRunLengthStreamTest );
  CPPUNIT_TEST( simpleTest );
  // takes so long: 
  CPPUNIT_TEST( OverflowTest );
  CPPUNIT_TEST( ContinueSequenceTest );
  CPPUNIT_TEST( ReaderTest1 );
  CPPUNIT_TEST( ReaderTest2 );
  CPPUNIT_TEST( ReaderTest3 );
  CPPUNIT_TEST( ReaderTest4 );
  CPPUNIT_TEST( extraLenTest );
  CPPUNIT_TEST( extraLenTest2 );
  CPPUNIT_TEST_SUITE_END();

public:
  CompressedRunLengthStreamTest();
  ~CompressedRunLengthStreamTest();
  
  void setUp();
  void tearDown();

  void simpleTest();
  void OverflowTest();
  void ContinueSequenceTest();
  void ReaderTest1();
  void ReaderTest2();
  void ReaderTest3();
  void ReaderTest4();
  void extraLenTest();
  void extraLenTest2();
private:
  static bool sWasDeleted;
};

