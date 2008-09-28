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
#include "..\..\src\ODIN\OSException.h"
#include "..\..\src\ODIN\CompressionException.h"
#include "ExceptionTest.h"
using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ExceptionTest );


void 
ExceptionTest::setUp()
{
}


void 
ExceptionTest::tearDown()
{
}


void 
ExceptionTest::testSimpleException()
{
  std::wstring res, winMsg;
  std::wstring ref = L"Exception of type Windows Exception thrown, cause: ";
  const DWORD testErrorCode = 125;
  GetWindowsMessage(testErrorCode, winMsg);
  ref += winMsg;

  try {
    ::SetLastError(testErrorCode); // The disk has no volume label
    CHECK_OS_EX(FALSE);
    CPPUNIT_FAIL("OSException was expected");
  } catch (Exception& e) {
    // ATLTRACE("!!!Exception 1!!! %S\n", e.GetMessage());
    res = e.GetMessage();
    CPPUNIT_ASSERT(res.compare(ref) == 0);
  }
}

void 
ExceptionTest::testExceptionWithErrorCode()
{
  std::wstring res, winMsg;
  const DWORD testErrorCode = 125;
  try {
    ::SetLastError(testErrorCode); // The disk has no volume label
    CHECK_OS_EX_INFO(FALSE, EWinException::testError);
    CPPUNIT_FAIL("OSException was expected");
  } catch (Exception& e) {
    // ATLTRACE("!!!Exception 2!!! %S\n", e.GetMessage());
    std::wstring ref = L"Exception of type Windows Exception thrown, cause: test message WinException. ";
    GetWindowsMessage(testErrorCode, winMsg);
    ref += winMsg;
    res = e.GetMessage();
    CPPUNIT_ASSERT(res.compare(ref) == 0);
  }
}

void 
ExceptionTest::testExceptionWithErrorParam()
{
  std::wstring res, winMsg;
  const DWORD testErrorCode = 125;
  try {
    ::SetLastError(testErrorCode); // The disk has no volume label
    throw EWinException((int)::GetLastError(), __WFILE__, __LINE__, EWinException::fileOpenError, L"X:");
    CPPUNIT_FAIL("OSException was expected");
  } catch (Exception& e) {
    //ATLTRACE("!!!Exception 3!!! %S\n", e.GetMessage());
    std::wstring ref = L"Exception of type Windows Exception thrown, cause: Failed to open file: X:. ";
    GetWindowsMessage(testErrorCode, winMsg);
    ref += winMsg;
    res = e.GetMessage();
    CPPUNIT_ASSERT(res.compare(ref) == 0);
  }
}

void 
ExceptionTest::testCompressionException()
{
  std::wstring res;
  try {
    THROWEX(EZLibCompressionException, -1);
  } catch (Exception& e) {
    res =  e.GetMessage();
    // ATLTRACE("!!!Exception!!! %S\n", e.GetMessage());
    const std::wstring ref = L"Exception of type Compression Exception thrown, cause: general compression/decompression error (-1)";
    CPPUNIT_ASSERT(res.compare(ref) == 0);
    e.LogMessage();
  }
}

void ExceptionTest::GetWindowsMessage(DWORD errorCode, wstring& msg)
{
  wchar_t *buffer = NULL;
  DWORD len;
  len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, NULL );
  if (len > 0)
    msg = std::wstring(buffer, 0, len-2);
  if (buffer != NULL)
    LocalFree(buffer);
}