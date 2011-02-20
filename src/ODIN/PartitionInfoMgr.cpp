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
 
/////////////////////////////////////////////////////////////////////////////
//
// Class to store, manage and retriebe the partition information of a disk
//
///////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include <string>
#include "PartitionInfoMgr.h"
#include "OSException.h"
#include "InternalException.h"
#include "FileFormatException.h"
#include "DriveUtil.h"
#include "..\..\src\ODIN\ImageStream.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

using namespace std;

const unsigned CPartitionInfoMgr::sMaxSupportedPartitions = 1000;

// {45A6FF26-9C51-43a4-B47E-A85D093DB487}
const GUID CPartitionInfoMgr::sMagicFileHeaderGUID = 
{ 0x45a6ff26, 0x9c51, 0x43a4, { 0xb4, 0x7e, 0xa8, 0x5d, 0x9, 0x3d, 0xb4, 0x87 } };

const WORD CPartitionInfoMgr::sVerMajor = 1;
const WORD CPartitionInfoMgr::sVerMinor = 0;

CPartitionInfoMgr::CPartitionInfoMgr()
{
  fBuffer = NULL;
  Reset();
}

CPartitionInfoMgr::~CPartitionInfoMgr()
{
  Reset();
}

void CPartitionInfoMgr::Reset()
{
  fPartitionCount = 0;
  fDiskSize = 0;
  delete fBuffer;
  fBuffer = NULL;
  ::ZeroMemory(&fBootLoader, sizeof(fBootLoader));
}

