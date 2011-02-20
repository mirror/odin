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
#include <string>
#include "IImageStream.h"
#include "FileHeader.h"
#include "compression.h"

class CDiskImageStream;
class CompressedRunLengthStreamReader;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Interface for implementing callbacks to file operations
//////////////////////////////////////////////////////////////////////////////////////////////////
class IFileImageStreamCallback {
public:
  // callback handlers (a return value true indicates that processing should continue,
  // false indicates that no more processing should be performed after the callback
  virtual bool PreReadEvent(void * buffer, unsigned length, unsigned *bytesRead) = 0;
  virtual bool PreWriteEvent(void *buffer, unsigned length, unsigned *bytesWritten) = 0;
  virtual bool PreSeekEvent(__int64 Offset, DWORD MoveMethod) = 0;

  // we could define post events as well, but as we do not need them at the moment we omit them
  // virtual void PostReadEvent(void * buffer, unsigned nLength, unsigned *nBytesRead) = 0;
  // virtual void PostWriteEvent(void *buffer, unsigned nLength, unsigned *nBytesWritten) = 0;
  // virtual void PostSeekEvent(__int64 Offset, DWORD MoveMethod) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Subclass encapsulating reading/writing files containing ODIN images
//////////////////////////////////////////////////////////////////////////////////////////////////

class CFileImageStream : public IImageStream
{
  public:
  CFileImageStream();

  virtual ~CFileImageStream();

  virtual LPCWSTR GetName() const
  {
    return fFileName.c_str();
  }
  virtual void Open(LPCWSTR name, TOpenMode mode);
  virtual void Close();
  virtual void Read(void *buffer, unsigned nLength, unsigned *nBytesRead);
  virtual void Write(void *buffer, unsigned nLength, unsigned *nBytesWritten);
  virtual void Seek(__int64 Offset, DWORD MoveMethod);
  virtual unsigned __int64 GetPosition() const
  {
    return fPosition;
  }

  virtual bool IsDrive() const { // true if volume or harddisk, false if file
    return false;
  }

  virtual IRunLengthStreamReader* GetRunLengthStreamReader() const;
  virtual void SetCompletedInformation(DWORD crc32, unsigned __int64 processedBytes);

  bool inline  IsCompressed(void) const { 
	  return (fCompressionFormat != noCompression); 
	};
  
  void SetCompressionFormat(TCompressionFormat format) {
    fCompressionFormat = format;
  }


  unsigned __int64 GetSize() const {
      return fSize;
  }
  
  TOpenMode GetOpenMode() const {
    return fOpenMode;
  }

  unsigned __int64 GetAllocatedBytes() const {
      return fUsedSize;
  }

  HANDLE GetFileHandle() const {
    return fHandle;
  }
  
  const CImageFileHeader& GetImageFileHeader() const {
    return fImageHeader;
  }

  LPCWSTR GetComment() const {
    return fComment.c_str();
  }

  void SetComment(LPCWSTR comment) {
    fComment = comment;
  }

  void WriteCrc32Checksum(DWORD crc32);
  
  DWORD GetCrc32Checksum() const {
    return fCrc32;
  }

  CImageFileHeader::VolumeFormat GetVolumeFormat() const {
    return fVolumeFormat;
  }

  void SetVolumeFormat(CImageFileHeader::VolumeFormat volFormat) {
    fVolumeFormat = volFormat;
  }

  void RegisterCallback(IFileImageStreamCallback *callback) {
    fCallback = callback;
  }

  void UnegisterCallback() {
    fCallback = NULL;
  }

  void SetFileCount(unsigned newFileCount) {
    fFileCount = newFileCount;
  }

  unsigned GetFileCount() const {
    return fFileCount;
  }


  void ReadImageFileHeader(bool readAllocMap);
  void WriteImageFileHeaderAndAllocationMap(CDiskImageStream* volumeImageStore);
  void WriteImageFileHeaderForSaveAllBlocks(unsigned __int64 volumeSize, unsigned bytesPerCluster);
  void CheckIfInfoFromFileHeaderIsSupported();
  void ReadIntern(void * buffer, unsigned nLength, unsigned *nBytesRead);
  void WriteIntern(void *buffer, unsigned nLength, unsigned *nBytesWritten);
  void SeekIntern(__int64 offset, DWORD moveMethod);


private:
  unsigned __int64 StoreVolumeBitmap(unsigned int chunkSize, HANDLE hOutHandle);
  void WriteComment();
  void ReadComment();
  void ReadCrc32Checksum();

private:
  std::wstring       fFileName;
  std::wstring       fComment;
  TCompressionFormat fCompressionFormat;
  unsigned __int64   fPosition;
  TOpenMode          fOpenMode;
  HANDLE             fHandle;
  CImageFileHeader   fImageHeader;
  unsigned __int64   fSize;
  unsigned __int64   fUsedSize;  
  CompressedRunLengthStreamReader* fAllocMapReader;
  DWORD              fCrc32;
  IFileImageStreamCallback *fCallback;
  unsigned           fFileCount; // number of files the image file is split across
                                 // (information only used to store in header)
  CImageFileHeader::VolumeFormat fVolumeFormat; // type of image to be stored
  friend class CSplitManager;
};


//////////////////////////////////////////////////////////////////////////////////////////////////
// Subclass encapsulating reading/writing files containing disk devices to save or restore a disk
// partition or a complete disk
//////////////////////////////////////////////////////////////////////////////////////////////////
class CSubVolumeLocker;

class CDiskImageStream: public IImageStream
{
  public:
  
