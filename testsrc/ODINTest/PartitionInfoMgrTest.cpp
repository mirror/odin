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
#include "..\..\src\ODIN\PartitionInfoMgr.h"
#include "PartitionInfoMgrTest.h"
#include "..\..\src\ODIN\OSException.h"
#include "..\..\src\ODIN\FileFormatException.h"
#include "..\..\src\ODIN\InternalException.h"

using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PartitionInfoMgrTest );

// section name in .ini file for configuration values
IMPL_SECTION(PartitionInfoMgrTest, L"PartitionInfoMgrTest")

void PartitionInfoMgrTest::setUp()
{
}

void PartitionInfoMgrTest::tearDown()
{
}

PartitionInfoMgrTest::PartitionInfoMgrTest() :
  fEnableTest(L"EnableDiskTest", false),
  fImageDisk(L"ImageDisk", L"\\Device\\Harddisk3\\Partition0")
{
  static bool once = true;
  if (once) {
    once = false;
    if (fEnableTest) {
      wcout << L"Printing configuration for PartitionInfoMgrTest:" << endl;
      wcout << L"===========================================" << endl;
      wcout << L"Hard disk that is backuped and restored: " << fImageDisk().c_str() << endl;
    }
    else
      wcout << L"The critical tests are disabled by configuration" << endl;
  }
}

// setup a test partition inf and return bufferwith structure
BYTE* PartitionInfoMgrTest::PreparePartitionInfo()
{
  DRIVE_LAYOUT_INFORMATION_EX dl;
  const int partCount = 12;

  PARTITION_INFORMATION_EX pi[partCount];
  BYTE* buffer;

  ZeroMemory(pi, sizeof(pi));
  dl.PartitionStyle = PARTITION_STYLE_MBR;
  dl.PartitionCount = partCount;
  dl.Mbr.Signature = 4711;
  
  pi[0].PartitionStyle = PARTITION_STYLE_MBR;
  pi[0].StartingOffset.QuadPart = 32256I64;  
  pi[0].PartitionLength.QuadPart = 1073447424I64;  
  pi[0].PartitionNumber = 1;  
  pi[0].RewritePartition = FALSE;  
  pi[0].Mbr.PartitionType = (BYTE)7;
  pi[0].Mbr.BootIndicator = FALSE;
  pi[0].Mbr.RecognizedPartition = TRUE;
  pi[0].Mbr.HiddenSectors = 63;

  pi[1].PartitionStyle = PARTITION_STYLE_MBR;
  pi[1].StartingOffset.QuadPart = 1073479680;  
  pi[1].PartitionLength.QuadPart = 1069350912;  
  pi[1].PartitionNumber = 0;  
  pi[1].RewritePartition = FALSE;  
  pi[1].Mbr.PartitionType = (BYTE)5;
  pi[1].Mbr.BootIndicator = FALSE;
  pi[1].Mbr.RecognizedPartition = FALSE;
  pi[1].Mbr.HiddenSectors = 2096640;

  pi[4].PartitionStyle = PARTITION_STYLE_MBR;
  pi[4].StartingOffset.QuadPart = 1073511936;  
  pi[4].PartitionLength.QuadPart = 536707584;  
  pi[4].PartitionNumber = 2;  
  pi[4].RewritePartition = FALSE;  
  pi[4].Mbr.PartitionType = (BYTE)7;
  pi[4].Mbr.BootIndicator = FALSE;
  pi[4].Mbr.RecognizedPartition = TRUE;
  pi[4].Mbr.HiddenSectors = 63;

  pi[5].PartitionStyle = PARTITION_STYLE_MBR;
  pi[5].StartingOffset.QuadPart = 1610219520;  
  pi[5].PartitionLength.QuadPart = 532611072;  
  pi[5].PartitionNumber = 0;  
  pi[5].RewritePartition = FALSE;  
  pi[5].Mbr.PartitionType = (BYTE)5;
  pi[5].Mbr.BootIndicator = FALSE;
  pi[5].Mbr.RecognizedPartition = TRUE;
  pi[5].Mbr.HiddenSectors = 1048320;

  pi[8].PartitionStyle = PARTITION_STYLE_MBR;
  pi[8].StartingOffset.QuadPart = 1610251776;  
  pi[8].PartitionLength.QuadPart = 532578816;  
  pi[8].PartitionNumber = 3;  
  pi[8].RewritePartition = FALSE;  
  pi[8].Mbr.PartitionType = (BYTE)6;
  pi[8].Mbr.BootIndicator = FALSE;
  pi[8].Mbr.RecognizedPartition = TRUE;
  pi[8].Mbr.HiddenSectors = 63;

  int bufferSize = (partCount-1) * sizeof(PARTITION_INFORMATION_EX) + sizeof(DRIVE_LAYOUT_INFORMATION_EX);
  buffer = new BYTE[bufferSize];
  memcpy(buffer, &dl, sizeof(dl));
  memcpy(buffer+sizeof(dl)-sizeof(PARTITION_INFORMATION_EX), &pi, sizeof(pi));
  return buffer;
}