CPartitionInfoMgr::CPartitionInfoMgr(BYTE*buffer)
{
  fBuffer = NULL;
  Reset();
  fBuffer = buffer;
  fPartitionCount = ((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionCount;
}

// Read drive layout and partition table from a disk device
void CPartitionInfoMgr::ReadPartitionInfoFromDisk(const wchar_t* driveName)
{
  wstring driveDeviceName(driveName);
  long ntStatus;
  DWORD bytesRead ,dummy, res;
  HANDLE hDisk;

  bool isRawDisk = driveDeviceName.find(L"Partition0") != std::string::npos;
  if (!isRawDisk) {
    ATLTRACE("CPartitionInfoMgr::ReadPartitionInfo Error: %S is not a disk device.\n", driveName);
    return;
  }
  DWORD options = FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS | FILE_NON_DIRECTORY_FILE;
  DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  DWORD access = (GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE);
  ntStatus = NTOpen(&hDisk, driveDeviceName.c_str(), access, FILE_ATTRIBUTE_NORMAL, shareMode, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT|FILE_RANDOM_ACCESS|FILE_NON_DIRECTORY_FILE);
  ATLTRACE("Opening raw disk device: %S.\n", driveDeviceName.c_str());
  CHECK_KERNEL_EX_HANDLE_PARAM1(ntStatus, EWinException::volumeOpenError, driveDeviceName.c_str());
  ATLTRACE("Opened raw disk device: %S with handle %u\n", driveDeviceName, hDisk);

  // allocate buffer that is large enough
  DWORD bufferSize = sMaxSupportedPartitions * sizeof(PARTITION_INFORMATION_EX) + sizeof(DRIVE_LAYOUT_INFORMATION_EX);
  BYTE* guessedBuffer = new BYTE [bufferSize];
  res = DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, guessedBuffer, bufferSize, &dummy, NULL);
  if (res == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    CloseHandle(hDisk);  
    delete fBuffer;
    fBuffer = NULL;
    THROW_INT_EXC(EInternalException::maxPartitionNumberExceeded);
  }
  CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_GET_DRIVE_LAYOUT_EX");
  fPartitionCount = ((DRIVE_LAYOUT_INFORMATION_EX*) guessedBuffer)->PartitionCount;
  
  // reallocate buffer with the size that is really in use
  bufferSize = CalcBufferSize();
  fBuffer = new BYTE[bufferSize];
  CopyMemory(fBuffer, guessedBuffer, bufferSize);
  delete guessedBuffer;

  if (((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionStyle !=  PARTITION_STYLE_MBR) {
    ATLTRACE("ReadDriveLayout partition style %x not supported \n", ((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionStyle);
    CloseHandle(hDisk);  
    THROW_INT_EXC(EInternalException::unsupportedPartitionFormat);
  }

  // get size of disk
  GET_LENGTH_INFORMATION lengthInfo;
  res = DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &lengthInfo, sizeof(lengthInfo), &dummy, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_GET_LENGTH_INFO");
  fDiskSize = lengthInfo.Length.QuadPart;

  // Set extended read mode for disks in Vista (fails on Win XP, no problem in Vista)
  res = DeviceIoControl(hDisk, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &dummy, NULL);
  // if (res == 0)
  //  ; // no error checking here because a failure always occurs on Win XP

  // Read Boot Loader Code
  res = SetFilePointer(hDisk, 0, NULL, FILE_BEGIN);
  if (res == INVALID_SET_FILE_POINTER)
    THROW_OS_EXC_PARAM1((int)::GetLastError(), EWinException::seekError, driveName);
  
  res = ReadFile(hDisk, fBootLoader.bootLoader, sizeof(fBootLoader), (DWORD *)&bytesRead, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::readFileError, driveName);  
  
  res = CloseHandle(hDisk);  
  CHECK_OS_EX_INFO(res, EWinException::closeHandleError);
}

  // Write drive layout and partition table to a disk device
  // CAUTION: This makes the whole exsting content of the disk unreadable!!!
void CPartitionInfoMgr::WritePartitionInfoToDisk(const wchar_t* driveName)
{
  wstring driveDeviceName(driveName);
  DWORD dummy, res, bytesWritten;
  HANDLE hDisk;
  CDiskImageStream *p = new CDiskImageStream();
  CDiskImageStream& diskStream = *p; // we use this to lock the volumes and unmount

  bool isRawDisk = driveDeviceName.find(L"Partition0") != std::string::npos;
  if (!isRawDisk) {
    ATLTRACE("CPartitionInfoMgr::ReadPartionInfo Error: %S is not a disk device.\n", driveName);
    return;
  }
  diskStream.Open(driveName, IImageStream::forWriting);
  hDisk = diskStream.GetFileHandle();
  ATLTRACE("Opened raw disk device: %S with handle %u\n", driveDeviceName, hDisk);

  // Set extended read mode for disks in Vista (fails on Win XP, no problem in Vista)
  res = DeviceIoControl(hDisk, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &dummy, NULL);
  // if (res == 0)
  //  ; // no error checking here because a failure always occurs on Win XP: code 87 illegal parameter

  res = SetFilePointer(hDisk, 0, NULL, FILE_BEGIN);
  if (res == INVALID_SET_FILE_POINTER)
    THROW_OS_EXC_PARAM1((int)::GetLastError(), EWinException::seekError, driveName);
  res = WriteFile(hDisk, fBootLoader.bootLoader, sizeof(fBootLoader), (DWORD *)&bytesWritten, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::readFileError, driveName);  

  DWORD bufferSize = CalcBufferSize();
  res = DeviceIoControl(hDisk, IOCTL_DISK_SET_DRIVE_LAYOUT_EX, fBuffer, bufferSize, NULL, 0, &dummy, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_SET_DRIVE_LAYOUT_EX");

  // notify windows that the partition scheme has changed and that the drives may have changed
//  res = DeviceIoControl(hDisk, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &dummy, NULL);
//  CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_UPDATE_PROPERTIES");

  diskStream.Close();
}

 // return number of partitions in use
unsigned CPartitionInfoMgr::GetPartitionCount() const
{
  unsigned count = 0;
  PARTITION_INFORMATION_EX* partInfo = &(((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionEntry[0]);
  for (unsigned i=0; i<fPartitionCount; i++) {
    if (partInfo[i].PartitionNumber > 0)
      count++;
  }
  return count;
}


 // Read drive layout and partition table from a file
void CPartitionInfoMgr::ReadPartitionInfoFromFile(const wchar_t* fileName)
{
  TPartitionInfoFileHeader header;
  HANDLE hDisk;
  DWORD access     = GENERIC_READ;
  DWORD shareMode  = FILE_SHARE_READ;
  DWORD createMode = OPEN_EXISTING; 
  DWORD res;
  DWORD sizeRead;

  if (!fileName)
    return;

  hDisk = CreateFile(fileName, access, shareMode, NULL, createMode, FILE_ATTRIBUTE_NORMAL, NULL);
  CHECK_OS_EX_HANDLE_PARAM1(hDisk, EWinException::fileOpenError, fileName);

  Reset();

  // Read file header
  res = ReadFile(hDisk, &header, sizeof(header), &sizeRead, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::readFileError, L"");
  if (!IsValidFileHeader(header)) {
    CloseHandle(hDisk);
    THROW_FILEFORMAT_EXC(EFileFormatException::magicByteError);
  }
  if (!IsSupportedVersion(header)) {
    CloseHandle(hDisk);
    THROW_FILEFORMAT_EXC(EFileFormatException::majorVersionError);
  }

  // read boot loader block
  res = SetFilePointer(hDisk, header.bootLoaderOffset, NULL, FILE_BEGIN);
  if (res == INVALID_SET_FILE_POINTER)
    THROW_OS_EXC_PARAM1((int)::GetLastError(), EWinException::seekError, fileName);
  res = ReadFile(hDisk, fBootLoader.bootLoader, header.bootLoaderLength, &sizeRead, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::readFileError, L"");

  // read partition information
  res = SetFilePointer(hDisk, header.partInfoOffset, NULL, FILE_BEGIN);
  if (res == INVALID_SET_FILE_POINTER)
    THROW_OS_EXC_PARAM1((int)::GetLastError(), EWinException::seekError, fileName);
  fBuffer = new BYTE[header.partInfoLength];
  res = ReadFile(hDisk, fBuffer, header.partInfoLength, &sizeRead, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::readFileError, L"");
  if (sizeRead != header.partInfoLength) {
    CloseHandle(hDisk);
    THROW_FILEFORMAT_EXC(EFileFormatException::wrongFileSizeError);
  }
  fPartitionCount = header.partitionCount;
  fDiskSize = header.diskExtent;
  res = CloseHandle(hDisk);  
  CHECK_OS_EX_INFO(res, EWinException::closeHandleError);

  if (((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionStyle !=  PARTITION_STYLE_MBR) {
    ATLTRACE("ReadDriveLayout partition style %x not supported \n", ((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionStyle);
    THROW_INT_EXC(EInternalException::unsupportedPartitionFormat);
  }
}

// Write drive layout and partition table to a file
void CPartitionInfoMgr::WritePartitionInfoToFile(const wchar_t* fileName)
{
  HANDLE hDisk;
  TPartitionInfoFileHeader header;
  DWORD access     = GENERIC_READ | GENERIC_WRITE;
  DWORD shareMode  = FILE_SHARE_READ;
  DWORD createMode = CREATE_ALWAYS;
  DWORD sizeWritten, bufferSize, res;
  
  if (!fileName) 
    return;

  PrepareFileHeader(header);
  hDisk = CreateFile(fileName, access, shareMode, NULL, createMode, FILE_ATTRIBUTE_NORMAL, NULL);
  CHECK_OS_EX_HANDLE_PARAM1(hDisk, EWinException::fileOpenError, fileName);

  // write header information
  res = WriteFile(hDisk, &header, sizeof(header), &sizeWritten, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::writeFileError, L"");
  
  // write boot loader block  
  res = WriteFile(hDisk, fBootLoader.bootLoader, sizeof(fBootLoader.bootLoader), &sizeWritten, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::writeFileError, L"");  

  // write partition information  
  bufferSize = CalcBufferSize();
  if (fBuffer) {
    res = WriteFile(hDisk, fBuffer, bufferSize, &sizeWritten, NULL);
    CHECK_OS_EX_PARAM1(res, EWinException::writeFileError, L"");  
  }

  res = CloseHandle(hDisk);  
  CHECK_OS_EX_INFO(res, EWinException::closeHandleError);
}

// Mark all partitions as writable so that they can be written to the disk
void CPartitionInfoMgr::MakeWritable()
{
  PARTITION_INFORMATION_EX* partInfo = &(((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionEntry[0]);
  for (unsigned i=0; i<fPartitionCount; i++) {
    partInfo[i].RewritePartition = TRUE;
  }
}
  
bool CPartitionInfoMgr::IsValidFileHeader(const wchar_t* fileName)
{
  bool ok = true;
  HANDLE hDisk;
  TPartitionInfoFileHeader header;
  DWORD res;
  DWORD sizeRead;

  hDisk = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CHECK_OS_EX_HANDLE_PARAM1(hDisk, EWinException::fileOpenError, fileName);

  // Read file header
  res = ReadFile(hDisk, &header, sizeof(header), &sizeRead, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::readFileError, L"");
  res = CloseHandle(hDisk);  
  CHECK_OS_EX_INFO(res, EWinException::closeHandleError);
  
  if (!IsValidFileHeader(header)) {
    ok = false;
  }
  if (!IsSupportedVersion(header)) {
    ok = false;
  }

  return ok;
}

void CPartitionInfoMgr::ClearPartitionInfo(int startIndex)      // only for Unit tests, extremely dangerous!!
{
  PARTITION_INFORMATION_EX* partInfo = &(((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionEntry[0]);
  LARGE_INTEGER zero = {0};
  // first delete all logical drives on an extended volume
  for (unsigned i=startIndex; i<fPartitionCount; i++) {
    if (partInfo[i].PartitionLength.QuadPart > zero.QuadPart) {
      partInfo[i].RewritePartition = TRUE;
      // partInfo[i].StartingOffset = zero;
      partInfo[i].PartitionLength = zero;
      partInfo[i].PartitionNumber = 0;
      partInfo[i].PartitionStyle = (PARTITION_STYLE)0;
      partInfo[i].Mbr.BootIndicator = 0;
      partInfo[i].Mbr.HiddenSectors = 0;
      partInfo[i].Mbr.PartitionType = 0;
      partInfo[i].Mbr.RecognizedPartition = 0;
    }
  }
}


bool CPartitionInfoMgr::IsValidFileHeader(const TPartitionInfoFileHeader& header) const {
  return header.guid == sMagicFileHeaderGUID ? true : false;
}

bool CPartitionInfoMgr::IsSupportedVersion(const TPartitionInfoFileHeader& header) const {
  return header.versionMajor == sVerMajor; // we assume that all 1.x versions are supported
}

void CPartitionInfoMgr::PrepareFileHeader(TPartitionInfoFileHeader& header)
{
  header.guid = sMagicFileHeaderGUID;
  header.versionMajor = 1;
  header.versionMinor = 0;
  header.partitionCount = fPartitionCount;
  header.diskExtent = fDiskSize;
  header.bootLoaderOffset = sizeof(TPartitionInfoFileHeader);
  header.bootLoaderLength = sizeof(fBootLoader.bootLoader);
  header.partInfoOffset = header.bootLoaderOffset + header.bootLoaderLength;
  header.partInfoLength = CalcBufferSize();
}

DWORD CPartitionInfoMgr::CalcBufferSize() const
{
  return (fPartitionCount-1) * sizeof(PARTITION_INFORMATION_EX) + sizeof(DRIVE_LAYOUT_INFORMATION_EX);
}

void CPartitionInfoMgr::DumpPartitionInfo() 
{
  ATLTRACE("ReadDriveLayout found the following partitions:\n");
  PARTITION_INFORMATION_EX* partInfo = &(((DRIVE_LAYOUT_INFORMATION_EX*) fBuffer)->PartitionEntry[0]);
  ATLTRACE("ReadDriveLayout number of partitions found: %d \n", fPartitionCount);
  for (unsigned i=0; i<fPartitionCount; i++) {
    ATLTRACE("Partition[%u]\n", i);
    ATLTRACE("  style       : %u\n", partInfo[i].PartitionStyle);
    ATLTRACE("  start offset: %I64u\n", partInfo[i].StartingOffset);
    ATLTRACE("  length      : %I64u\n", partInfo[i].PartitionLength);
    ATLTRACE("  number      : %u\n", partInfo[i].PartitionNumber);
    ATLTRACE("  rewrite     : %u\n", partInfo[i].RewritePartition);
    ATLTRACE("  type        : %x\n", (unsigned) partInfo[i].Mbr.PartitionType);
    ATLTRACE("  boot indicator: %u\n", (unsigned) partInfo[i].Mbr.BootIndicator);
    ATLTRACE("  recognized    : %u\n", (unsigned) partInfo[i].Mbr.RecognizedPartition);
    ATLTRACE("  hidden sectors:   : %u\n", partInfo[i].Mbr.HiddenSectors);
  }
}