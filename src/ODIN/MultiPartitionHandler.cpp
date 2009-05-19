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
// This class is a small convenience class on top of ODIN Manager that 
// handles disk images split into multiple volume files. It has a naming
// convention how partion names are mapped to file names containing the
// image files for each individual partition (.img)
//
///////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include <string>
#include "MultiPartitionHandler.h"
#include "OdinManager.h"
#include "SplitManagerCallback.h"
#include "DriveList.h"
#include "FileNameUtil.h"
#include "ImageStream.h"
#include "PartitionInfoMgr.h"
#include "ParamChecker.h"
#include "UserFeedback.h"

using namespace std;

// This method handles also multiple partitons in multiple files
void CMultiPartitionHandler::BackupPartitionOrDisk(int index, LPCWSTR fileName, COdinManager &odinMgr, 
                                                   ISplitManagerCallback* cb, IWaitCallback* wcb, IUserFeedback& feedback)
{
  BOOL ok = TRUE;
  CDriveInfo* pDriveInfo = odinMgr.GetDriveList()->GetItem(index);
  bool isHardDisk = pDriveInfo->IsCompleteHardDisk();
  CDriveInfo **pContainedVolumes = NULL;
  wstring volumeFileName;
  CParamChecker checker(feedback, odinMgr);

  if (isHardDisk && odinMgr.GetSaveOnlyUsedBlocksOption()) {
    int subPartitions = pDriveInfo->GetContainedVolumes();
    pContainedVolumes = new CDriveInfo* [subPartitions];
    int res = odinMgr.GetDriveList()->GetVolumes(pDriveInfo, pContainedVolumes, subPartitions);

    // check  if there are file names in conflict with files created during backup
    for (int i=0; i<subPartitions; i++) {
      CFileNameUtil::GenerateFileNameForEntireDiskBackup(volumeFileName, fileName, pContainedVolumes[i]->GetDeviceName());
      ok = checker.CheckForExistingConflictingFilesEntireDisk(volumeFileName.c_str(), fileName);
      if (!ok)
        break;
    }

    if (ok) {
      wstring mbrFileName(fileName);
      CFileNameUtil::GenerateFileNameForMBRBackupFile(mbrFileName);
      CPartitionInfoMgr partInfoMgr;
      partInfoMgr.ReadPartitionInfoFromDisk(pDriveInfo->GetDeviceName().c_str());
      partInfoMgr.WritePartitionInfoToFile(mbrFileName.c_str());

      odinMgr.Uncancel();
      if (odinMgr.GetTakeSnapshotOption())  {
        odinMgr.SetMultiVolumeMode(true);
        odinMgr.MakeSnapshot(index, wcb);
      }

      for (int i=0; i<subPartitions && !odinMgr.WasCancelled(); i++) {
        CFileNameUtil::GenerateFileNameForEntireDiskBackup(volumeFileName, fileName, pContainedVolumes[i]->GetDeviceName());
        wcb->OnPartitionChange(i, subPartitions);
        odinMgr.SetMultiVolumeIndex(i);
        odinMgr.SavePartition(odinMgr.GetDriveList()->GetIndexOfDeviceName(pContainedVolumes[i]->GetDeviceName()),
          volumeFileName.c_str(), odinMgr.GetSplitSize() ? cb : NULL, wcb);
        ATLTRACE(L"Found sub-partition: %s\n", pContainedVolumes[i]->GetDisplayName().c_str());
        odinMgr.WaitToCompleteOperation(wcb);
      }

      delete pContainedVolumes;
      odinMgr.SetMultiVolumeIndex(0);
      if (odinMgr.GetTakeSnapshotOption()) {
        odinMgr.ReleaseSnapshot(false);
        odinMgr.SetMultiVolumeMode(false);
      }
      odinMgr.Uncancel();
    }
  } else { // not a complete hard disk, but only a paritition or complete disk but with all blocks
    ok = checker.CheckForExistingConflictingFilesSimple(fileName, *cb);
    if (ok) {
      // save drive to file
      odinMgr.SetMultiVolumeMode(false);
      if (!odinMgr.GetTakeSnapshotOption())
        wcb->OnPartitionChange(0, 1);
      odinMgr.SavePartition(index, fileName, cb, wcb);
      odinMgr.WaitToCompleteOperation(wcb);
    }
  }
}


