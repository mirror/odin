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
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OdinManager a class managing the overall process for saving and resotring the disk images
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OdinThread.h"
#include "WriteThread.h"
#include "ReadThread.h"
#include "CompressionThread.h"
#include "DecompressionThread.h"
#include "BufferQueue.h"
#include "ImageStream.h"
#include "OdinManager.h"
#include "DriveList.h"
#include "InternalException.h"
#include "SplitManager.h"
#include "VSSWrapper.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

// section name in .ini file for configuration values
IMPL_SECTION(COdinManager, L"Options")

COdinManager::COdinManager()
  :fCompressionMode(L"CompressionMode", noCompression),
   fSaveAllBlocks(L"SaveAllBlocks", false),
   fSplitFiles(L"SplitFiles", false),
   fSplitFileSize(L"SplitFileSize", 0),
   fReadBlockSize(L"ReadWriteBlockSize", 1048576), // 1MB
   fTakeVSSSnapshot(L"TakeVSSSnaphot", false)
{
  fDriveList = NULL;
  fVerifyCrc32 = 0;
  fVSS = NULL;
  Init();
}

COdinManager::~COdinManager()
{
  delete fDriveList;
  Reset();
}

void COdinManager::Init()
{
  fReadThread = NULL;
  fWriteThread = NULL;
  fCompDecompThread = NULL;
  fSourceImage = NULL;
  fTargetImage = NULL;
  fEmptyReaderQueue = NULL;
  fFilledReaderQueue = NULL;
  fEmptyCompDecompQueue = NULL;
  fFilledCompDecompQueue = NULL;
  fSplitCallback = NULL;
  fIsSaving = false;
  fIsRestoring = false;
  fVSS = NULL;
  fMultiVolumeMode = false;
  if (!CVssWrapper::VSSIsSupported()) {
    fTakeVSSSnapshot = false;
  }
}
  
void COdinManager::RefreshDriveList()
{
  delete fDriveList;
  fDriveList = new CDriveList(false);
}

  // Terminates all worker threads, bCancelled indicates if this is 
  // becuse a running operation was aborted.
void COdinManager::Terminate(bool bCancelled)
{
  if (NULL != fReadThread) {
    fVerifyCrc32 = fReadThread->GetCrc32();
    fReadThread->Terminate();
  }
  if (NULL != fWriteThread) {
    fWriteThread->Terminate();
  }
  if (NULL != fCompDecompThread) {
    fCompDecompThread->Terminate();
  }
  if (NULL != fSplitCallback) {
    delete fSplitCallback;
  }
  if (fVSS && !fMultiVolumeMode) {
    fVSS->ReleaseSnapshot(bCancelled);
    delete fVSS;
    fVSS = NULL;
  }
  Reset();
}

void COdinManager::Reset()
{
  delete fReadThread;
  fReadThread = NULL;
  delete fWriteThread;
  fWriteThread = NULL;
  delete fCompDecompThread;
  fCompDecompThread = NULL;
  delete fSourceImage;
  fSourceImage = NULL;
  delete fTargetImage;
  fTargetImage = NULL;
  delete fEmptyReaderQueue;
  fEmptyReaderQueue = NULL;
  delete fFilledReaderQueue;
  fFilledReaderQueue = NULL;
  delete fEmptyCompDecompQueue;
  fEmptyCompDecompQueue = NULL;
  delete fFilledCompDecompQueue;
  fFilledCompDecompQueue = NULL;
  if (!fMultiVolumeMode) {
    if (fVSS) {
      delete fVSS;
      fVSS = NULL;
    }
    Init();
  }
}

CDriveInfo* COdinManager::GetDriveInfo(int index) {
  return fDriveList->GetItem(index);
}

unsigned COdinManager::GetDriveCount() {
    return (unsigned) fDriveList->GetCount();
  }


