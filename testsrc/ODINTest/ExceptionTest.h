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

class ExceptionTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( ExceptionTest );
  CPPUNIT_TEST( testSimpleException );
  CPPUNIT_TEST( testExceptionWithErrorCode );
  CPPUNIT_TEST( testExceptionWithErrorParam );
  CPPUNIT_TEST( testCompressionException );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void testSimpleException();
  void testExceptionWithErrorCode();
  void testExceptionWithErrorParam();
  void testCompressionException();
private:
  void GetWindowsMessage(DWORD errorCode, std::wstring& msg);

};