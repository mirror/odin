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

#pragma once

#include <string>
#include "UserFeedback.h"

class IUserFeedback;
class CDriveList;
class COdinManager;
class ISplitManagerCallback;

class CParamChecker {
public:
  CParamChecker(IUserFeedback& feedback, COdinManager& odinManager);
  bool CheckUniqueFileName(LPCWSTR path, LPCWSTR filePattern, bool useConfirmMessage);
  IUserFeedback::TFeedbackResult CheckConditionsForSavePartition(const std::wstring& fileName, int index);
  IUserFeedback::TFeedbackResult CheckConditionsForVerifyMBRFile(const std::wstring& fileName);
  IUserFeedback::TFeedbackResult CheckConditionsForVerifyPartition(const std::wstring& fileName, ISplitManagerCallback& splitCB,
    int& volType, unsigned __int64* partitionSizeToSave, DWORD& crc32FromFileHeader);
  IUserFeedback::TFeedbackResult CheckConditionsForRestorePartition(const std::wstring& fileName, ISplitManagerCallback& splitCB,
     int index, unsigned& noFiles, unsigned __int64& totalSize);
  bool CheckForExistingConflictingFilesSimple(LPCWSTR fileName, ISplitManagerCallback& splitCB);
  bool CheckForExistingConflictingFilesEntireDisk(LPCWSTR volumeFileName, LPCWSTR fileName);

private:

  CDriveInfo* GetDriveInfo(int index);
  unsigned GetDriveCount();


  IUserFeedback& fFeedback;
  COdinManager& fOdinManager;
};