void COdinManager::SavePartition(int driveIndex, LPCWSTR fileName, ISplitManagerCallback* cb)
{
	CDriveInfo *pDriveInfo=NULL;
	if (fDriveList && driveIndex >= 0 && driveIndex < (int) fDriveList->GetCount())
	{
		pDriveInfo = fDriveList->GetItem(driveIndex);
    DoCopy(isDrive, pDriveInfo->GetDeviceName().c_str(), isFile, fileName, 0, 0, pDriveInfo->GetClusterSize(),
      cb, false, driveIndex);
	}
}

void COdinManager::RestorePartition(LPCWSTR fileName, int driveIndex, unsigned noFiles, unsigned __int64 totalSize, ISplitManagerCallback* cb)
{
	CDriveInfo *pDriveInfo=NULL;
	if (fDriveList && driveIndex >= 0 && driveIndex < (int) fDriveList->GetCount())
	{
		pDriveInfo = fDriveList->GetItem(driveIndex);
    DoCopy(isFile, fileName, isDrive, pDriveInfo->GetDeviceName().c_str(), noFiles, totalSize, 0, cb, false, driveIndex);
	}
}

void COdinManager::VerifyPartition(LPCWSTR fileName, int driveIndex, unsigned noFiles, unsigned __int64 totalSize, ISplitManagerCallback* cb)
{
  DoCopy(isFile, fileName, isUndefined, NULL, noFiles, totalSize, 0, cb, true, driveIndex);
}

void COdinManager::MakeSnapshot(int driveIndex) {
  if (!fMultiVolumeMode)
    return; // ignore 

  if (fTakeVSSSnapshot) {
    CDriveInfo*	pDriveInfo = driveIndex<0 ? NULL : fDriveList->GetItem(driveIndex);
    int subPartitions = pDriveInfo ? pDriveInfo->GetContainedVolumes() : 0;
    bool isHardDisk = pDriveInfo ? pDriveInfo->IsCompleteHardDisk() : false;
    LPCWSTR *mountPoints;

    if (isHardDisk) {
        CDriveInfo **pContainedVolumes = new CDriveInfo* [subPartitions];
        mountPoints = new LPCWSTR [subPartitions];

        int res = fDriveList->GetVolumes(pDriveInfo, pContainedVolumes, subPartitions);
        for (int i=0; i<res; i++) {
          ATLTRACE(L"Found sub-partition: %s\n", pContainedVolumes[i]->GetDisplayName().c_str());
          mountPoints[i] = pContainedVolumes[i]->GetMountPoint().c_str();
        }
        delete pContainedVolumes;
    } else {
        subPartitions = 1;
        mountPoints = new LPCWSTR [1];
    }

    fVSS = new CVssWrapper();
    fVSS->PrepareSnapshot(mountPoints, subPartitions);
    delete mountPoints;
  }
}

void COdinManager::ReleaseSnapshot(bool bCancelled) {
  if (!fMultiVolumeMode)
    return; // ignore 

  fVSS->ReleaseSnapshot(bCancelled);

}


void COdinManager::GetDriveNameList(std::list<std::wstring>& driveNames)
{
 	for (int i=0; i<(int)fDriveList->GetCount(); i++)
		driveNames.push_back(fDriveList->GetItem(i)->GetDisplayName());
}