void PartitionInfoMgrTest::TestEquality(BYTE* partInfoBuffer, BYTE* readBuffer)
{
  DRIVE_LAYOUT_INFORMATION_EX* dl1 = (DRIVE_LAYOUT_INFORMATION_EX*) partInfoBuffer;
  DRIVE_LAYOUT_INFORMATION_EX* dl2 = (DRIVE_LAYOUT_INFORMATION_EX*) readBuffer;
  
  DWORD partCount1 = dl1->PartitionCount;
  DWORD partCount2 = dl2->PartitionCount;
  CPPUNIT_ASSERT_EQUAL(partCount1, partCount2);
  CPPUNIT_ASSERT_EQUAL(dl1->PartitionStyle, dl2->PartitionStyle);
  CPPUNIT_ASSERT_EQUAL(dl1->Mbr.Signature, dl2->Mbr.Signature);

  PARTITION_INFORMATION_EX* pi1 = dl1->PartitionEntry;
  PARTITION_INFORMATION_EX* pi2 = dl2->PartitionEntry;
  for (unsigned i=0; i<partCount1; i++) {
    CPPUNIT_ASSERT_EQUAL(pi1->PartitionStyle, pi2->PartitionStyle);
    CPPUNIT_ASSERT_EQUAL(pi1->StartingOffset.QuadPart, pi2->StartingOffset.QuadPart);
    CPPUNIT_ASSERT_EQUAL(pi1->PartitionLength.QuadPart, pi2->PartitionLength.QuadPart);
    CPPUNIT_ASSERT_EQUAL(pi1->PartitionNumber, pi2->PartitionNumber);
    CPPUNIT_ASSERT_EQUAL(pi1->RewritePartition, pi2->RewritePartition);
    CPPUNIT_ASSERT_EQUAL(pi1->Mbr.PartitionType, pi2->Mbr.PartitionType);
    CPPUNIT_ASSERT_EQUAL(pi1->Mbr.BootIndicator, pi2->Mbr.BootIndicator);
    CPPUNIT_ASSERT_EQUAL(pi1->Mbr.RecognizedPartition, pi2->Mbr.RecognizedPartition);
    CPPUNIT_ASSERT_EQUAL(pi1->Mbr.HiddenSectors, pi2->Mbr.HiddenSectors);
  }
}


void PartitionInfoMgrTest::ValidFileHeaderTest()
{
  CPartitionInfoMgr partitionInfoMgr;
  CPartitionInfoMgr::TPartitionInfoFileHeader header;

  partitionInfoMgr.PrepareFileHeader(header);
  CPPUNIT_ASSERT_EQUAL(partitionInfoMgr.IsValidFileHeader(header), true);
  header.guid.Data4[7] = 0xff;
  CPPUNIT_ASSERT_EQUAL(partitionInfoMgr.IsValidFileHeader(header), false);
}

