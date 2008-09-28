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
#ifndef __DRIVELIST_H__
#define __DRIVELIST_H__

//---------------------------------------------------------------------------
extern bool HaveNTCalls;
//---------------------------------------------------------------------------
enum TDeviceType { driveFixed, driveRemovable, driveFloppy, driveUnknown };

//---------------------------------------------------------------------------
class CDriveInfo {
  public:
   CDriveInfo(std::wstring displayName, std::wstring deviceName, std::wstring mountPoint, bool isDisk, int containedVolumes = 0);

   std::wstring  GetDisplayName () {
		 return fDisplayName;
	 }
     
	 std::wstring  GetDeviceName() {
		 return fDeviceName;
	 }

   std::wstring GetMountPoint() {
		 return fMountPoint;
	 }

   TDeviceType GetDriveType() {
		 return fDriveType;
	 }

   __int64 GetBytes() {
		return fBytes;
	 }
     
	 __int64 GetSectors() { 
		 return fSectors;
	 }

   int GetBytesPerSector() {
		 return fBytesPerSector;
	 }

   int  GetSectorsPerTrack(){
		 return fSectorsPerTrack;
	 }

   unsigned __int64 GetUsedSize() {
     return fUsedBytes;
   }

   DWORD GetClusterSize() {
     return fClusterSize;
   }
     
   int GetPartitionType() {
     return fPartitionType;
   }

   bool IsKnownType() {
     return fKnownType;
   }

   bool IsReadable() {
		 return fReadable;
	 }

   bool IsWritable() {
		 return fWritable;
	 }

   // if it is a disk returns number of contained partitions, if ist is partition returns 0
   int GetContainedVolumes() {
     return fContainedVolumes;
   }

   // returns true if it is a hard disk, returns false if it is a partition
   bool IsCompleteHardDisk() {
     return fIsDisk;
   }

   void Refresh(void);
 private:

  private:
    std::wstring fDisplayName, fDeviceName, fMountPoint;
    TDeviceType fDriveType;
    __int64 fBytes, fSectors;
    unsigned __int64 fUsedBytes;
    int fBytesPerSector, fSectorsPerTrack;
    DWORD fClusterSize;
    bool fReadable;
    bool fWritable;
    int fPartitionType;
    bool fKnownType;
    int fContainedVolumes; // number of volumes contained in a physical disk
    bool fIsDisk;           // true: is a complete hard disk, false: is a partition
};

#include <vector>

//---------------------------------------------------------------------------
class CDriveList {
  public:

     CDriveList(bool ShowProgress);
     ~CDriveList();

  private:
    std::vector<CDriveInfo*> fDriveList;
  
  public:

	CDriveInfo *GetItem(int index)
	{
		return  fDriveList[index]; 
	}

  size_t GetCount(void) 
	{ 
		return fDriveList.size(); 
	}

  // return index of list for given drive letter or -1 if not found
  int GetIndexOfDrive(LPCWSTR drive) const;
};
//---------------------------------------------------------------------------
#endif