void COdinManager::DoCopy(TImageStoreType sourceType, LPCWSTR fileIn, TImageStoreType targetType, LPCWSTR fileOut, 
                          unsigned noFiles, unsigned __int64 totalSize, unsigned bytesPerCluster, 
                          ISplitManagerCallback* cb, bool verifyOnly, int driveIndex)
{
	int nBufferCount = 8;
  TCompressionFormat decompressionFormat = noCompression;
  bool bSaveAllBlocks = fSaveAllBlocks;
  fVerifyCrc32 = 0;
  CDriveInfo*	pDriveInfo = driveIndex<0 ? NULL : fDriveList->GetItem(driveIndex);
  bool isHardDisk = pDriveInfo ? pDriveInfo->IsCompleteHardDisk() : false;
  LPCWSTR mountPoint = NULL;

  if (isHardDisk && !fSaveAllBlocks) {
    ATLASSERT(fMultiVolumeMode);
  } else {
    mountPoint = pDriveInfo ? pDriveInfo->GetMountPoint().c_str() : NULL;
  }

  // Create the output image store
  switch (targetType) {
    case isFile:
      fTargetImage = new CFileImageStream();
      if (fSplitFileSize > 0) {
        fTargetImage->Open(NULL, IImageStream::forWriting);
        fSplitCallback = new CSplitManager(fileOut, fSplitFileSize, (CFileImageStream*)fTargetImage, cb);
        ((CFileImageStream*)fTargetImage)->RegisterCallback(fSplitCallback);      
      } else {
        fTargetImage->Open(fileOut, IImageStream::forWriting);
      }
      break;
    case isDrive: {
      fTargetImage = new CDiskImageStream();
      int subPartitions = pDriveInfo ? pDriveInfo->GetContainedVolumes() : 0;
      ((CDiskImageStream*)fTargetImage)->SetContainedSubPartitionsCount(subPartitions);
      fTargetImage->Open(fileOut, IImageStream::forWriting);
      break;
    } 
    case isNBD:
      break;
    default:
      if (!verifyOnly)
        THROW_INT_EXC(EInternalException::inputTypeNotSet);
  }  // else if NBD

  // Create the input image store
  switch (sourceType) {
    case isFile:
      fSourceImage = new CFileImageStream();
      if (noFiles > 0) {
         fSourceImage->Open(NULL, IImageStream::forReading);
         fSplitCallback = new CSplitManager(fileIn, (CFileImageStream*)fSourceImage, totalSize, cb);
         ((CFileImageStream*)fSourceImage)->RegisterCallback(fSplitCallback);      
      } else {
         fSourceImage->Open(fileIn, IImageStream::forReading);
      }
      break;
    case isDrive: {
      std::wstring vssVolume;
      if (fTakeVSSSnapshot && ! fMultiVolumeMode && !verifyOnly) {
        fVSS = new CVssWrapper();
        LPCWSTR* mountPointPtr = &mountPoint;
        fVSS->PrepareSnapshot(mountPointPtr, 1);
        vssVolume = fVSS->GetSnapshotDeviceName(0);
        if (vssVolume.length())
            fileIn  = vssVolume.c_str() ;
      }
      fSourceImage = new CDiskImageStream();
      fSourceImage->Open(fileIn, IImageStream::forReading);
      break;
    }  // case isDrive:
    case isNBD:
      break;
    default:
      THROW_INT_EXC(EInternalException::outputTypeNotSet);
  }  // switch (SourceType)

  // Determine the block sizes we'll be using
  fEmptyReaderQueue = new CImageBuffer(fReadBlockSize, nBufferCount, L"fEmptyReaderQueue");
  fFilledReaderQueue = new CImageBuffer(L"fFilledReaderQueue");

  CImageBuffer *writerInQueue = NULL;
  CImageBuffer *writerOutQueue = NULL;
  if (sourceType == isFile) {
    CFileImageStream *fileStream = (CFileImageStream*) fSourceImage;
    fileStream->ReadImageFileHeader(true);
    decompressionFormat = fileStream->GetImageFileHeader().GetCompressionFormat();
  }

  if (fCompressionMode != noCompression || decompressionFormat != noCompression) {
    fEmptyCompDecompQueue = new CImageBuffer(fReadBlockSize, nBufferCount, L"fEmptyCompDecompQueue");
    fFilledCompDecompQueue = new CImageBuffer(L"fFilledCompDecompQueue");
    writerInQueue = fFilledCompDecompQueue;
    writerOutQueue = fEmptyCompDecompQueue;
  } else {
    writerInQueue = fFilledReaderQueue;
    writerOutQueue = fEmptyReaderQueue;
  }

  unsigned __int64 volumeBitmapOffset, volumeBitmapLength;
  fReadThread = new CReadThread(fSourceImage, fEmptyReaderQueue, fFilledReaderQueue, verifyOnly);
  fWriteThread = new CWriteThread(fTargetImage, writerInQueue, writerOutQueue, verifyOnly);
  if (sourceType == isFile) {
      CFileImageStream *fileStream = (CFileImageStream*) fSourceImage;
      unsigned __int64 dataOffset;
      LPCWSTR comment = fileStream->GetComment();
      ATLTRACE("Restoring disk image with comment: %S\n", comment ? comment : L"None");
      ATLTRACE("Restoring disk image with CRC32: %x\n", fileStream->GetCrc32Checksum());
      if (!verifyOnly) {
        fileStream->GetImageFileHeader().GetClusterBitmapOffsetAndLength(volumeBitmapOffset, volumeBitmapLength);
        fWriteThread->SetAllocationMapReaderInfo(fSourceImage->GetRunLengthStreamReader(), fileStream->GetImageFileHeader().GetClusterSize());
      }
      dataOffset = fileStream->GetImageFileHeader().GetVolumeDataOffset();
      fReadThread->SetVolumeDataOffset(dataOffset);
      if (decompressionFormat != noCompression) {
        fCompDecompThread = new CDecompressionThread(decompressionFormat, fFilledReaderQueue, 
                              fEmptyReaderQueue, fEmptyCompDecompQueue, writerInQueue);
      } 
      fIsRestoring = true;
  } else if (sourceType == isDrive) {

      CFileImageStream *fileStream = (CFileImageStream*) fTargetImage;
      fileStream->SetComment(fComment.c_str());
      fileStream->SetCompressionFormat(GetCompressionMode());
      fileStream->SetVolumeFormat(isHardDisk ? CImageFileHeader::volumeHardDisk : CImageFileHeader::volumePartition);
      ((CDiskImageStream*)fSourceImage)->SetBytesPerCluster(bytesPerCluster);
      if (bSaveAllBlocks || isHardDisk || !((CDiskImageStream*)fSourceImage)->SupportsReadingOnlyUsedBlocks()) 
        bSaveAllBlocks = true;
      if (!bSaveAllBlocks) {
        fileStream->WriteImageFileHeaderAndAllocationMap((CDiskImageStream*)fSourceImage);
        fReadThread->SetAllocationMapReaderInfo(fSourceImage->GetRunLengthStreamReader(), fileStream->GetImageFileHeader().GetClusterSize());
        if ( fSplitFileSize > 0 && fileStream->GetPosition() > fSplitFileSize)
          THROW_INT_EXC(EInternalException::chunkSizeTooSmall); 
      } else {
        fileStream->WriteImageFileHeaderForSaveAllBlocks(fSourceImage->GetSize(), 
          ((CDiskImageStream*)fSourceImage)->GetBytesPerCluster());
      }
      if (fCompressionMode != noCompression) {
        fCompDecompThread = new CCompressionThread(GetCompressionMode(), fFilledReaderQueue, 
                                  fEmptyReaderQueue, fEmptyCompDecompQueue, writerInQueue);
      }  
      fIsSaving = true;
  }

  if (fReadThread != NULL)
    fReadThread->Resume();
  if (fWriteThread != NULL)
    fWriteThread->Resume();
  if (fCompDecompThread != NULL)
    fCompDecompThread->Resume();
}

