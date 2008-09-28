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
#include "..\..\src\ODIN\CompressedRunLengthStream.h"
#include "BitArrayTest.h"
#include <iostream>
using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( BitArrayTest );


void 
BitArrayTest::setUp()
{
}


void 
BitArrayTest::tearDown()
{
}


void 
BitArrayTest::testGetBit()
{
  // test GetBit()
  BYTE testArray[] = {0x66, 0x66,0x66,0x66};

  CBitArray b;
  b.LoadBuffer(testArray, sizeof(testArray) * 8);
  bool val;
  unsigned result = 0;

  for (int i=0; i<32; i++)  {
    val = b.GetBit(i);    
    // cout << "Test bit " << i << " result: " << val << endl;
    result <<= 1;
    if (val)
      result |= 1;
  }
  unsigned *referencePtr = (unsigned*) testArray;
  /*
  cout << "GetBit() result: " << result << " org: " << *referencePtr << endl;
  if (result == *referencePtr)
    cout << "  ... ok" << endl;
  else
    cout << "  ... failure" << endl;
  */
  CPPUNIT_ASSERT_EQUAL(result, *referencePtr);
}

void 
BitArrayTest::testSetBit()
{
  BYTE testArray[] = {0x0, 0x0, 0x0, 0x0};

  CBitArray b;
  b.LoadBuffer(testArray, sizeof(testArray) * 8);

  // test set bit
  b.SetBit(0, true);
  b.SetBit(1, false);
  b.SetBit(2, false);
  b.SetBit(3, true);
  b.SetBit(4, true);
  b.SetBit(5, false);
  b.SetBit(6, false);
  b.SetBit(7, true);

  b.SetBit(8, true);
  b.SetBit(9, false);
  b.SetBit(10, false);
  b.SetBit(11, true);
  b.SetBit(12, true);
  b.SetBit(13, false);
  b.SetBit(14, false);
  b.SetBit(15, true);

  b.SetBit(16, true);
  b.SetBit(17, false);
  b.SetBit(18, false);
  b.SetBit(19, true);
  b.SetBit(20, true);
  b.SetBit(21, false);
  b.SetBit(22, false);
  b.SetBit(23, true);

  b.SetBit(24, true);
  b.SetBit(25, false);
  b.SetBit(26, false);
  b.SetBit(27, true);
  b.SetBit(28, true);
  b.SetBit(29, false);
  b.SetBit(30, false);
  b.SetBit(31, true);

  unsigned reference = 0x99999999;
  unsigned* resultPtr = (unsigned*) (b.fArray);
  /*
  cout << "SetBit() result: " << result << " expected: " << *resultPtr << endl;
  if (reference == *resultPtr)
    cout << "  ... ok" << endl;
  else
    cout << "  ... failure" << endl;
  */
  CPPUNIT_ASSERT_EQUAL(reference, *resultPtr);
}

void 
BitArrayTest::testFindSetBit()
{
  unsigned result = 0;
  CBitArray b;
  BYTE testArray2[] = {0x0, 0x66,0x66,0xF0};
  b.LoadBuffer(testArray2, sizeof(testArray2) * 8);  
  result = b.FindSetBit();
  /*
  cout << "FindSetBit() result: " << result << " expected: 9" << endl;
  if (result == 9)
    cout << "  ... ok" << endl;
  else
    cout << "  ... failure" << endl;
  */
  BYTE testArray3[] = {0x0, 0x0, 0x0, 0xF0};
  b.LoadBuffer(testArray3, sizeof(testArray3) * 8);  
  result = b.FindSetBit();
  /*
  cout << "FindSetBit() result: " << result << " expected: 28" << endl;
  if (result == 28)
    cout << "  ... ok" << endl;
  else
    cout << "  ... failure" << endl;
  */
  CPPUNIT_ASSERT(result==28);
}


void 
BitArrayTest::testRunLength()
{
  unsigned result = 0;
  CBitArray b;
  // test  run length calculation
  BYTE testArray4[] = {0xF0, 0xFF, 0x7F, 0x0};
  b.LoadBuffer(testArray4, sizeof(testArray4) * 8);  
  // 11110000 11111111 01111111 00000000
  // 00001111 11111111 11111110 00000000 lowest bit to highest bit
  int index = 0;
  result = b.GetRunLengthNotSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 4" << endl;
  CPPUNIT_ASSERT(result == 4);

  index = 8;
  result = b.GetRunLengthNotSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 0" << endl;
  CPPUNIT_ASSERT(result == 0);

  index = 20;
  result = b.GetRunLengthNotSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 0" << endl;
  CPPUNIT_ASSERT(result == 0);

  index = 0;
  result = b.GetRunLengthSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 0" << endl;
  CPPUNIT_ASSERT(result == 0);

  index = 16;
  result = b.GetRunLengthSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 7" << endl;
  CPPUNIT_ASSERT(result == 7);

  index = 8;
  result = b.GetRunLengthSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 15" << endl;
  CPPUNIT_ASSERT(result == 15);

  BYTE testArray5[] = {0x00, 0xFF, 0x00, 0xFF};
  b.LoadBuffer(testArray5, sizeof(testArray5) * 8);  
  index = 12;
  result = b.GetRunLengthSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 4" << endl;
  CPPUNIT_ASSERT(result == 4);

  index = 21;
  result = b.GetRunLengthNotSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 3" << endl;
  CPPUNIT_ASSERT(result == 3);

  // run into the first for loop but not into second
  index = 29;
  result = b.GetRunLengthSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 3" << endl;
  CPPUNIT_ASSERT(result == 3);

  testArray4[3] = 0x0;
  b.LoadBuffer(testArray4, sizeof(testArray4) * 8);  
  index = 28;
  result = b.GetRunLengthNotSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 4" << endl;
  CPPUNIT_ASSERT(result == 4);
}

void 
BitArrayTest::testRunLengthNoByteBoundary()
{
  unsigned result = 0;
  CBitArray b;
  int index = 0;

  // test bit array that does not end on a byte boundary
  BYTE testArray6[] = {0x00, 0xFF, 0x00, 0x00};
  b.LoadBuffer(testArray6, sizeof(testArray6) * 8 - 3);  
  index = 20;
  result = b.GetRunLengthNotSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 9" << endl;
  CPPUNIT_ASSERT(result == 9);

  index = 25;
  result = b.GetRunLengthNotSet(index);
  // cout << "GetRunLengthNotSet(" << index << ") result: " << result << " expected: 4" << endl;
  CPPUNIT_ASSERT(result == 4);

  BYTE testArray7[] = {0x00, 0x00, 0xFF, 0xFF};
  b.LoadBuffer(testArray7, sizeof(testArray7) * 8 - 3);  
  index = 20;
  result = b.GetRunLengthSet(index);
  // cout << "GetRunLengthSet(" << index << ") result: " << result << " expected: 9" << endl;
  CPPUNIT_ASSERT(result == 9);

  index = 25;
  result = b.GetRunLengthSet(index);
  // cout << "GetRunLengthSet(" << index << ") result: " << result << " expected: 4" << endl;
  CPPUNIT_ASSERT(result == 4);
}

