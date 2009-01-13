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

#include <list>
#include "Compression.h"
#include "Config.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OdinManager a class managing the overall process for saving and resotring the disk images
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDriveInfo;
class CDriveList;
class COdinThread;
class CReadThread;
class CWriteThread;
class CImageBuffer;
class IImageStream;
class CSplitManager;
class ISplitManagerCallback;
class CVssWrapper;

class COdinManager
{
public:  

  COdinManager();
  ~COdinManager();
  void RefreshDriveList();
  void Terminate(bool bCancelled);
  void SavePartition(int driveIndex, LPCWSTR fileName, ISplitManagerCallback* cb);
  void RestorePartition(LPCWSTR fileName, int driveIndex, unsigned noFiles, unsigned __int64 totalSize, ISplitManagerCallback* cb);
  void VerifyPartition(LPCWSTR fileName, int driveIndex, unsigned noFiles, unsigned __int64 totalSize, ISplitManagerCallback* cb);
  void CancelOperation();
  unsigned GetThreadCount();
  bool GetThreadHandles(HANDLE* handles, unsigned size);
  void GetDriveNameList(std::list<std::wstring>& driveNames);
  LPCWSTR GetErrorMessage();
  bool WasError();
  unsigned __int64 GetTotalBytesToProcess();
  unsigned __int64 GetBytesProcessed();
  CDriveInfo* GetDriveInfo(int index);
  unsigned GetDriveCount();
  const CDriveList* GetDriveList() {
    return fDriveList;
  }
  
  unsigned __int64 GetSplitSize() const {
    return fSplitFileSize;
  }
  
  void SetSplitSize(unsigned __int64 splitSize) {
    fSplitFileSize = splitSize;
    fSplitFiles = splitSize !=0;
  }
  
  DWORD GetVerifiedChecksum() {
    return fVerifyCrc32;
  }

  TCompressionFormat GetCompressionMode() {
    return (TCompressionFormat)fCompressionMode();
  }
  
  void SetCompressionMode(TCompressionFormat mode) {
    fCompressionMode = mode;
  }

  bool GetSaveOnlyUsedBlocksOption() {
    return !fSaveAllBlocks;
  }

  void SetSaveOnlyUsedBlocksOption(bool saveOnlyUsedBlocks) {
    fSaveAllBlocks = ! saveOnlyUsedBlocks;
  }

  void SetComment(LPCWSTR comment) {
    fComment = comment;
  }

  LPCWSTR GetComment() const {
    return fComment.c_str();
  }

  int GetReadBlockSize() const {
    return fReadBlockSize;
  }

  bool IsRunning() const  {
    return fIsSaving || fIsRestoring;
  }

  void SetTakeSnapshotOption(bool makeSnapshot) {
    fTakeVSSSnapshot = makeSnapshot;
  }
  bool GetTakeSnapshotOption() {
    return fTakeVSSSnapshot;
  }

  void SetMultiVolumeMode (bool mode) {
    fMultiVolumeMode = mode;
  }

  bool GetMultiVolumeMode() const {
    return fMultiVolumeMode;
  }

  void MakeSnapshot(int driveIndex);
  void ReleaseSnapshot(bool bCancelled);

private:
  enum TImageStoreType { isUndefined, isFile, isDrive, isNBD };

  void Init();
  void Reset();
  void DoCopy(TImageStoreType sourceType, LPCWSTR fileIn, TImageStoreType targetType, LPCWSTR fileOut, 
    unsigned noFiles, unsigned __int64 totalSize, unsigned bytesPerCluster, ISplitManagerCallback* cb, 
    bool verifyOnly, int driveIndex);
  bool IsFileReadable(LPCWSTR fileName);

  CDriveList  *fDriveList;
  CReadThread *fReadThread;
  CWriteThread *fWriteThread;
  COdinThread *fCompDecompThread;
  IImageStream *fSourceImage;
  IImageStream *fTargetImage;
  CImageBuffer *fEmptyReaderQueue;
    // queue with empty blocks used by reader thread;
  CImageBuffer *fEmptyCompDecompQueue;
    // queue with empty blocks used by compression or decompression thread;
  CImageBuffer *fFilledReaderQueue;
    // queue with filled blocks filled by reader thread;
  CImageBuffer *fFilledCompDecompQueue;
    // queue with filled blocks filled by compression or decompression thread;
  bool fIsSaving;
    // currently saving of a partition is in progress
  bool fIsRestoring;
    // currently restoring of a partition is in progress
      
  CSplitManager* fSplitCallback;
    // callback object to handle splitting files in chunks
  DWORD fVerifyCrc32; // checksum after a verify run
  std::wstring fComment; // a comment used when storing a file
  CVssWrapper *fVSS;
  bool fMultiVolumeMode; 
    // COdinManager can run in one of two modes. if fMultiVolumeMode is set to false it
    // will create and release a VSS snapshot on its own as part of a DoCopy() operation.
    // If this option is set to true a user has control to take and release a snspshot.
    // This can be used to backup multiple volumes (e.g. all that are part of a physical
    // disk) within one snaphot. This option is only meaningful if fTakeVSSSnapshot is set
    // to true. Default is false.
  
  DECLARE_SECTION()
  DECLARE_ENTRY(int /*TCompressionFormat*/, fCompressionMode) // mode how to compress images
  DECLARE_ENTRY(bool, fSaveAllBlocks) // // save all/or only used blocks
  DECLARE_ENTRY(bool, fSplitFiles) // split files into chunks when writing
  DECLARE_ENTRY(unsigned __int64, fSplitFileSize) // size in bytes after which to split image files
  DECLARE_ENTRY(int, fReadBlockSize) // size in bytes to read from or write to disk in one chunk
  DECLARE_ENTRY(bool, fTakeVSSSnapshot)  // use VSS service to take a snapshot
};