void COdinManager::CancelOperation()
{
  if (fReadThread != NULL) {
    fReadThread->CancelThread();
  }
  if (fWriteThread != NULL) {
    fWriteThread->CancelThread();
  }
  if (fCompDecompThread != NULL) {
    fCompDecompThread->CancelThread();
  }
}

unsigned COdinManager::GetThreadCount()
{
  int count;

  if (fReadThread!=NULL && fWriteThread!=NULL)
    count = 2;
  else
    count = 0;
  if (fCompDecompThread!=NULL)
    ++count;
  return count;
}

bool COdinManager::GetThreadHandles(HANDLE* handles, unsigned size)
{
  int i=0;

  if (size<GetThreadCount())
    return false;
  memset(handles, 0, size*sizeof(HANDLE));
  
  if (fReadThread!=NULL)
    handles[i++] = fReadThread->GetHandle();

  if (fWriteThread!=NULL)
    handles[i++] = fWriteThread->GetHandle();

  if (fCompDecompThread!=NULL)
    handles[i++] = fCompDecompThread->GetHandle();
  
  return true;
}

LPCWSTR COdinManager::GetErrorMessage()
{
  LPCWSTR msg = NULL;

  if (fReadThread!=NULL && fReadThread->GetErrorFlag())
    msg = fReadThread->GetErrorMessage();

  if (msg==NULL && fWriteThread!=NULL && fWriteThread->GetErrorFlag())
    msg = fWriteThread->GetErrorMessage();

  if (msg==NULL && fCompDecompThread!=NULL && fCompDecompThread->GetErrorFlag())
    msg = fCompDecompThread->GetErrorMessage();
  
  return msg;
}