  typedef struct {
    __int64 Bytes;
    int BytesPerSector;
    int BytesPerCluster;
  } TTargetGeometry ;

  CDiskImageStream();
  CDiskImageStream(int volumeCount);
  virtual ~CDiskImageStream();
  
  virtual LPCWSTR GetName() const
  {
    return fName.c_str();
  }
  virtual void Open(LPCWSTR name, TOpenMode mode);
  virtual void Close();
  virtual unsigned __int64 GetPosition() const
  {
    return fPosition;
  }
  virtual void Read(void * buffer, unsigned nLength, unsigned *nBytesRead);
  virtual void Write(void *buffer, unsigned nLength, unsigned *nBytesWritten);
  virtual void Seek(__int64 Offset, DWORD MoveMethod);
  virtual bool IsDrive() const { // true if volume or harddisk, false if file
    return true;
  }
  virtual IRunLengthStreamReader* GetRunLengthStreamReader() const;
  virtual void SetCompletedInformation(DWORD crc32, unsigned __int64 processedBytes);

  unsigned __int64 StoreVolumeBitmap(unsigned int chunkSize, HANDLE hOutHandle, LPCWSTR fileName);
  void ReadDriveLayout();
  void UnlockSubVolume(int i);

  void SetContainedSubPartitionsCount(int containedVolumeCount) {
    fContainedVolumeCount = containedVolumeCount;
  }

  HANDLE GetFileHandle() const {
    return fHandle;
  }
  
  unsigned __int64 GetSize() const {
      return fSize;
  }

  void SetBytesPerCluster(unsigned bytesPerCluster) {
    fBytesPerCluster = bytesPerCluster;
  }

  unsigned GetBytesPerCluster() const {
    return fBytesPerCluster;
  }

  unsigned GetBytesPerClusterFromBootSector() const {
    return fBytesPerClusterFromBootSector;
  }

  unsigned __int64 GetAllocatedBytes() const {
      return fBytesUsed;
  }

  bool IsMounted() {
    return fIsMounted; // if partition is known then file system is supported
  }

private:

  // methods:
  void Init();
  void DismountAndLockVolume();
  long OpenDevice(DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE);
  void CloseDevice();
  void CalculateFATExtraOffset();
  void  CheckFileSystem();

  // types and fields:
  std::wstring       fName;
  unsigned __int64   fPosition;
  TOpenMode          fOpenMode;
  HANDLE             fHandle;
  unsigned __int64   fSize;     // number of total bytes in partition
  unsigned fBytesPerSector;
  unsigned fBytesPerCluster;
  unsigned fBytesPerClusterFromBootSector;
  unsigned __int64 fBytesUsed;  // number of used bytes in partition
  CompressedRunLengthStreamReader* fAllocMapReader;
  bool               fIsMounted;
  bool               fWasLocked; // true if volume could be locked succesfully
  BYTE               fPartitionType; // indicator of partition type (FAT16, FAT32, NTFS, ...)
  unsigned           fExtraOffset; // number of extra sectors to be saved before partition bitmap starts
  int                fContainedVolumeCount; // number of contained partitions in a physical disk
  CSubVolumeLocker*  fSubVolumeLocker;     // object maintaining locks of volumes contained in a physical disk
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Subclass encapsulating reading/writing files containing disk devices across a network
// connection: not yet implemented
//////////////////////////////////////////////////////////////////////////////////////////////////
class CNetImageStream: public IImageStream
{
public:
  
  CNetImageStream()
  {
  }

  virtual ~CNetImageStream()
  {
  }
  
  virtual LPCWSTR GetName() const;
  virtual void Open(LPCWSTR name, TOpenMode mode);
  virtual void Close();
  virtual unsigned __int64 GetPosition() const;
  virtual void Read(void * buffer, unsigned nLength, unsigned *nBytesRead);
  virtual void Write(void *buffer, unsigned nLength, unsigned *nBytesWritten);
  virtual void Seek(__int64 Offset, DWORD MoveMethod);
  virtual void SetCompletedInformation(DWORD crc32, unsigned __int64 processedBytes);
  virtual bool IsDrive() { // true if volume or harddisk, false if file
    return false;
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// A helper class used to lock/unlock all contained volumes of a physical disk
// (used by CDiskImageStream)
//////////////////////////////////////////////////////////////////////////////////////////////////
class CSubVolumeLocker
{
public:
  CSubVolumeLocker(LPCWSTR rootName, int containedVolumes);
  ~CSubVolumeLocker();

  void CloseAndUnlockVolume(int index);
  void OpenAndLockVolume(LPCWSTR volName, int index);

private:
  HANDLE* fHandles;
  int fSize;
};