void PartitionInfoMgrTest::SupportedVersionTest()
{
  CPartitionInfoMgr partitionInfoMgr;
  CPartitionInfoMgr::TPartitionInfoFileHeader header;

  partitionInfoMgr.PrepareFileHeader(header);
  // check default version:
  header.versionMajor = CPartitionInfoMgr::sVerMajor;
  header.versionMinor = CPartitionInfoMgr::sVerMinor;
  CPPUNIT_ASSERT_EQUAL(partitionInfoMgr.IsSupportedVersion(header), true);
  // check higher minor version:
  header.versionMinor = CPartitionInfoMgr::sVerMinor + 1;
  CPPUNIT_ASSERT_EQUAL(partitionInfoMgr.IsSupportedVersion(header), true);
  // check wrong minor version:
  header.versionMajor = CPartitionInfoMgr::sVerMajor + 1;
  header.versionMinor = CPartitionInfoMgr::sVerMinor;
  CPPUNIT_ASSERT_EQUAL(partitionInfoMgr.IsSupportedVersion(header), false);
}

void PartitionInfoMgrTest::CreateFileHeaderTest()
{
  BYTE* partInfoBuffer = PreparePartitionInfo();

  CPartitionInfoMgr partitionInfoMgr(partInfoBuffer);
  CPPUNIT_ASSERT_EQUAL(partitionInfoMgr.GetPartitionCount(), 3U);

  // delete partInfoBuffer; // do not delete here. will be deleted in destructor of CPartitionInfoMgr
}

void PartitionInfoMgrTest::ReadWriteFileHeaderTest()
{
  BYTE* partInfoBuffer = PreparePartitionInfo();
  fFileName = L"PartitionInfoMgrTest.mbr";

  CPartitionInfoMgr partitionInfoMgr(partInfoBuffer);
  CPartitionInfoMgr partitionInfoMgr2;
  partitionInfoMgr.WritePartitionInfoToFile(fFileName.c_str());
  partitionInfoMgr2.ReadPartitionInfoFromFile(fFileName.c_str());
  BYTE* readBuffer = partitionInfoMgr2.GetBuffer();
  TestEquality(partInfoBuffer, readBuffer);
  ::DeleteFile(fFileName.c_str());
}