bool COdinManager::WasError()
{
  return (fReadThread!=NULL && fReadThread->GetErrorFlag()) ||
         (fWriteThread!=NULL && fWriteThread->GetErrorFlag()) ||
         (fCompDecompThread!=NULL && fCompDecompThread->GetErrorFlag());
}

unsigned __int64 COdinManager::GetTotalBytesToProcess()
{
  // TODO: 
  // The total number of bytes to process is used as reference for the progress bar.
  // For backup use:
  // allocated bytes of source image if used block are saved
  // size of source partition if all blocks are saved
  // For restore use:
  // used blocks of image file header if image file contains only used blocks
  // all bytes of destination partition if image file contains all blocks
  // Note that image file can be compressed heavily so that sizes of image file may lead to inaccurate results
  
  unsigned __int64 res;

  if (fIsSaving || (fIsRestoring && fTargetImage == NULL) ) {
  //               ^ verify mode! (rhs of or condition)                   
    if (NULL != fSourceImage) {
      if (fSaveAllBlocks)
        res = fSourceImage->GetSize();
      else {
        res = fSourceImage->GetAllocatedBytes();
        if (res == 0) // happens for raw disks
          res = fSourceImage->GetSize();
      }
    } else {
      res = 0;
    }
  } else if (fIsRestoring) {
    if (NULL != fTargetImage) {
      if (fSaveAllBlocks)
        res = fTargetImage->GetSize();
      else {
        res = fSourceImage->GetAllocatedBytes();
        if (res == 0) // happens for raw disks
          res = fSourceImage->GetSize();
      }
    } else {
      // we are in verify mode
      res = 0;
    }
  } else {
    res =0;
  }
/*
  if (NULL != fSourceImage) {
    if (fSaveAllBlocks)
      res = fSourceImage->GetSize();
    else {
      res = fSourceImage->GetAllocatedBytes();
      if (res == 0) // happens for raw disks
        res = fSourceImage->GetSize();
    }
  }
  else
    res = 0;
 */
  return res;
}

unsigned __int64 COdinManager::GetBytesProcessed()
{
  // TODO: This value is used to calculate progress bar. Use always the number of processed bytes
  // from the write thread, because if image is restored and compressed heavily the number of the 
  // image file may be very inaccurate.

  if (fIsRestoring && NULL != fWriteThread)
    return fWriteThread->GetBytesProcessed();
  else if (fIsSaving && NULL != fReadThread)
    return fReadThread->GetBytesProcessed();
  else
    return 0;
}

bool COdinManager::IsFileReadable(LPCWSTR fileName)
{
  HANDLE h = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
  if (h != INVALID_HANDLE_VALUE)
  {
	  CloseHandle(h);
  }
 
  return h != INVALID_HANDLE_VALUE;
}
