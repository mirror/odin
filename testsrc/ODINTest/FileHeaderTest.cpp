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
#include "..\..\src\ODIN\Compression.h"
#include "FileHeaderTest.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( FileHeaderTest );


// Static constants:
int FileHeaderTest::sCounter = 0;
wchar_t fileName[] = L"TestImageFileHeader.dat";
const DWORD crc32Value = 0x4321;
wchar_t comment[] = L"Give peace a chance!";
const BYTE allocBitmap[] = {0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21};
const unsigned __int64 volumeSize = 8000000000;
const unsigned __int64 cUsedSize = 6000000000;
//const unsigned __int64 dataSize = 0x89ABCDEF01;
const unsigned clusterSize = 4096;
const unsigned fileCount = 1;
const BYTE volumeData[] = {
  0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
  0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
  0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
  0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
  0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
  0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
  0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
  0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
};

void FileHeaderTest::setUp()
{
}

void FileHeaderTest::tearDown()
{
}

void FileHeaderTest::WriteHeaderTest()
{
  unsigned __int64 off, allocMapOffset, allocMapLength, crc32Offset, dataOffset, dataLength;
  unsigned commentLength;
  
  fHandle = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  CPPUNIT_ASSERT(fHandle != INVALID_HANDLE_VALUE);

  // first write a default file header
  fImageHeader.WriteHeaderToFile(fHandle);

  crc32Offset = Seek(0, FILE_END);
  fImageHeader.SetVerifyFormat(CImageFileHeader::verifyCRC32);
  fImageHeader.SetVerifyOffsetAndLength(crc32Offset, sizeof(DWORD));
  WriteCrc32Checksum(0, crc32Offset, sizeof(DWORD)); // dummy value just to reserve space at position in file
  
  off = Seek(0, FILE_END);
  commentLength = sizeof(comment);
  
  // simulate writing a comment
  WriteComment();
  fImageHeader.SetComment(off, commentLength);

  allocMapOffset = Seek(0, FILE_END);
  allocMapLength = sizeof(allocBitmap);

  
  /* simulate writing a volume bitmap */
  WriteAllocationBitmap();
  fImageHeader.SetVolumeBitmapInfo(CImageFileHeader::simpleCompressedRunLength, allocMapOffset, allocMapLength);

  /* simulate writing data */
  dataLength  = sizeof(volumeData);
  dataOffset = Seek(0, FILE_END);
  WriteVolumeData();
 
  // now we would know real checksum, simulate writing it
  WriteCrc32Checksum(crc32Value, crc32Offset, sizeof(DWORD));

  fImageHeader.SetVolumeSize(volumeSize);
  fImageHeader.SetCompressionFormat(compressionBZIP2);
  fImageHeader.SetVolumeDataOffset(dataOffset);
  fImageHeader.SetVolumeUsedSize(cUsedSize);
  fImageHeader.SetClusterSize(clusterSize);
  fImageHeader.SetDataSize(dataLength);
  fImageHeader.SetFileCount(fileCount);
  fImageHeader.SetVolumeType(CImageFileHeader::volumePartition);
  // now write file header again after all information is complete
  fImageHeader.WriteHeaderToFile(fHandle);

  CloseHandle(fHandle);
}

