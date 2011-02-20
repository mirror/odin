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
/////////////////////////////////////////////////////////////////////////////
//
// This class is a small convenience class on top of ODIN Manager that 
// handles disk images split into multiple volume files. It has a naming
// convention how partion names are mapped to file names containing the
// image files for each individual partition (.img)
//
///////////////////////////////////////////////////////////////////////////// 

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

class ISplitManagerCallback;
class IWaitCallback;
class COdinManager;
class IUserFeedback;

class CMultiPartitionHandler {
public:

  static void BackupPartitionOrDisk(int index, LPCWSTR fileName, COdinManager &odinMgr, ISplitManagerCallback* cb, 
    IWaitCallback* wcb, IUserFeedback& feedback);

  static void RestorePartitionOrDisk(int index, LPCWSTR fileName, COdinManager &mgr, ISplitManagerCallback* cb, IWaitCallback* wcb);
  static bool VerifyPartitionOrDisk( LPCWSTR fileName, COdinManager &odinMgr, DWORD& crc32FromFileHeader, ISplitManagerCallback* cb, IWaitCallback* wcb, IUserFeedback& feedback);

private:
  static void WaitForDriveReady(COdinManager &odinMgr, int index, unsigned partitionCount, const std::wstring& targetDiskDeviceName);
  static void GetNoFilesAndFileSize(LPCWSTR fileName, ISplitManagerCallback* cb, unsigned& fileCount, 
                                         unsigned __int64& fileSize, bool& isEntireDriveImageFile);

};