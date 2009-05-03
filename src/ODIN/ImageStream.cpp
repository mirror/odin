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
#include "ImageStream.h"
#include "OSException.h"
#include "InternalException.h"
#include "FileFormatException.h"
#include "CompressedRunLengthStream.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

// using namespace std;
/////////////////////////////////////////////////////////////////////////////////////
// Implementation of class CFileImageStream
/////////////////////////////////////////////////////////////////////////////////////

CFileImageStream::CFileImageStream()
{
  fHandle = NULL;
  fSize = fPosition = fCrc32 = fFileCount = 0;
  fAllocMapReader = NULL;
  fCallback = NULL;
}

CFileImageStream::~CFileImageStream()
{
  Close();
  delete fAllocMapReader;
}

void CFileImageStream::Open(LPCWSTR name, TOpenMode mode)
{
  DWORD access     = GENERIC_READ | (mode==forWriting?GENERIC_WRITE:0);
  DWORD shareMode  = FILE_SHARE_READ;
  DWORD createMode = (mode==forWriting?OPEN_ALWAYS:OPEN_EXISTING); 
  // note: use OPEN_ALWAYS and not CREATE_ALWAYS because file header is written later in an existing file!
  fOpenMode = mode;
  if (name) {
    fFileName = name;
    fHandle = CreateFile(name, access, shareMode, NULL, createMode, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK_OS_EX_HANDLE_PARAM1(fHandle, EWinException::fileOpenError, fFileName.c_str());
  }
}

void CFileImageStream::Close()
{
  if (fHandle != NULL && fHandle != INVALID_HANDLE_VALUE) {
    int res = CloseHandle(fHandle);  
    CHECK_OS_EX_INFO(res, EWinException::closeHandleError);
    fHandle = NULL;
  }
}


void CFileImageStream::Read(void * buffer, unsigned nLength, unsigned *nBytesRead)
{
  bool res=true;
  if (fCallback) {
    res = fCallback->PreReadEvent(buffer, nLength, nBytesRead);
  }
  if (res)
    ReadIntern(buffer, nLength, nBytesRead);
}

void CFileImageStream::Write(void *buffer, unsigned nLength, unsigned *nBytesWritten)
{
  bool res=true;
  if (fCallback) {
    res = fCallback->PreWriteEvent(buffer, nLength, nBytesWritten);
  }
  if (res)
    WriteIntern(buffer, nLength, nBytesWritten);
}

void CFileImageStream::Seek(__int64 offset, DWORD moveMethod)
{
  bool res=true;
  if (fCallback) {
    res = fCallback->PreSeekEvent(offset, moveMethod);
  }
  if (res)
    SeekIntern(offset, moveMethod);
}

void CFileImageStream::ReadIntern(void * buffer, unsigned nLength, unsigned *nBytesRead)
{
  BOOL bSuccess = ReadFile(fHandle, buffer, nLength, (DWORD *)nBytesRead, NULL) != FALSE;
  CHECK_OS_EX_PARAM1(bSuccess, EWinException::readFileError, fFileName.c_str());
  fPosition += *nBytesRead;
  // ATLTRACE("new read pos is %u\n", (unsigned)fPosition);
}

void CFileImageStream::WriteIntern(void *buffer, unsigned nLength, unsigned *nBytesWritten)
{
  BOOL bSuccess;
  unsigned nWrote;

  bSuccess = WriteFile(fHandle, buffer, nLength, (unsigned long *)&nWrote, NULL) != FALSE;
  CHECK_OS_EX_PARAM1(bSuccess, EWinException::writeFileError, fFileName.c_str());
  fPosition += nWrote;
  *nBytesWritten = nWrote;
}

void CFileImageStream::SeekIntern(__int64 offset, DWORD moveMethod)
{
  BOOL bSuccess;   
  LARGE_INTEGER pos, newOffset;
  
  pos.QuadPart = offset;
  bSuccess = SetFilePointerEx(fHandle, pos, &newOffset, moveMethod);
  CHECK_OS_EX_PARAM1(bSuccess, EWinException::seekError, fFileName.c_str());
  fPosition = newOffset.QuadPart;
}


void CFileImageStream::ReadImageFileHeader(bool readAllocMap)
{
  fImageHeader.ReadHeaderFromFile(fHandle);
  CheckIfInfoFromFileHeaderIsSupported();

  unsigned __int64 clusterBitmapOffset, clusterBitmapLength;
  fUsedSize = fImageHeader.GetVolumeUsedSize();
  fSize = fImageHeader.GetVolumeSize();

  if (readAllocMap) {
    fImageHeader.GetClusterBitmapOffsetAndLength(clusterBitmapOffset, clusterBitmapLength);
    if (clusterBitmapOffset != 0 && clusterBitmapLength != 0) {
      fAllocMapReader = new CompressedRunLengthStreamReader(fFileName.c_str(), clusterBitmapOffset, (DWORD)clusterBitmapLength);
    }
  }
  ReadCrc32Checksum();
  ReadComment();
}

void CFileImageStream::CheckIfInfoFromFileHeaderIsSupported()
{
  if (!fImageHeader.IsValidFileHeader())
    THROW_FILEFORMAT_EXC(EFileFormatException::magicByteError);
  if (!fImageHeader.IsSupportedVersion())
    THROW_FILEFORMAT_EXC(EFileFormatException::majorVersionError);
  if (!fImageHeader.IsSupportedChecksumMethod())
    THROW_FILEFORMAT_EXC(EFileFormatException::wrongChecksumMethod);
  if (!fImageHeader.IsSupportedCompressionFormat())
    THROW_FILEFORMAT_EXC(EFileFormatException::wrongCompressionMethod);
  if (!fImageHeader.IsSupportedVolumeEncodingFormat())
    THROW_FILEFORMAT_EXC(EFileFormatException::wrongVolumeEncodingMethod);
}

void CFileImageStream::WriteImageFileHeaderAndAllocationMap(CDiskImageStream* volumeImageStore)
{
  const int cReadChunkSize = 2 * 1024 * 1024;
  unsigned __int64 allocMapLength;
  unsigned __int64 usedSize = 0;
  unsigned __int64 volumeBitmapOffset = 0;

  // first write a default file header
  fImageHeader.WriteHeaderToFile(fHandle);
  Seek(0, FILE_END);
  fImageHeader.SetVerifyFormat(CImageFileHeader::verifyCRC32);
  fImageHeader.SetCompressionFormat(fCompressionFormat);
  fImageHeader.SetVerifyOffsetAndLength(fPosition, sizeof(DWORD));
  fImageHeader.SetVolumeType(fVolumeFormat);
  WriteCrc32Checksum(0); // dummy value just to reserve space at position in file
  WriteComment();
  volumeBitmapOffset = fPosition; 
  if (volumeImageStore) {
    allocMapLength = volumeImageStore->StoreVolumeBitmap(cReadChunkSize, fHandle, fFileName.c_str());
    fImageHeader.SetVolumeBitmapInfo(CImageFileHeader::simpleCompressedRunLength, volumeBitmapOffset, allocMapLength);
    usedSize  = volumeImageStore->GetAllocatedBytes();
  }
  fImageHeader.SetVolumeSize(volumeImageStore->GetSize());
  fImageHeader.SetVolumeDataOffset(allocMapLength + volumeBitmapOffset);
  fImageHeader.SetVolumeUsedSize(usedSize);
  fImageHeader.SetClusterSize(volumeImageStore->GetBytesPerCluster());
  // now write file header again after all information is complete
  fImageHeader.WriteHeaderToFile(fHandle);

  // update fPosition to file position
  LARGE_INTEGER pos, curPos;
  pos.QuadPart = 0LL;
  BOOL ok = SetFilePointerEx(fHandle, pos, &curPos, FILE_CURRENT);
  fPosition = curPos.QuadPart;
}

void CFileImageStream::WriteImageFileHeaderForSaveAllBlocks(unsigned __int64 volumeSize, unsigned bytesPerCluster)
{
  const int cReadChunkSize = 2 * 1024 * 1024;
  unsigned __int64 dataOffset;

  // first write a default file header
  fImageHeader.WriteHeaderToFile(fHandle);
  Seek(0, FILE_END);
  fImageHeader.SetVerifyFormat(CImageFileHeader::verifyCRC32);
  fImageHeader.SetCompressionFormat(fCompressionFormat);
  fImageHeader.SetVerifyOffsetAndLength(fPosition, sizeof(DWORD));
  fImageHeader.SetVolumeType(fVolumeFormat);
  WriteCrc32Checksum(0); // dummy value just to reserve space at position in file
  WriteComment();
  dataOffset = fPosition; 
  fImageHeader.SetVolumeBitmapInfo(CImageFileHeader::noVolumeBitmap, 0, 0);
  fImageHeader.SetVolumeSize(volumeSize);
  fImageHeader.SetVolumeDataOffset(dataOffset);
  fImageHeader.SetVolumeUsedSize(volumeSize);
  fImageHeader.SetClusterSize(bytesPerCluster);
  // now write file header again after all information is complete
  fImageHeader.WriteHeaderToFile(fHandle);

  // update fPosition to file position
  LARGE_INTEGER pos, curPos;
  pos.QuadPart = 0LL;
  BOOL ok = SetFilePointerEx(fHandle, pos, &curPos, FILE_CURRENT);
  fPosition = curPos.QuadPart;
}

void CFileImageStream::WriteCrc32Checksum(DWORD crc32) {
  unsigned byteCount = 0;
  unsigned __int64 pos;
  DWORD length;

  fImageHeader.GetVerifyOffsetAndLength(pos, length);
  if (pos != 0) {
    Seek(pos, FILE_BEGIN);
    Write(&crc32, length, &byteCount);
    Seek(0, FILE_END);
  }
}

void CFileImageStream::WriteComment() {
  unsigned byteCount = 0;
  unsigned __int64 pos;
  LPCWSTR buffer = fComment.c_str();
  unsigned commentLenBytes = (unsigned) (fComment.length() * sizeof(fComment[0]));

  if (!fComment.empty()) {
    Seek(0, FILE_END);
    pos = fPosition;
    Write((void*)buffer, commentLenBytes, &byteCount);
    Seek(0, FILE_END);
    fImageHeader.SetComment(pos, commentLenBytes);  
  }
}

void CFileImageStream::ReadComment() {
  unsigned __int64 oldOffset, offset;
  DWORD length;
  unsigned count;

  fImageHeader.GetCommentOffsetAndLength(offset, length);
  BYTE* buffer = new BYTE[length];

  // save old file position
  Seek(0, FILE_CURRENT);
  oldOffset = fPosition;

  Seek(offset, FILE_BEGIN);
  Read((void*)buffer, length, &count);
  // restore old file position
  Seek(oldOffset, FILE_BEGIN);

  if (count != length)
    THROW_FILEFORMAT_EXC(EFileFormatException::wrongCommentLength);
  fComment = std::wstring((LPCWSTR)buffer, length/sizeof(wchar_t));
  delete [] buffer;
}

void CFileImageStream:: ReadCrc32Checksum() {
  unsigned __int64 oldOffset, offset;
  DWORD length;
  unsigned count;
  
  fImageHeader.GetVerifyOffsetAndLength(offset, length);
  if (length != 4) 
    THROW_FILEFORMAT_EXC(EFileFormatException::wrongChecksumLength);
  BYTE* buffer = new BYTE[length];
  
  // save old file position
  Seek(0, FILE_CURRENT);
  oldOffset = fPosition;

  Seek(offset, FILE_BEGIN);
  Read((void*)buffer, length, &count);
  // restore old file position
  Seek(oldOffset, FILE_BEGIN);

  if (count != length)
    THROW_FILEFORMAT_EXC(EFileFormatException::wrongChecksumLength);
  fCrc32 = *(DWORD*)buffer;
  ATLTRACE("CRC32 from reading file header is. %u\n", fCrc32);
  delete [] buffer;
}

IRunLengthStreamReader* CFileImageStream::GetRunLengthStreamReader() const {
  return fAllocMapReader;
}

void CFileImageStream::SetCompletedInformation(DWORD crc32, unsigned __int64 processedBytes)
{
  WriteCrc32Checksum(crc32);
  fImageHeader.SetDataSize(processedBytes);
  fImageHeader.SetFileCount(fFileCount);
  Seek(0, FILE_BEGIN);
  fImageHeader.WriteHeaderToFile(fHandle);
  Seek(0, FILE_END);
}

#include <winioctl.h>
#include "DriveUtil.h"
/////////////////////////////////////////////////////////////////////////////////////
// Implementation of class CDiskImageStream
/////////////////////////////////////////////////////////////////////////////////////

CDiskImageStream::CDiskImageStream()
{
  fHandle = NULL;
  fBytesUsed = 0;
  fSize = 0;
  fAllocMapReader = NULL;
  fExtraOffset = 0;
  fPosition = 0;
  fContainedVolumeCount = 0;
  fSubVolumeLocker = NULL;
  fWasLocked = false;
}

CDiskImageStream::~CDiskImageStream()
{
  Close();
  delete fAllocMapReader;
}

void CDiskImageStream::ReadDriveLayout()
{
  // http://groups.google.com/group/microsoft.public.development.device.drivers/browse_thread/thread/3a52aa64b3845135
    DWORD dummy, res;
    // allocate buffer that is large enough
    DWORD bufferSize = 100 * sizeof(PARTITION_INFORMATION_EX) + sizeof(DRIVE_LAYOUT_INFORMATION_EX);
    BYTE* buffer = new BYTE [bufferSize];
    res = DeviceIoControl(fHandle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, buffer, bufferSize, &dummy, NULL);
    CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_GET_DRIVE_LAYOUT_EX");
    if (((DRIVE_LAYOUT_INFORMATION_EX*) buffer)->PartitionStyle !=  PARTITION_STYLE_MBR)
      ATLTRACE("ReadDriveLayout partition style %x not supported \n", ((DRIVE_LAYOUT_INFORMATION_EX*) buffer)->PartitionStyle);
    ATLTRACE("ReadDriveLayout found the following partitions:\n");
    PARTITION_INFORMATION_EX* partInfo = &(((DRIVE_LAYOUT_INFORMATION_EX*) buffer)->PartitionEntry[0]);
    unsigned partitionCount = ((DRIVE_LAYOUT_INFORMATION_EX*) buffer)->PartitionCount;
    ATLTRACE("ReadDriveLayout number of partitions found: %d \n", partitionCount);
    for (unsigned i=0; i<partitionCount; i++) {
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
    delete buffer;
}

long CDiskImageStream::OpenDevice(DWORD shareMode)
{
  long ntStatus;
  DWORD access = (fOpenMode==forWriting) ? (GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE) : GENERIC_READ| SYNCHRONIZE;
  ATLTRACE("Opening raw disk device: %S.\n", fName.c_str());
  ntStatus = NTOpen(&fHandle, fName.c_str(), access, FILE_ATTRIBUTE_NORMAL, shareMode, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT|FILE_RANDOM_ACCESS|FILE_NON_DIRECTORY_FILE);
  return ntStatus;
}

void CDiskImageStream::CloseDevice()
{
  if (fHandle != NULL) {
    DWORD res = CloseHandle(fHandle);  
    CHECK_OS_EX_INFO(res, EWinException::closeHandleError)
    fHandle = NULL;
  }
}

void CDiskImageStream::Open(LPCWSTR name, TOpenMode mode)
{ 
  PARTITION_INFORMATION_EX partInfo;
  DISK_GEOMETRY diskGeometry;
  GET_LENGTH_INFORMATION lengthInfo;
  DWORD dummy, res;
  long ntStatus;
  bool isRawDisk;
  DWORD shareMode, options;
  
  fName = name;
  fOpenMode = mode;
  isRawDisk = fName.find(L"Partition0") != std::string::npos;
  options = FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS | FILE_NON_DIRECTORY_FILE;
  if (isRawDisk) {
    shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    ntStatus = OpenDevice(shareMode);
    CHECK_KERNEL_EX_HANDLE_PARAM1(ntStatus, EWinException::volumeOpenError, fName.c_str());
    ATLTRACE("Opened raw disk device: %S with handle %u\n", name, fHandle);
    if (fContainedVolumeCount > 0) {
      std::wstring rootName = fName.substr(0, fName.length() - 1);
      fSubVolumeLocker = new CSubVolumeLocker(rootName.c_str(), fContainedVolumeCount);
    }
    ReadDriveLayout();
  } else {
    shareMode  = (mode==forWriting) ? 0 : FILE_SHARE_WRITE;
    ntStatus = OpenDevice(shareMode);
    if (0 != ntStatus) {
      Sleep(1000);
        // sometimes this calls fails just try again after a little pause
      ntStatus = OpenDevice(shareMode);
    }
    CHECK_KERNEL_EX_HANDLE_PARAM1(ntStatus, EWinException::volumeOpenError, fName.c_str());
  }

  memset(&partInfo, 0, sizeof(partInfo));
  memset(&diskGeometry, 0, sizeof(diskGeometry));
  memset(&lengthInfo, 0, sizeof(lengthInfo));
  res = DeviceIoControl(fHandle, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partInfo, sizeof(partInfo), &dummy, NULL);
  //CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_GET_PARTITION_INFO_EX");
  // on some disks this might fail try PARTITION_INFORMATION then
  if (res==0) {
    PARTITION_INFORMATION partInfo2;
    memset(&partInfo, 0, sizeof(partInfo));
    res = DeviceIoControl(fHandle, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &partInfo2, sizeof(partInfo2), &dummy, NULL);
    CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_GET_PARTITION_INFO_EX and IOCTL_DISK_GET_PARTITION_INFO");
    fPartitionType = partInfo2.PartitionType;
  }
  else {
    CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_GET_DRIVE_GEOMETRY");
    fPartitionType = partInfo.Mbr.PartitionType;
  }

  fIsMounted = DeviceIoControl(fHandle, FSCTL_IS_VOLUME_MOUNTED, NULL, 0, NULL, 0, &dummy, NULL) != FALSE;
    
  res = DeviceIoControl(fHandle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &lengthInfo, sizeof(lengthInfo), &dummy, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_GET_LENGTH_INFO");
  res = DeviceIoControl(fHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY	, NULL, 0, &diskGeometry, sizeof(diskGeometry), &dummy, NULL);
  fSize = lengthInfo.Length.QuadPart;
  ATLTRACE("Size of volume %S is %u\n", fName.c_str(), (DWORD)fSize);
  fBytesPerSector = diskGeometry.BytesPerSector;
  // Note: for NTFS use: FSCTL_GET_NTFS_VOLUME_DATA to get cluster size
  //       for FAT use getDiskFreeSpaceEx (see KB 231497,http://support.microsoft.com/?scid=kb%3Ben-us%3B231497&x=21&y=17)
  if (fOpenMode == forReading)
    CalculateFATExtraOffset(); // FAT has some sectors before the bitmap starts counting

  // note: under Vista/Server2008 there are new limitations in the direct access of disks
  // there are reserved areas that can not be written to even with admin rights, see
  // http://support.microsoft.com/kb/942448 for details. Therefore we unmount volume
  //http://msdn.microsoft.com/newsgroups/default.aspx?dg=microsoft.public.win32.programmer.kernel&tid=d0ff3e7a-e32c-49dc-b3e6-6cbdd1da67ac&cat=en-us-msdn-windev-winsdk&lang=en&cr=US&sloc=en-us&m=1&p=1
  if (fOpenMode==forWriting) {
    res = DeviceIoControl(fHandle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
    CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"FSCTL_LOCK_VOLUME");
    res = DeviceIoControl(fHandle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
    CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"FSCTL_DISMOUNT_VOLUME");
    fWasLocked = true;
    if (!isRawDisk) { // does not work for raw disks in Win XP, no problem in Vista
      res = DeviceIoControl(fHandle, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &dummy, NULL);
      if (res == 0)
        ATLTRACE("Warning: DeviceIoControl with FSCTL_ALLOW_EXTENDED_DASD_IO failed.\n");
    }
  }
}

void CDiskImageStream::Close()
{
  DWORD res, dummy;
  NTFS_VOLUME_DATA_BUFFER volData;
  // if it is a physical disk unlock all contained volumes
  if (fSubVolumeLocker) {
    delete fSubVolumeLocker;
    fSubVolumeLocker = NULL;
  }
  
  // close handle and unlock volume
  if (fHandle != NULL && fHandle != INVALID_HANDLE_VALUE) {
    if (fOpenMode==forWriting && fWasLocked) {
      // inform Windows to reload and remount the drives
      res = DeviceIoControl(fHandle, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &dummy, NULL);
      CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_DISK_UPDATE_PROPERTIES");
      res = DeviceIoControl(fHandle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
      CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"FSCTL_UNLOCK_VOLUME");
      ATLTRACE("Closed raw disk device: %S with handle %u\n", fName.c_str(), fHandle);
    }

    CloseDevice();

    // resize partition if necessary and supported (we must close device first and then open again otherwise it fails)
    // only supported on NTFS not on FAT
    bool isRawDisk = fName.find(L"Partition0") != std::string::npos;
    if ( fOpenMode == forWriting && !isRawDisk) {
      long ntStatus = OpenDevice(0);
      if (0 != ntStatus) {
        Sleep(1000);
        // sometimes this calls fails just try again after a little pause
        ntStatus = OpenDevice(0);
      }
      CHECK_KERNEL_EX_HANDLE_PARAM1(ntStatus, EWinException::volumeOpenError, fName.c_str());
      ZeroMemory(&volData, sizeof(volData));
      res = DeviceIoControl(fHandle, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &volData, sizeof(volData), &dummy, NULL);
      LONGLONG newVolSize = fSize / fBytesPerCluster;
      if (res && volData.TotalClusters.QuadPart != newVolSize) {
        res = DeviceIoControl(fHandle, FSCTL_EXTEND_VOLUME, &newVolSize, sizeof(newVolSize), NULL, 0, &dummy, NULL);
        // CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"FSCTL_EXTEND_VOLUME");
      }
      CloseDevice();
    }
  }
}

//---------------------------------------------------------------------------
// Read() - As the name suggests, read from the file/device.  Special care
// has to be taken when reading from handles created with "NtCreateFile()".
// Windows versions based on NT have problems with partial reads of disk
// devices opened with NtCreateFile().  When there isn't enough data in the
// file to satisfy the entire length of the read, ReadFile() is supposed to
// always read as many as there are.  Disk devices of the type
// "\Device\Harddisk0\Partition1" opened with NtCreateFile don't do this.
// When there isn't enough data left to satisfy the entire read, ReadFile()
// will fail entirely and GetLastError() will return ERROR_INVALID_PARAMETER.
// To compensate for this, whenever we encounter ERROR_INVALID_PARAMETER, we
// step down the size of the read to the device's sector size and then read
// until we get an error again.
//
// The same ERROR_INVALID_PARAMETER error will be returned in cases where
// the read length is not a multiple of the disk's sector size.  The same
// step down code gets around this by using a temporary buffer to read
// into a sector at a time, and them copies the exact number of bytes
// requested out after reading the sector that puts it past the requested
// length.  In this case, nbytesRead will be set to the requested length,
// but the read position will be at the next sector boundary.

void CDiskImageStream::Read(void * buffer, unsigned nLength, unsigned *nbytesRead)
{
  BOOL bSuccess;
  DWORD nbytesReadTemp;

  if ((unsigned __int64)nLength > fSize - fPosition) // we will read beyond end of partition--> results in error see above
    nLength = (DWORD)(fSize - fPosition);

  bSuccess = ReadFile(fHandle, buffer, nLength, &nbytesReadTemp, NULL) != FALSE;
  CHECK_OS_EX_PARAM1(bSuccess, EWinException::readVolumeError, fName.c_str());
  //ATLTRACE("Read(): read bytes expected: %u, read: %u\n", nLength, nbytesReadTemp);
  
  fPosition += nbytesReadTemp;
  *nbytesRead = nbytesReadTemp;
}

void CDiskImageStream::Write(void *buffer, unsigned nLength, unsigned *nBytesWritten)
{
  BOOL bSuccess;
  DWORD nWrote;

  bSuccess = WriteFile(fHandle, buffer, nLength, &nWrote, NULL) != FALSE;
  CHECK_OS_EX_PARAM1(bSuccess, EWinException::writeVolumeError, fName.c_str());
  fPosition += nWrote;
  *nBytesWritten = nWrote;
}

void CDiskImageStream::Seek(__int64 offset, DWORD moveMethod)
{
  BOOL bSuccess;
  LARGE_INTEGER pos, newOffset;
  pos.QuadPart = offset;
  bSuccess = SetFilePointerEx(fHandle, pos, &newOffset, moveMethod);
  CHECK_OS_EX_PARAM1(bSuccess, EWinException::seekError, fName.c_str());
  fPosition = newOffset.QuadPart;
}

IRunLengthStreamReader* CDiskImageStream::GetRunLengthStreamReader() const {
  return fAllocMapReader;
}

/* for debugging extra offset for FAT partitions
void  CDiskImageStream::GetVolumeBitmapInfo()
{
  DWORD chunkSize = 1024;
  unsigned nBitmapBytes;
  VOLUME_BITMAP_BUFFER *volumeBitmap;
  DWORD noClustersPerChunk, nBytesReturned;
  unsigned __int64 startCluster=0;
  bool run = true;
  noClustersPerChunk = chunkSize * 8;
  unsigned __int64 i = 0;
  nBitmapBytes = (noClustersPerChunk + 7) / 8;
  // nBitmanBytes is actually one larger than we need because there is one bitmap byte already in the VOLUME_BITMAP_BUFFER
  // struct. 
  volumeBitmap = (VOLUME_BITMAP_BUFFER *)malloc(sizeof(VOLUME_BITMAP_BUFFER) + nBitmapBytes - 1);
  memset(volumeBitmap, 0, sizeof(VOLUME_BITMAP_BUFFER));

  while (run ) {
      int ret = DeviceIoControl(fHandle, FSCTL_GET_VOLUME_BITMAP, &startCluster, sizeof(startCluster), volumeBitmap, sizeof(VOLUME_BITMAP_BUFFER) + nBitmapBytes - 1, &nBytesReturned, NULL);
      if (ret == 0 && GetLastError() != ERROR_MORE_DATA)
        CHECK_OS_EX_PARAM1(ret, EWinException::ioControlError, L"FSCTL_GET_VOLUME_BITMAP");

      if (volumeBitmap->StartingLcn.QuadPart != startCluster) {
        ATLTRACE("wrong start cluster\n");
        break;
      }
      i += volumeBitmap->BitmapSize.QuadPart;
      startCluster += noClustersPerChunk;
      run = noClustersPerChunk == i;
   }
  free(volumeBitmap);
  ATLTRACE("Size of FSCTL_GET_VOLUME_BITMAP bitmap is: %I64u\n", i);
  ATLTRACE("Size of volume in bytes FSCTL_GET_VOLUME_BITMAPfrom  is: %I64u\n", i*fBytesPerCluster);
  ATLTRACE("Size of volume in bytes fSize is: %I64u\n", fSize);
  ATLTRACE("Size of volume in clusters from fSize is: %I64u\n", fSize/fBytesPerCluster);
  ATLTRACE("Difference in clusters is: %I64u, difference in sectors is: %u\n", fSize/fBytesPerCluster - i, (fSize/fBytesPerCluster - i) * fBytesPerCluster / fBytesPerSector);
}
*/

unsigned __int64 CDiskImageStream::StoreVolumeBitmap(unsigned int chunkSize, HANDLE hOutHandle, LPCWSTR fileName)
{
  unsigned nBitmapBytes;
  VOLUME_BITMAP_BUFFER *volumeBitmap;
  unsigned __int64 startCluster, stopCluster;
  unsigned noClustersPerChunk, noClusters;
  DWORD nBytesReturned;
  unsigned __int64 bitmapSize, startOffset;
  LARGE_INTEGER newPos, curPos;
  BOOL ok;

  CompressedRunLengthStreamWriter writer(hOutHandle, fBytesPerCluster ? fExtraOffset * fBytesPerSector / fBytesPerCluster : 0);
  newPos.QuadPart = 0LL;
  ok = SetFilePointerEx(hOutHandle, newPos, &curPos, FILE_END);
  startOffset = curPos.QuadPart;
   
  noClustersPerChunk = chunkSize * 8;
  noClusters = (unsigned) (fSize / fBytesPerCluster);
  unsigned noIterations = (unsigned) (noClusters + noClustersPerChunk - 1)/ noClustersPerChunk;

  // Determine the number of clusters in a chunk, and the number of cluster bitmap bytes that are taken by a chunk.
  // noClustersPerChunk = (chunkSize + fBytesPerCluster - 1) / fBytesPerCluster;
  nBitmapBytes = (noClustersPerChunk + 7) / 8;
  // nBitmanBytes is actually one larger than we need because there is one bitmap byte already in the VOLUME_BITMAP_BUFFER
  // struct. 
  volumeBitmap = (VOLUME_BITMAP_BUFFER *)malloc(sizeof(VOLUME_BITMAP_BUFFER) + nBitmapBytes - 1);
  memset(volumeBitmap, 0, sizeof(VOLUME_BITMAP_BUFFER));
  startCluster = stopCluster = 0;

  for (unsigned i=0; i<noIterations; i++ ) {
      stopCluster = startCluster + noClustersPerChunk;
      int ret = DeviceIoControl(fHandle, FSCTL_GET_VOLUME_BITMAP, &startCluster, sizeof(startCluster), volumeBitmap, sizeof(VOLUME_BITMAP_BUFFER) + nBitmapBytes - 1, &nBytesReturned, NULL);
      if (ret == 0 && GetLastError() != ERROR_MORE_DATA)
        CHECK_OS_EX_PARAM1(ret, EWinException::ioControlError, L"FSCTL_GET_VOLUME_BITMAP");

      if (volumeBitmap->StartingLcn.QuadPart != startCluster)
        THROW_INT_EXC(EInternalException::volumeBitmapBufferSizeError);

      startCluster += noClustersPerChunk;
      writer.AddBuffer(&(volumeBitmap->Buffer[0]), (nBytesReturned - sizeof(VOLUME_BITMAP_BUFFER) + 1) * 8);
   }
  writer.Flush();
  free(volumeBitmap);
  ok = SetFilePointerEx(hOutHandle, newPos, &curPos, FILE_END);
  CHECK_OS_EX_PARAM1(ok, EWinException::seekError, L"");

  bitmapSize = curPos.QuadPart - startOffset;
  ATLTRACE("Size of cluster allocation bitmap is: %I64u\n", bitmapSize);

  fBytesUsed = writer.Get1Count() * fBytesPerCluster;
  ATLTRACE("Number of bytes in use: %I64u\n", fBytesUsed);
  ATLTRACE("Number of bytes free: %I64u\n", writer.Get0Count() * fBytesPerCluster);

  // create a reader instance that can be used later to read the cluster bitmap values
  fAllocMapReader = 
    new CompressedRunLengthStreamReader(fileName, startOffset, (DWORD) bitmapSize);

  return bitmapSize;
}

void CDiskImageStream::SetCompletedInformation(DWORD crc32, unsigned __int64 processedBytes)
{
  // ignore nothing to do
}

void CDiskImageStream::CalculateFATExtraOffset()
{
  // FAT partitions do not start the logical cluster numbers counting at offset 0 of the partition
  // the reserved sectors in the beginning must be taken extra into account. To calculate the
  // offset we need to read the boot sector. For NTFS this extra offset is 0.
  DWORD lcn0Offset;

#pragma pack(push,1)
  struct TWinBootSector {
    unsigned char Jump[3];
    char          OEMID[8];
    WORD          BytesPerSector;
    BYTE          SectorsPerCluster;
    WORD          ReservedSectors;
    BYTE          NumberOfFATs;
    WORD          RootEntries;
    WORD          FAT_TotalSectors;
    BYTE          MediaDescriptor;
    WORD          SectorsPerFAT;
    WORD          SectorsPerTrack;
    WORD          Heads;
    DWORD         HiddenSectors;
    DWORD         FAT_BigTotalSectors;
    DWORD         BigSectorsPerFAT;
    LONGLONG      TotalSectors;
    LONGLONG      MFTCluster;
    LONGLONG      MFTMirrorCluster;
    DWORD         ClustersPerFileRecord;
    DWORD         ClustersPerIndexBlock;
    LONGLONG      VolumeSerial;
    DWORD         Checksum;
    unsigned char Bootstrap[426];
    WORD          EndOfSector;
    unsigned char Cruft[1536];
  };

#pragma pack(pop)

  TWinBootSector *bootSector = NULL;
  unsigned int bytesRead;

  bootSector = new TWinBootSector;

  Read(bootSector, sizeof(TWinBootSector), &bytesRead);
  Seek(0, FILE_BEGIN);
  if (bytesRead == sizeof(TWinBootSector)) {
      ATLTRACE("Bytes per cluster from boot sector is: %u\n", bootSector->SectorsPerCluster * fBytesPerSector);
      ATLTRACE("CalculateFATExtraOffset() OEMID is: %s\n", bootSector->OEMID);
      ATLTRACE("CalculateFATExtraOffset() partition type is: %d\n", fPartitionType);
    if ((!memcmp(bootSector->OEMID, "NTFS    ", 8) || !memcmp(bootSector->OEMID, "MSDOS5.0", 8))) {
      // fBytesPerCluster = bootSector->SectorsPerCluster * fBytesPerSector;
		  DWORD sectorsPerFAT = 0;

      // PARTITION_HUGE??? 
      if (fPartitionType == PARTITION_FAT_16 || fPartitionType == PARTITION_HUGE)
        sectorsPerFAT = bootSector->SectorsPerFAT;
      else if (fPartitionType == PARTITION_FAT32 || fPartitionType == PARTITION_FAT32_XINT13)
        sectorsPerFAT = bootSector->BigSectorsPerFAT;
      else // must be NTFS
        ;
		  
      if (sectorsPerFAT > 0) {
        ATLTRACE("Detected a FAT16 or FAT32 partition\n");
        // Calculate RootDirSectors
		    DWORD rootDirSectors;
		    rootDirSectors = ((bootSector->RootEntries * 32) + (fBytesPerSector - 1)) / fBytesPerSector;
        ATLTRACE("root dir sectors: %u\n", rootDirSectors);

		    // Calculate offset of logical cluster number 0 (LCN 0)

		    lcn0Offset = bootSector->ReservedSectors + (bootSector->NumberOfFATs * sectorsPerFAT) /* + rootDirSectors*/;
        ATLTRACE("logical cluster number 0 offset is: %u\n", lcn0Offset);
        ATLTRACE("logical cluster number 0 offset in bytes: %u\n", (lcn0Offset * fBytesPerSector));
        ATLTRACE("logical cluster number 0 offset with root dir sectors is: %u\n", lcn0Offset+ rootDirSectors);
        ATLTRACE("logical cluster number 0 offset in bytes with root dir sectors is: %u\n", (lcn0Offset+ rootDirSectors) * fBytesPerSector);
        fExtraOffset = lcn0Offset+ rootDirSectors;
      }
    }  // if (is NTFS/FAT32)
  }  
  // If we didn't end up with a filesystem we know about, then delete the boot sector.  Otherwise we keep it.
  delete bootSector;
  bootSector = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// A helper class used to lock/unlock all contained volumes of a physical disk
// (used by CDiskImageStream)
//////////////////////////////////////////////////////////////////////////////////////////////////

CSubVolumeLocker::CSubVolumeLocker(LPCWSTR rootName, int containedVolumes) {
  fHandles = new HANDLE[containedVolumes];
  const int bufferSize = 10;
  wchar_t buffer[bufferSize];

  fHandles = new HANDLE[containedVolumes];
  fSize = containedVolumes;

  for (int i=0; i<containedVolumes; i++)
    fHandles[i] = NULL;
  for (int i=0; i<containedVolumes; i++) {
    std::wstring volName = rootName;
    _itow_s(i+1, buffer, 10, bufferSize);
    volName += buffer;
    OpenAndLockVolume(volName.c_str(), i);
  }
}
  
CSubVolumeLocker::~CSubVolumeLocker() {
  // for (int i=0; i<fSize; i++)
  for (int i=fSize-1; i>=0; i--)
    CloseAndUnlockVolume(i);
  delete [] fHandles;
}

void CSubVolumeLocker::OpenAndLockVolume(LPCWSTR volName, int index) {
  DWORD res, access, dummy;
  long ntStatus;
  HANDLE h;

  access     = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
  DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

  ATLTRACE("Opening sub volume: %S.\n", volName);
  ntStatus = NTOpen(&h, volName, access, FILE_ATTRIBUTE_NORMAL, shareMode, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT|FILE_RANDOM_ACCESS|FILE_NON_DIRECTORY_FILE);
  CHECK_KERNEL_EX_HANDLE_PARAM1(ntStatus, EWinException::volumeOpenError, volName);
  ATLTRACE("Opened  sub volume: %S with handle %u\n", volName, h);
  fHandles[index] = h;
  res = DeviceIoControl(h, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"FSCTL_LOCK_VOLUME");
  res = DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
  CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"FSCTL_DISMOUNT_VOLUME");
}

void CSubVolumeLocker::CloseAndUnlockVolume(int index) {
  HANDLE h = fHandles[index];
  if (h != NULL && h != INVALID_HANDLE_VALUE) {
      DWORD res, dummy;
      // res = DeviceIoControl(h, IOCTL_VOLUME_ONLINE, NULL, 0, NULL, 0, &dummy, NULL);
      // CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"IOCTL_VOLUME_ONLINE");
      res = DeviceIoControl(h, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
      CHECK_OS_EX_PARAM1(res, EWinException::ioControlError, L"FSCTL_UNLOCK_VOLUME");
      res = CloseHandle(h);  
      CHECK_OS_EX_INFO(res, EWinException::closeHandleError);
      ATLTRACE("Closed sub volume  with handle %u\n", h);

  }
}