void FileHeaderTest::ReadHeaderTest()
{
  unsigned __int64 offset, length64, volLength64, usedSize;
  DWORD verifyLength, commentLength;
  
  fHandle = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CPPUNIT_ASSERT(fHandle != INVALID_HANDLE_VALUE);

  // first read file header from file
  fImageHeader.ReadHeaderFromFile(fHandle);
  // check format and version
  CPPUNIT_ASSERT(fImageHeader.IsValidFileHeader());
  CPPUNIT_ASSERT(fImageHeader.IsSupportedVersion());
  CPPUNIT_ASSERT_EQUAL(fImageHeader.GetMajorVersion(), 1U);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.GetMinorVersion(), 0U);

  // get checksum
  CImageFileHeader::VerifyFormat verifyFormat = fImageHeader.GetVerifyFormat();
  CPPUNIT_ASSERT(verifyFormat == CImageFileHeader::verifyCRC32);
  fImageHeader.GetVerifyOffsetAndLength(offset, verifyLength);
  ReadAndCheckBuffer(offset, verifyLength, (BYTE*)&crc32Value, sizeof(crc32Value));

  // get comment
  fImageHeader.GetCommentOffsetAndLength(offset, commentLength);
  ReadAndCheckBuffer(offset, commentLength, (BYTE*)comment, sizeof(comment));

  // get volume type
  CImageFileHeader::VolumeFormat volumeType = fImageHeader.GetVolumeType();
  CPPUNIT_ASSERT(volumeType == CImageFileHeader::volumePartition);


  // get allocation bitmap
  CImageFileHeader::VolumeEncodingFormat encFormat = fImageHeader.GetVolumeEncoding();
  CPPUNIT_ASSERT(encFormat == CImageFileHeader::simpleCompressedRunLength);
  fImageHeader.GetClusterBitmapOffsetAndLength(offset, length64);
  ReadAndCheckBuffer(offset, (unsigned)length64, (BYTE*)allocBitmap, sizeof(allocBitmap));

  // get volume data
  TCompressionFormat compFormat = fImageHeader.GetCompressionFormat();
  CPPUNIT_ASSERT(compFormat == compressionBZIP2);
  unsigned __int64 volSize = fImageHeader.GetVolumeSize();
  CPPUNIT_ASSERT(volSize == volumeSize);
  unsigned clusSize = fImageHeader.GetClusterSize();
  CPPUNIT_ASSERT(clusSize == clusterSize);
  DWORD fc = fImageHeader.GetFileCount();
  CPPUNIT_ASSERT(fc == fileCount);

  offset = fImageHeader.GetVolumeDataOffset();
  volLength64 = fImageHeader.GetDataSize();
  ReadAndCheckBuffer(offset, (unsigned)volLength64, (BYTE*)volumeData, sizeof(volumeData));

  usedSize = fImageHeader.GetVolumeUsedSize();
  CPPUNIT_ASSERT(cUsedSize == usedSize);

  unsigned __int64 filSize = fImageHeader.GetFileSize();
  unsigned __int64 ref = sizeof(volumeData) + commentLength + length64 + verifyLength + sizeof(CImageFileHeader::TDiskImageFileHeader);
  CPPUNIT_ASSERT(filSize == ref);

  CloseHandle(fHandle);
  DeleteFile(fileName);
}

void FileHeaderTest::ValidFileHeaderTest()
{
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsValidFileHeader(), true);
  fImageHeader.fHeader.guid.Data4[7] = 0xff;
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsValidFileHeader(), false);
}

void FileHeaderTest::SupportedVersionTest()
{
  // check default version:
  fImageHeader.fHeader.versionMajor = CImageFileHeader::sVerMajor;
  fImageHeader.fHeader.versionMinor = CImageFileHeader::sVerMinor;
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedVersion(), true);
  // check higher minor version:
  fImageHeader.fHeader.versionMinor = CImageFileHeader::sVerMinor + 1;
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedVersion(), true);
  // check wrong minor version:
  fImageHeader.fHeader.versionMajor = CImageFileHeader::sVerMajor + 1;
  fImageHeader.fHeader.versionMinor = CImageFileHeader::sVerMinor;
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedVersion(), false);
}

void FileHeaderTest::SupportedChecksumMethodTest()
{
  fImageHeader.SetVerifyFormat(CImageFileHeader::verifyCRC32);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedChecksumMethod(), true);
  fImageHeader.SetVerifyFormat(CImageFileHeader::verifyNone);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedChecksumMethod(), true);
  fImageHeader.SetVerifyFormat((CImageFileHeader::VerifyFormat)3);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedChecksumMethod(), false);
}