void PartitionInfoMgrTest::ReadWriteFileHeaderExceptionTest()
{
  CPartitionInfoMgr partitionInfoMgr;
  fFileName = L"PartitionInfoMgrTest.mbr";
  DWORD sizeWritten, res;
  HANDLE hDisk;
  BYTE bufferWrongData[] = {100, 100};
  DWORD wrongVersionNo = 0x10002;
  PARTITION_STYLE partStyle = PARTITION_STYLE_GPT;

  // Test wrong magic number
  partitionInfoMgr.WritePartitionInfoToFile(fFileName.c_str());
  hDisk = CreateFile(fFileName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CPPUNIT_ASSERT(hDisk != INVALID_HANDLE_VALUE);
  res = WriteFile(hDisk, bufferWrongData, sizeof(bufferWrongData), &sizeWritten, NULL);
  CPPUNIT_ASSERT(res);
  CloseHandle(hDisk);

  try {
    partitionInfoMgr.ReadPartitionInfoFromFile(fFileName.c_str());
    CPPUNIT_FAIL("EFileFormatException expected with error code magicByteError");
  } catch (EFileFormatException& e) {
    CPPUNIT_ASSERT_EQUAL(e.GetErrorCode() , (int) EFileFormatException::magicByteError);
  }
  res = ::DeleteFile(fFileName.c_str());
  CPPUNIT_ASSERT(res);

  // test wrong version number  
  BYTE* partInfoBuffer = PreparePartitionInfo();
  CPartitionInfoMgr partitionInfoMgr2(partInfoBuffer);
  partitionInfoMgr2.WritePartitionInfoToFile(fFileName.c_str());
  hDisk = CreateFile(fFileName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CPPUNIT_ASSERT(hDisk != INVALID_HANDLE_VALUE);
  res = SetFilePointer(hDisk, sizeof(GUID), NULL, FILE_BEGIN);
  res = WriteFile(hDisk, &wrongVersionNo, sizeof(wrongVersionNo), &sizeWritten, NULL);
  CPPUNIT_ASSERT(res);
  res = CloseHandle(hDisk);
  CPPUNIT_ASSERT(res);

  try {
    partitionInfoMgr.ReadPartitionInfoFromFile(fFileName.c_str());
    CPPUNIT_FAIL("EFileFormatException expected with error code majorVersionError");
  } catch (EFileFormatException& e) {
    CPPUNIT_ASSERT_EQUAL(e.GetErrorCode() , (int) EFileFormatException::majorVersionError);
  }
  res = ::DeleteFile(fFileName.c_str());
  CPPUNIT_ASSERT(res);

  // test wrong file size  
  partitionInfoMgr.WritePartitionInfoToFile(fFileName.c_str());
  try {
    partitionInfoMgr.ReadPartitionInfoFromFile(fFileName.c_str());
    CPPUNIT_FAIL("EFileFormatException expected with error code wrongFileSizeError");
  } catch (EFileFormatException& e) {
    CPPUNIT_ASSERT_EQUAL(e.GetErrorCode() , (int) EFileFormatException::wrongFileSizeError);
  }
  res = ::DeleteFile(fFileName.c_str());
  CPPUNIT_ASSERT(res);

  // Test wrong partition format
  BYTE* partInfoBuffer2 = PreparePartitionInfo();
  CPartitionInfoMgr partitionInfoMgr3(partInfoBuffer2);
  partitionInfoMgr3.WritePartitionInfoToFile(fFileName.c_str());
  hDisk = CreateFile(fFileName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CPPUNIT_ASSERT(hDisk != INVALID_HANDLE_VALUE);
  res = SetFilePointer(hDisk, sizeof(CPartitionInfoMgr::TPartitionInfoFileHeader) + sizeof(CPartitionInfoMgr::TBootLoader), NULL, FILE_BEGIN);
  res = WriteFile(hDisk, &partStyle, sizeof(partStyle), &sizeWritten, NULL);
  CPPUNIT_ASSERT(res);
  CloseHandle(hDisk);

  try {
    partitionInfoMgr.ReadPartitionInfoFromFile(fFileName.c_str());
    CPPUNIT_FAIL("EInternalException expected with error code unsupportedPartitionFormat");
  } catch (EInternalException& e) {
    CPPUNIT_ASSERT_EQUAL(e.GetErrorCode() , (int) EInternalException::unsupportedPartitionFormat);
  }
  res = ::DeleteFile(fFileName.c_str());
  CPPUNIT_ASSERT(res);
}

void PartitionInfoMgrTest::ReadWriteDiskTest ()
{
  if (!fEnableTest) 
    return;

  CPartitionInfoMgr partitionInfoMgr;
  DWORD res;
  fFileName = L"XPartitionInfoMgrTest.mbr";
  partitionInfoMgr.ReadPartitionInfoFromDisk(fImageDisk().c_str());
  partitionInfoMgr.WritePartitionInfoToFile(fFileName.c_str());
  // We first delete all logical drives (they are not found if the extended partition is deleted)
  partitionInfoMgr.ClearPartitionInfo(4);
  partitionInfoMgr.WritePartitionInfoToDisk(fImageDisk().c_str());
  // The we delete the primary and extended partition
  partitionInfoMgr.ClearPartitionInfo(0);
  partitionInfoMgr.WritePartitionInfoToDisk(fImageDisk().c_str());
  // Then restore from file

  partitionInfoMgr.ReadPartitionInfoFromFile(fFileName.c_str());
  partitionInfoMgr.MakeWritable();
  partitionInfoMgr.DumpPartitionInfo();
  partitionInfoMgr.WritePartitionInfoToDisk(fImageDisk().c_str());
  res = ::DeleteFile(fFileName.c_str());
  CPPUNIT_ASSERT(res);
}
