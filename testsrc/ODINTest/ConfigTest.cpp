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
#include <iostream>
#include "ConfigTest.h"
#include "..\..\src\ODIN\InternalException.h"
using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ConfigTest );

IMPL_SECTION(ConfigTest, L"UnitTestSection1")

void ConfigTest::setUp()
{
}

void ConfigTest::tearDown()
{
}

void ConfigTest::CfgInt1Test()
{
  CConfigEntry<int> cfgInt(L"intkey", 0);
  CPPUNIT_ASSERT(cfgInt == 0);
  cfgInt = 1247;
  CPPUNIT_ASSERT(cfgInt == 1247);
}

void ConfigTest::CfgInt2Test()
{
  CConfigEntry<int> cfgInt(L"intkey", 0);
  CPPUNIT_ASSERT(cfgInt == 1247);
}

void ConfigTest::CfgString1Test()
{
  CConfigEntry<std::wstring> cfgString(L"HelloString", L"init");
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"init")) == 0);
  cfgString = std::wstring(L"Hello String"); // test = operator with std::wstring
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"Hello String")) == 0);

  CConfigEntry<std::wstring> cfgString2(L"GoodByeString", std::wstring(L"xxx"));
  CPPUNIT_ASSERT(cfgString2().compare(std::wstring(L"xxx")) == 0);
  cfgString2 = L"Good Bye String"; // test = operator with const char*
  CPPUNIT_ASSERT(cfgString2().compare(std::wstring(L"Good Bye String")) == 0);
}

void ConfigTest::CfgString2Test()
{
  CConfigEntry<std::wstring> cfgString(L"HelloString", L"init");
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"Hello String")) == 0);
  CConfigEntry<std::wstring> cfgString2(L"GoodByeString", std::wstring(L"xxx"));
  CPPUNIT_ASSERT(cfgString2().compare(std::wstring(L"Good Bye String")) == 0);
}

void ConfigTest::CfgUnsigned1Test()
{
  CConfigEntry<unsigned> cfg(L"Unsigned_1", 1234);
  CPPUNIT_ASSERT(cfg == 1234);
  cfg = 5678;
  CPPUNIT_ASSERT(cfg == 5678);
}

void ConfigTest::CfgUnsigned2Test()
{
  CConfigEntry<unsigned> cfg(L"Unsigned_1", 1111);
  CPPUNIT_ASSERT(cfg == 5678);
}

void ConfigTest::CfgBool1Test()
{
  CConfigEntry<bool> cfg(L"Bool_1", false);
  CPPUNIT_ASSERT(cfg == false);
  cfg = true;
  CPPUNIT_ASSERT(cfg == true);
}

void ConfigTest::CfgBool2Test()
{
  CConfigEntry<bool> cfg(L"Bool_1", false);
  CPPUNIT_ASSERT(cfg == true);
}

void ConfigTest::CfgChar1Test()
{
  CConfigEntry<wchar_t> cfg(L"Char_1", L'A');
  CPPUNIT_ASSERT(cfg == L'A');
  cfg = L'Z';
  CPPUNIT_ASSERT(cfg == L'Z');
}

void ConfigTest::CfgChar2Test()
{
  CConfigEntry<wchar_t> cfg(L"Char_1", L'B');
  CPPUNIT_ASSERT(cfg == L'Z');
}

void ConfigTest::CfgDouble1Test()
{
  CConfigEntry<double> cfg(L"Double_1", 1234.5678);
  CPPUNIT_ASSERT(cfg == 1234.5678);
  cfg = 8765.4321;
  CPPUNIT_ASSERT(cfg == 8765.4321);
}

void ConfigTest::CfgDouble2Test()
{
  CConfigEntry<double> cfg(L"Double_1", -12.12);
  CPPUNIT_ASSERT(cfg == 8765.4321);
}

IMPL_NAMED_SECTION(ConfigTest, treesection, L"Trees")

void ConfigTest::CfgNamedTest1() 
{
  DECLARE_INIT_ENTRY_WITH_SECTION(wstring, cfgString, L"Tree_1", treesection, L"Maple");
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"Maple")) == 0);
  cfgString = L"Oak"; 
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"Oak")) == 0);
}

void ConfigTest::CfgNamedTest2() 
{
  DECLARE_INIT_ENTRY_WITH_SECTION(wstring, cfgString, L"Tree_1", treesection, L"Maple");
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"Oak")) == 0);
}

IMPL_NAMED_SECTION(ConfigTest, fruitssection, L"Fruits")

void ConfigTest::CfgNamedTest3() {
  CConfigEntry<wstring, fruitssection> cfgString(L"Fruit_1", L"Apple");  
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"Apple")) == 0);
  cfgString = L"Pear"; 
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"Pear")) == 0);
}

void ConfigTest::CfgNamedTest4() {
  CConfigEntry<wstring, fruitssection> cfgString(L"Fruit_1", L"Apple");  
  CPPUNIT_ASSERT(cfgString().compare(std::wstring(L"Pear")) == 0);
}

void ConfigTest::TableOverflowTest()
{
  wchar_t* longString;
  size_t len = CConfigEntryStatics::sStringTable.GetCapacity()+10;
  longString = new wchar_t[len];
  for (size_t i=0; i < (len*sizeof(wchar_t)/sizeof(longString[0])); i++)
    longString[i] = i%26 + L'a';
  longString[len-1] = L'\0';

  try {
    // must throw an internal exception
    CConfigEntry<int, fruitssection> cfg(longString, 123);
    delete [] longString;
    CPPUNIT_FAIL("InternalException with error code internalStringTableOverflow was expected.");
  } catch (EInternalException& e) {
    delete [] longString;
    CPPUNIT_ASSERT(e.GetErrorCode() == EInternalException::internalStringTableOverflow);
  }
}

void ConfigTest::LastTest() 
{
  BOOL ok;
  ok = CConfigEntryStatics::sIni.DeleteSection(L"Standard");
  CPPUNIT_ASSERT_EQUAL(ok, TRUE);
  ok = CConfigEntryStatics::sIni.DeleteSection(ConfigTest::fruitssection);
  CPPUNIT_ASSERT_EQUAL(ok, TRUE);
  ok = CConfigEntryStatics::sIni.DeleteSection(ConfigTest::treesection);
  CPPUNIT_ASSERT_EQUAL(ok, TRUE);
}