void FileHeaderTest::SupportedCompressionFormatTest()
{
  fImageHeader.SetCompressionFormat(noCompression);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedCompressionFormat(), true);
  fImageHeader.SetCompressionFormat(compressionGZip);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedCompressionFormat(), true);
  fImageHeader.SetCompressionFormat(compressionBZIP2);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedCompressionFormat(), true);
  fImageHeader.SetCompressionFormat((TCompressionFormat)99);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedCompressionFormat(), false);
}

void FileHeaderTest::SupportedVolumeEncodingFormatTest()
{
  fImageHeader.SetVolumeBitmapInfo(CImageFileHeader::noVolumeBitmap, 0, 0);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedVolumeEncodingFormat(), true);
  fImageHeader.SetVolumeBitmapInfo(CImageFileHeader::simpleCompressedRunLength, 0, 0);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedVolumeEncodingFormat(), true);
  fImageHeader.SetVolumeBitmapInfo((CImageFileHeader::VolumeEncodingFormat)2, 0, 0);
  CPPUNIT_ASSERT_EQUAL(fImageHeader.IsSupportedVolumeEncodingFormat(), false);
}



// Helper functions

void FileHeaderTest::WriteCrc32Checksum(DWORD crc32, unsigned __int64 offset, unsigned length)
{
  CPPUNIT_ASSERT_EQUAL( Seek(offset, FILE_BEGIN), offset);
  unsigned count = Write((void*)&crc32Value, sizeof(crc32));
  CPPUNIT_ASSERT_EQUAL(count, sizeof(crc32));
}

void FileHeaderTest::WriteComment()
{
  unsigned count = Write((void*)comment, sizeof(comment));
  CPPUNIT_ASSERT_EQUAL(count, sizeof(comment));
}

void FileHeaderTest::WriteVolumeData()
{
  unsigned count = Write((void*)volumeData, sizeof(volumeData));
  CPPUNIT_ASSERT_EQUAL(count, sizeof(volumeData));
}

void FileHeaderTest::WriteAllocationBitmap()
// just write some bytes to simulate allocation bitmap
{
  unsigned count = Write((void*)allocBitmap, sizeof(allocBitmap));
  CPPUNIT_ASSERT_EQUAL(count, sizeof(allocBitmap));
}
  
unsigned FileHeaderTest::Read(void * buffer, unsigned nLength)
{
  unsigned nbytesRead;
  BOOL bSuccess = ReadFile(fHandle, buffer, nLength, (DWORD *)&nbytesRead, NULL) != FALSE;
  CPPUNIT_ASSERT_EQUAL(bSuccess, TRUE);
  return nbytesRead;
}

unsigned FileHeaderTest::Write(void *buffer, unsigned nLength)
{
  BOOL bSuccess;
  unsigned nWrote;

  bSuccess = WriteFile(fHandle, buffer, nLength, (unsigned long *)&nWrote, NULL) != FALSE;
  CPPUNIT_ASSERT_EQUAL(bSuccess, TRUE);
  return nWrote;
}

unsigned __int64 FileHeaderTest::Seek(__int64 Offset, DWORD MoveMethod)
{
  BOOL bSuccess;   
  LARGE_INTEGER pos, newOffset;
  pos.QuadPart = Offset;

  bSuccess = SetFilePointerEx(fHandle, pos, &newOffset, MoveMethod);
  CPPUNIT_ASSERT_EQUAL(bSuccess, TRUE);
  return newOffset.QuadPart;
}

void FileHeaderTest::ReadAndCheckBuffer(unsigned __int64 offset, unsigned length, BYTE* reference, unsigned refLength)
{
  BYTE* buffer = new BYTE[length];
  Seek(offset, FILE_BEGIN);
  unsigned count = Read((void*)buffer, length);
  CPPUNIT_ASSERT_EQUAL(count, length);
  CPPUNIT_ASSERT_EQUAL(count, refLength);
  int res = memcmp(buffer, reference, refLength);
  CPPUNIT_ASSERT_EQUAL(res, 0);
  delete [] buffer;
}