void CMultiPartitionHandler::RestorePartitionOrDisk(int index, LPCWSTR fileName, COdinManager &odinMgr, 
                                                    ISplitManagerCallback* cb, IWaitCallback* wcb)
{
  CDriveInfo* pDriveInfo = odinMgr.GetDriveList()->GetItem(index); 
  bool isHardDisk;
  unsigned fileCount = 0;  
  unsigned __int64 fileSize = 0;
  wstring volumeFileName;
  wstring baseName = fileName;
  wstring mbrName = fileName;  
  bool isEntireDriveImagefile;

  // check if this is an .mbr file
  isHardDisk = CFileNameUtil::TestIsHardDiskImage(fileName);
  if (isHardDisk) {
    isEntireDriveImagefile = false;
  } else {
    // or a .img file:
    CFileNameUtil::RemoveTrailingNumberFromFileName(baseName);
    GetNoFilesAndFileSize(baseName.c_str(), cb, fileCount, fileSize, isEntireDriveImagefile);
    // treat special case where we have split file mode and only a single file with appendix 0000
    if (fileCount == 0 && baseName != fileName)
      baseName = fileName;
  }

  if (isHardDisk && !isEntireDriveImagefile) {
    wstring mbrFileName(fileName);
    wstring targetDiskDeviceName (pDriveInfo->GetDeviceName());
    CFileNameUtil::GenerateFileNameForMBRBackupFile(mbrFileName);
    CPartitionInfoMgr partInfoMgr;
    partInfoMgr.ReadPartitionInfoFromFile(mbrFileName.c_str());
    partInfoMgr.MakeWritable();
    partInfoMgr.WritePartitionInfoToDisk(pDriveInfo->GetDeviceName().c_str());
    odinMgr.RefreshDriveList();
    // now get drive info again, because the index might have changed:
    index = odinMgr.GetDriveList()->GetIndexOfDeviceName(targetDiskDeviceName);
    pDriveInfo = odinMgr.GetDriveList()->GetItem(index);

    odinMgr.Uncancel();
    unsigned subPartitions = partInfoMgr.GetPartitionCount();
    unsigned diskNo = pDriveInfo->GetDiskNumber();
    wstring volumeDeviceName, partitionFileName;
    for (unsigned i=0; i<subPartitions && !odinMgr.WasCancelled(); i++) {
      CFileNameUtil::GenerateDeviceNameForVolume(volumeDeviceName, diskNo, i+1);
      CFileNameUtil::GenerateFileNameForEntireDiskBackup(volumeFileName, fileName, volumeDeviceName);
      ATLTRACE(L"Found sub-partition: %s\n", volumeDeviceName.c_str());
      int volumeIndex = odinMgr.GetDriveList()->GetIndexOfDeviceName(volumeDeviceName);
      CFileNameUtil::RemoveTrailingNumberFromFileName(volumeFileName);
      GetNoFilesAndFileSize(volumeFileName.c_str(), cb, fileCount, fileSize, isEntireDriveImagefile);
      wcb->OnPartitionChange(i, subPartitions);
      odinMgr.RestorePartition(volumeFileName.c_str(), volumeIndex, fileCount, fileSize, fileCount>0 ? cb : NULL, wcb);
      odinMgr.WaitToCompleteOperation(wcb);
    }
  } else {
		  // restore file to drive
      wcb->OnPartitionChange(0, 1);
      odinMgr.RestorePartition(baseName.c_str(), index, fileCount, fileSize, cb, wcb);
      odinMgr.WaitToCompleteOperation(wcb);
  }
  odinMgr.Uncancel();

}

bool CMultiPartitionHandler::VerifyPartitionOrDisk(LPCWSTR fileName, COdinManager &odinMgr, 
                                                   DWORD& crc32FromFileHeader, ISplitManagerCallback* cb, 
                                                   IWaitCallback* wcb, IUserFeedback& feedback)
{
    unsigned noFiles = 0;
    unsigned __int64 totalSize = 0;
    int volType;
    wstring volumeFileName;
    unsigned subPartitions;
    CParamChecker checker(feedback, odinMgr);
    IUserFeedback::TFeedbackResult res;
    bool isEntireDriveImage;

    bool isHardDisk = CFileNameUtil::TestIsHardDiskImage(fileName);

    // verify conditions for MBR file containing boot loader and partition table information
    if (isHardDisk) {
      wstring mbrFileName(fileName);
      res = checker.CheckConditionsForVerifyMBRFile(fileName);
      if (res != IUserFeedback::TOk && res != IUserFeedback::TYes)
        return false;
      CFileNameUtil::GenerateFileNameForMBRBackupFile(mbrFileName);
      CPartitionInfoMgr partInfoMgr;
      partInfoMgr.ReadPartitionInfoFromFile(mbrFileName.c_str());
      subPartitions = partInfoMgr.GetPartitionCount();
    } else {
      subPartitions = 1;
    }

    odinMgr.Uncancel();
    for (unsigned i=0; i<subPartitions && !odinMgr.WasCancelled(); i++) {
      if (isHardDisk) {
        wstring volumeDeviceName;
        CFileNameUtil::GenerateDeviceNameForVolume(volumeDeviceName, 99 /* dummy value */, i+1);
        CFileNameUtil::GenerateFileNameForEntireDiskBackup(volumeFileName, fileName, volumeDeviceName);
      }
      else
        volumeFileName = fileName;
      res = checker.CheckConditionsForVerifyPartition(volumeFileName, *cb, volType, NULL, crc32FromFileHeader);
      if (res == IUserFeedback::TOk) {
        CFileNameUtil::RemoveTrailingNumberFromFileName(volumeFileName);
        GetNoFilesAndFileSize(volumeFileName.c_str(), cb, noFiles, totalSize, isEntireDriveImage);
        odinMgr.VerifyPartition(volumeFileName.c_str(), -1, noFiles, totalSize, cb, wcb);
        odinMgr.WaitToCompleteOperation(wcb);
      }
    }
    odinMgr.Uncancel();
    return true;
}

void CMultiPartitionHandler::GetNoFilesAndFileSize(LPCWSTR fileName, ISplitManagerCallback* cb, unsigned& fileCount, 
                                         unsigned __int64& fileSize, bool& isEntireDriveImageFile)
{
  wstring splitFileName(fileName);
  fileCount = 0;
  fileSize = 0;

  cb->GetFileName(0, splitFileName);

  if (!CFileNameUtil::IsFileReadable(fileName) && CFileNameUtil::IsFileReadable(splitFileName.c_str())) {
    CFileImageStream fileStream;
    fileStream.Open(splitFileName.c_str(), IImageStream::forReading);
    fileStream.ReadImageFileHeader(false);
    fileCount = fileStream.GetImageFileHeader().GetFileCount();  
    fileSize = fileStream.GetImageFileHeader().GetFileSize();
    isEntireDriveImageFile = fileStream.GetImageFileHeader().GetVolumeType() == CImageFileHeader::volumeHardDisk;
    fileStream.Close();
  }
}

