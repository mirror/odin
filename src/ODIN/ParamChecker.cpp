/******************************************************************************

    ODIN - Open Disk Imager in a Nutshell

    Copyright (C) 2009

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
// Class that offers methods for checking user provided parameters for
// for consistency, asks the user for confirmation on various conditions
// before triggerin a backup, verify or restore operation
//
///////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include <sstream>
#include <string>
#include <list>
#include <atlmisc.h>  // CString
#include "UserFeedback.h"
#include "DriveList.h"
#include "ParamChecker.h"
#include "UserFeedback.h"
#include "OdinManager.h"
#include "FileNameUtil.h"
#include "Util.h"
#include "ImageStream.h"
#include "PartitionInfoMgr.h"
#include "SplitManagerCallback.h"
#include "resource.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

using namespace std;

CParamChecker::CParamChecker(IUserFeedback& feedback, COdinManager& odinManager) 
  : fFeedback(feedback), fOdinManager(odinManager) {
    
}

bool CParamChecker::CheckUniqueFileName(LPCWSTR path, LPCWSTR filePattern, bool useConfirmMessage)
{
  wstring dir(path);
  WIN32_FIND_DATA res;
  HANDLE h;
  IUserFeedback::TFeedbackResult msgRes;
  list<wstring> files;
  list <wstring>::iterator itBegin, itEnd;
  bool success = true;
  size_t off;
  int count = 0;

  off = dir.rfind(L'\\');
  if (off != string::npos)
    dir = dir.substr(0, off+1);
  else
    dir.clear();

  h = FindFirstFile(filePattern, &res);
  if (h == INVALID_HANDLE_VALUE) 
    return true;
  else {
    files.push_back(res.cFileName);
    ++count;
    while ( FindNextFile(h, &res) ) {
      files.push_back(res.cFileName);
		  ++count;
    }
    FindClose(h);

    if (count > 0) {
      if (useConfirmMessage) {
        wostringstream msg;
        WTL::CString prefix, postfix;
                   
        prefix.Format(IDS_ASK_DELETE_FILES, dir.c_str(), count);
        postfix.LoadString(IDS_ASK_CONTINUE);

        msg << (LPCWSTR)prefix << endl;
        int displayCount = min(10, count);
        itBegin = files.begin();
        for (int i=0; i<displayCount; i++)
        {
          msg << *itBegin++ << endl;
        }
        if (count > displayCount)
          msg << _T("...") << endl;
        msg << (LPCWSTR)postfix << endl;
        msgRes = fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TOkCancel, msg.str().c_str());
      } else {
        msgRes = IUserFeedback::TOk;
      }

      if (msgRes == IUserFeedback::TOk) {
        // delete files in list
        for (itBegin = files.begin(), itEnd = files.end(); itBegin != itEnd && success; itBegin++)
        {
          wstring file = dir + (*itBegin).c_str();
          success = DeleteFile(file.c_str()) != FALSE;
          if (!success) {
            WTL::CString msg;
            msg.Format(IDS_CANNOTDELETEFILE, file.c_str());
            msgRes = fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TOkCancel, (LPCWSTR)msg);
			break;
		  }
        }
      } else
        success = false;
      }
  }    
  return success;
}

IUserFeedback::TFeedbackResult CParamChecker::CheckConditionsForSavePartition(const wstring& fileName, int index)
{
  wstring targetDrive, dir;
  IUserFeedback::TFeedbackResult res = IUserFeedback::TOk;
  unsigned __int64 freeBytesAvailable, totalNumberOfBytes;
  const size_t cBufferSize = 64;
  wchar_t buffer[cBufferSize];
  WTL::CString msgStr;

  if (index < 0) {
    msgStr.LoadString(IDS_VOLUME_NOSEL);
    fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TConfirm, msgStr);
    return IUserFeedback::TCancel;
  }

  // check if target file already exists and warn that it will be overwritten
  if (CFileNameUtil::IsFileReadable(fileName.c_str())) {
    msgStr.Format(IDS_FILE_EXISTS, fileName.c_str());
    res = fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TOkCancel, (LPCWSTR)msgStr);
    if (res != IUserFeedback::TOk)
      return res;
  }

  // check if target file can be written to
  if (!CFileNameUtil::IsFileWritable(fileName.c_str())) {
    msgStr.Format(IDS_CANTWRITEFILE, fileName.c_str());
    fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
    return IUserFeedback::TCancel;
  }

  // check available disk space
  CFileNameUtil::GetDirFromFileName(fileName, dir);
  ATLTRACE("Getting dir from file : %S\n", dir.c_str());
  CFileNameUtil::GetFreeBytesOfDisk(dir.c_str(), &freeBytesAvailable, &totalNumberOfBytes);
  MakeByteLabel(freeBytesAvailable, buffer, cBufferSize);
  ATLTRACE("Number of bytes available on disk (for this user): %S\n", buffer);
  MakeByteLabel(totalNumberOfBytes, buffer, cBufferSize);
  ATLTRACE("Total number of bytes on disk : %S\n", buffer);

  unsigned __int64 bytesToSave = GetDriveInfo(index)->GetUsedSize();
  if (bytesToSave == 0)
    bytesToSave = GetDriveInfo(index)->GetBytes();


  if (freeBytesAvailable < bytesToSave) {
    msgStr.LoadStringW(IDS_NOTENOUGHSPACE);
    res = fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TYesNo, (LPCWSTR)msgStr);

    if (res != IUserFeedback::TYes)
      return res;
  } else
    res = IUserFeedback::TOk;

  // check if file size of image possibly exceeds 4GB limit on FAT32:
  CFileNameUtil::GetDriveFromFileName(fileName, targetDrive);
  int targetIndex = fOdinManager.GetDriveList()->GetIndexOfDrive(targetDrive.c_str());
  bool isFAT = targetIndex >= 0 && 
              (GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_FAT32 ||
               GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_HUGE ||
               GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_FAT32_XINT13 ||
               GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_FAT_12 ||
               GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_FAT32_XINT13 ||
               GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_FAT_16);
  if (isFAT && bytesToSave > (2i64<<32)-1 && fOdinManager.GetSplitSize() == 0) {
    msgStr.LoadStringW(IDS_4GBLIMITEXCEEDED);
    res = fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TYesNo, (LPCWSTR)msgStr);
    if (res != IUserFeedback::TYes)
      return res;
    else
      res = IUserFeedback::TOk;
  }

  CDriveInfo* pDriveInfo = fOdinManager.GetDriveList()->GetItem(index);
  bool isHardDisk = pDriveInfo->IsCompleteHardDisk();
  CDriveInfo **pContainedVolumes = NULL;
  int partitionsToSave;
  wstring volumeFileName;
  bool displayedMountedWarning = false;
  
  if (isHardDisk) {
    partitionsToSave = pDriveInfo->GetContainedVolumes();
    pContainedVolumes = new CDriveInfo* [partitionsToSave];
    int no = fOdinManager.GetDriveList()->GetVolumes(pDriveInfo, pContainedVolumes, partitionsToSave);
  } else {
    partitionsToSave = 1;
    pContainedVolumes = new CDriveInfo* [1];
    pContainedVolumes[0] = GetDriveInfo(index);
  }
  
  for (int i=0; i<partitionsToSave; i++) {
    wstring sourceDrive = pContainedVolumes[i]->GetMountPoint();
    if (!pContainedVolumes[i]->IsMounted() && !displayedMountedWarning) {
      msgStr.LoadStringW(IDS_UNMOUNTED_VOLUME);
      res = fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TOkCancel, (LPCWSTR)msgStr);
      displayedMountedWarning = true; // display only once
      if (res != IUserFeedback::TOk) {
        delete pContainedVolumes;
        return res;
      }
    } else if (sourceDrive.compare(targetDrive) == 0) {
      // warning: do not use same drive for source and destination
      msgStr.LoadStringW(IDS_NOTSAMEDRIVE);
      res = fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TOkCancel, (LPCWSTR)msgStr);
      if (res != IUserFeedback::TOk) {
        delete pContainedVolumes;
        return res;
      }
    }

    // warning: do not backup windows partition
    wstring sysDir, sysDrive;
    wchar_t systemDir[MAX_PATH];
    int count = GetSystemDirectory(systemDir, MAX_PATH);
    sysDir = systemDir;
    CFileNameUtil::GetDriveFromFileName(sysDir, sysDrive);
    if (sourceDrive.compare(sysDrive) == 0 && !fOdinManager.GetTakeSnapshotOption()) {
      msgStr.LoadStringW(IDS_NOWINDIRBACKUP);
      res = fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TOkCancel, (LPCWSTR)msgStr);
    }
  }

  delete pContainedVolumes;

  // internal implementation limitation: file size of split size must be bigger than fReadBlockSize
  if (fOdinManager.GetSplitSize() > 0 && fOdinManager.GetSplitSize() < fOdinManager.GetReadBlockSize()) {
    const int BUFSIZE=80;
    wchar_t buffer[BUFSIZE];
    WTL::CString msgStr;
    MakeByteLabel( fOdinManager.GetReadBlockSize(), buffer, BUFSIZE);
    msgStr.Format(IDS_SPLITSIZETOOSMALL, buffer);
    res = fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
    res = IUserFeedback::TCancel;
  }

  return res;
}

IUserFeedback::TFeedbackResult CParamChecker::CheckConditionsForVerifyMBRFile(const std::wstring& fileName)
{
  CPartitionInfoMgr partInfoMgr;
  wstring mbrFileName(fileName);

  CFileNameUtil::GenerateFileNameForMBRBackupFile(mbrFileName);
  if (!CFileNameUtil::IsFileReadable(mbrFileName.c_str()))
    return IUserFeedback::TCancel;

  if (!partInfoMgr.IsValidFileHeader(mbrFileName.c_str()))
    return IUserFeedback::TCancel;

  return IUserFeedback::TOk;
}

IUserFeedback::TFeedbackResult CParamChecker::CheckConditionsForVerifyPartition(const std::wstring& fileName, ISplitManagerCallback& splitCB,
    int& volType, unsigned __int64* partitionSizeToSave, DWORD& crc32FromFileHeader)
{
  CFileImageStream fileStream;
  IUserFeedback::TFeedbackResult res = IUserFeedback::TOk;
  WTL::CString msgStr;
  wstring openName = fileName;
  wstring splitFileName = fileName;
  splitCB.GetFileName(0, splitFileName);
  unsigned noFiles = 0;
  unsigned __int64 totalSize = 0;

  if (!CFileNameUtil::IsFileReadable(openName.c_str()))
    openName = splitFileName; // try multiple files mode

  if (!CFileNameUtil::IsFileReadable(openName.c_str()))
  {
    msgStr.Format(IDS_CANTREADFILE, fileName.c_str());
    fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
    return IUserFeedback::TCancel;
  }

  fileStream.Open(openName.c_str(), IImageStream::forReading);
  fileStream.ReadImageFileHeader(false);
  if (!fileStream.GetImageFileHeader().IsValidFileHeader())
  {
    msgStr.LoadString(IDS_WRONGFILEFORMAT);
    fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
    res = IUserFeedback::TCancel;
  }
  if (partitionSizeToSave)
    *partitionSizeToSave = fileStream.GetImageFileHeader().GetVolumeSize();
  noFiles = fileStream.GetImageFileHeader().GetFileCount();
  totalSize = fileStream.GetImageFileHeader().GetFileSize();
  crc32FromFileHeader = fileStream.GetCrc32Checksum();
  volType = fileStream.GetImageFileHeader().GetVolumeType();

  if (totalSize == 0 || crc32FromFileHeader == 0) {
    // The image is corrupt and was not written completely (this information is stored as last step)
    msgStr.Format(IDS_INCOMPLETE_IMAGE, openName.c_str());
    fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
    res = IUserFeedback::TCancel;
  }
  return res;
}

IUserFeedback::TFeedbackResult CParamChecker::CheckConditionsForRestorePartition(const wstring& fileName, ISplitManagerCallback& splitCB,
     int index, unsigned& noFiles, unsigned __int64& totalSize)
{
  unsigned __int64 targetPartitionSize, partitionSizeToSave;
  CFileImageStream fileStream;
  IUserFeedback::TFeedbackResult res = IUserFeedback::TOk;
  int volType;
  WTL::CString msgStr;
  wstring openName = fileName;
  
  noFiles = 0;
  totalSize = 0;
  if (index < 0) {
    msgStr.LoadStringW(IDS_VOLUME_NOSEL);
    fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
    return IUserFeedback::TCancel;
  }

  CDriveInfo* pDriveInfo = fOdinManager.GetDriveList()->GetItem(index);
  bool isMBRFile = CFileNameUtil::TestIsHardDiskImage(fileName.c_str());

  if (isMBRFile) {
    // verify conditions for MBR file containing boot loader and partition table information
    res = CheckConditionsForVerifyMBRFile(fileName);
    if (res != IUserFeedback::TOk && res != IUserFeedback::TYes)
      return res;
    // check that target is a harddisk too 
    if (!pDriveInfo->IsCompleteHardDisk()) {
      msgStr.LoadStringW(IDS_WRONGPARTITIONTYPE);
      fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
      return IUserFeedback::TCancel;
    }

    // check that source and target size match
    CPartitionInfoMgr partInfoMgr;
    wstring mbrFileName(fileName);
    CFileNameUtil::GenerateFileNameForMBRBackupFile(mbrFileName);
    partInfoMgr.ReadPartitionInfoFromFile(mbrFileName.c_str());

    if (partInfoMgr.GetDiskSize() > (unsigned __int64) pDriveInfo->GetBytes()) {
      const int bufSize=25;
      wchar_t buffer1[bufSize], buffer2[bufSize];
      FormatNumberWithDots(partInfoMgr.GetDiskSize(), buffer1, bufSize);        
      FormatNumberWithDots(pDriveInfo->GetBytes(), buffer2, bufSize);
      msgStr.FormatMessage(IDS_IMAGETOOBIG, buffer1, buffer2);
      fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
      res = IUserFeedback::TCancel;
    } else if (partInfoMgr.GetDiskSize() < (unsigned __int64) pDriveInfo->GetBytes()) {
      msgStr.LoadStringW(IDS_IMAGETOOSMALL);
      res = fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TYesNo, (LPCWSTR)msgStr);
    }
    if (res != IUserFeedback::TOk && res != IUserFeedback::TYes)
      return res;

    volType = CImageFileHeader::volumeHardDisk;
  } else {
    DWORD dummy;
    res = CheckConditionsForVerifyPartition(fileName, splitCB, volType, &partitionSizeToSave, dummy);
    if (res != IUserFeedback::TYes && res != IUserFeedback::TOk)
      return res;
  }

  // prevent restoring a hard disk to a partition or a partition to a hard disk
  if (( pDriveInfo->IsCompleteHardDisk() && volType != CImageFileHeader::volumeHardDisk) ||
      (!pDriveInfo->IsCompleteHardDisk() && volType != CImageFileHeader::volumePartition)) {
    msgStr.LoadStringW(IDS_WRONGPARTITIONTYPE);
    fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
    res = IUserFeedback::TCancel;
    return res;
  }

  if (!isMBRFile) {  
      targetPartitionSize =  pDriveInfo->GetBytes();
      if (targetPartitionSize < partitionSizeToSave) {
        const int bufSize=25;
        wchar_t buffer1[bufSize], buffer2[bufSize];
        FormatNumberWithDots(targetPartitionSize, buffer1, bufSize);        
        FormatNumberWithDots(partitionSizeToSave, buffer2, bufSize);
        msgStr.FormatMessage(IDS_IMAGETOOBIG, buffer1, buffer2);
        fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
        res = IUserFeedback::TCancel;
      }
      else if (targetPartitionSize > partitionSizeToSave) {
        msgStr.LoadStringW(IDS_IMAGETOOSMALL);
        res = fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TYesNo, (LPCWSTR)msgStr);
      }
      else
        res = IUserFeedback::TOk; 

      // warning: do not restore to windows partition
      wstring sysDir, sysDrive;
      wchar_t systemDir[MAX_PATH];
      int count = GetSystemDirectory(systemDir, MAX_PATH);
      sysDir = systemDir;
      CFileNameUtil::GetDriveFromFileName(sysDir, sysDrive);
      wstring targetDrive =  pDriveInfo->GetMountPoint();
      if (sysDrive.compare(targetDrive) == 0 && !fOdinManager.GetTakeSnapshotOption()) {
        msgStr.LoadStringW(IDS_NORESTORETOWINDOWS);
        fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msgStr);
        res = IUserFeedback::TCancel;
      }
  }

  if (res == IUserFeedback::TOk) {
    WTL::CString msg;
    msg.Format(IDS_ERASE_DRIVE, fOdinManager.GetDriveInfo(index)->GetMountPoint().c_str());
    res = fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TOkCancel, (LPCWSTR)msg);
    if (res != IUserFeedback::TOk)
      return res;
  }
  
  return res;
}

bool CParamChecker::CheckForExistingConflictingFilesSimple(LPCWSTR fileName, ISplitManagerCallback& splitCB)
{
  bool ok = true;
  if (fOdinManager.GetSplitSize() > 0) {
    // check  if there are file names in conflict with files created during backup
    wstring filePattern;
    CFileNameUtil::GetSplitFilePattern(splitCB, fileName, filePattern);
    ok = CheckUniqueFileName(fileName, filePattern.c_str(), true);
  } else if (CFileNameUtil::IsFileReadable(fileName)) {
    ok = DeleteFile(fileName) == TRUE;
    if (!ok) {
      WTL::CString msg;
      msg.Format(IDS_CANNOTDELETEFILE, fileName);
      fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msg);
    }
  }
  return ok;
}

bool CParamChecker::CheckForExistingConflictingFilesEntireDisk(LPCWSTR volumeFileName, LPCWSTR fileName)
{
  bool ok = true;
  wstring mbrFileName(fileName);
  CFileNameUtil::GenerateFileNameForMBRBackupFile(mbrFileName);

  if (CFileNameUtil::IsFileReadable(mbrFileName.c_str())) {
    WTL::CString msgStr;
    msgStr.Format(IDS_FILE_EXISTS, mbrFileName.c_str());
    IUserFeedback::TFeedbackResult res = fFeedback.UserMessage(IUserFeedback::TWarning, IUserFeedback::TOkCancel, (LPCWSTR)msgStr);

    if (res != IUserFeedback::TOk)
      return false;
    ok = DeleteFile(mbrFileName.c_str()) == TRUE;
    if (!ok) {
      WTL::CString msg;
      msg.Format(IDS_CANNOTDELETEFILE, mbrFileName.c_str());
      fFeedback.UserMessage(IUserFeedback::TError, IUserFeedback::TConfirm, (LPCWSTR)msg);
    }
  }

  if (ok) {
    // check  if there are file names in conflict with files created during backup
    wstring filePattern;
    CFileNameUtil::GetEntireDiskFilePattern(volumeFileName, filePattern);
    ok = CheckUniqueFileName(volumeFileName, filePattern.c_str(), true);
  }
  return ok;
}

// Helper  functions:
CDriveInfo* CParamChecker::GetDriveInfo(int index) {
  return fOdinManager.GetDriveList()->GetItem(index);
}

unsigned CParamChecker::GetDriveCount() {
    return (unsigned) fOdinManager.GetDriveList()->GetCount();
}

void CParamChecker::FormatNumberWithDots(unsigned __int64 value, LPWSTR buffer, size_t bufsize) {
  const int bufSize = 25;
  wchar_t buffer2[bufSize];
  _ui64tow_s(value, buffer2, bufSize, 10);
  size_t len2 = wcslen(buffer2);
  size_t j=len2 + ((len2-1) / 3);
  buffer[j--] = L'\0';
  for (int i=len2-1; i>=0; i--) {
    buffer[j--] = buffer2[i];
    if (i > 0 &&(len2-i) %3 == 0)
      buffer[j--] = L'.';
  }  
}