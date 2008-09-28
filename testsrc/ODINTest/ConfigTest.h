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
#include "..\..\src\ODIN\Config.h"

class ConfigTest : public CppUnit::TestFixture
{
private:
  DECLARE_SECTION()
  DECLARE_NAMED_SECTION(fruitssection);
  DECLARE_NAMED_SECTION(treesection);
  CPPUNIT_TEST_SUITE( ConfigTest );
  CPPUNIT_TEST( CfgInt1Test );
  CPPUNIT_TEST( CfgInt2Test );
  CPPUNIT_TEST( CfgString1Test );
  CPPUNIT_TEST( CfgString2Test );
  CPPUNIT_TEST( CfgUnsigned1Test );
  CPPUNIT_TEST( CfgUnsigned2Test );
  CPPUNIT_TEST( CfgBool1Test );
  CPPUNIT_TEST( CfgBool2Test );
  CPPUNIT_TEST( CfgChar1Test );
  CPPUNIT_TEST( CfgChar2Test );
  CPPUNIT_TEST( CfgDouble1Test );
  CPPUNIT_TEST( CfgDouble2Test );
  CPPUNIT_TEST( CfgNamedTest1 );
  CPPUNIT_TEST( CfgNamedTest2 );
  CPPUNIT_TEST( CfgNamedTest3 );
  CPPUNIT_TEST( CfgNamedTest4 );
  CPPUNIT_TEST( TableOverflowTest );
  CPPUNIT_TEST( LastTest );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void CfgInt1Test();
  void CfgInt2Test();
  void CfgString1Test();
  void CfgString2Test();
  void CfgUnsigned1Test();
  void CfgUnsigned2Test();
  void CfgBool1Test();
  void CfgBool2Test();
  void CfgChar1Test();
  void CfgChar2Test();
  void CfgDouble1Test();
  void CfgDouble2Test();
  void CfgNamedTest1();
  void CfgNamedTest2();
  void CfgNamedTest3();
  void CfgNamedTest4();
  void TableOverflowTest();
  void LastTest();
};