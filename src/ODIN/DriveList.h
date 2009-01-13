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
   CDriveInfo(
     const std::wstring& displayName, 
     const std::wstring& deviceName, 
     const std::wstring& mountPoint, 
     bool isDisk);

   const std::wstring& GetDisplayName () const {
		 return fDisplayName;
	 }
     
	 const std::wstring& GetDeviceName() const {
		 return fDeviceName;
	 }

   const std::wstring& GetMountPoint() const {
		 return fMountPoint;
	 }

   TDeviceType GetDriveType() const {
		 return fDriveType;
	 }

   __int64 GetBytes() const {
		return fBytes;
	 }
     
	 __int64 GetSectors() const { 
		 return fSectors;
	 }

   int GetBytesPerSector() const {
		 return fBytesPerSector;
	 }

   int  GetSectorsPerTrack() const {
		 return fSectorsPerTrack;
	 }

   unsigned __int64 GetUsedSize() const {
     return fUsedBytes;
   }

   DWORD GetClusterSize() const {
     return fClusterSize;
   }
     
   int GetPartitionType() const {
     return fPartitionType;
   }

   unsigned GetDiskNumber();

   bool IsKnownType() const {
     return fKnownType;
   }

   bool IsReadable() const {
		 return fReadable;
	 }

   bool IsWritable() const {
		 return fWritable;
	 }

   // if it is a disk set number of contained partitions
   void SetContainedVolumes(int volumeCount) {
     fContainedVolumes = volumeCount;
   }

   // if it is a disk returns number of contained partitions, if ist is partition returns 0
   int GetContainedVolumes() const {
     return fContainedVolumes;
   }

   // returns true if it is a hard disk, returns false if it is a partition
   bool IsCompleteHardDisk() const {
     return fIsDisk;
   }

   void SetParent(CDriveInfo* pParent) {
     fParent = pParent;
   }

   const CDriveInfo* GetParent() const {
     return fParent;
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
    CDriveInfo* fParent;   // parent object e.g. hard disk of a partition
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

	CDriveInfo *GetItem(int index) const
	{
		return  fDriveList[index]; 
	}

  size_t GetCount(void) const
	{ 
		return fDriveList.size(); 
	}

  // return index of list for given drive letter or -1 if not found
  int GetIndexOfDrive(LPCWSTR drive) const;

  // return index of list for given device name or -1 if not found
  int GetIndexOfDeviceName(const std::wstring& deviceName) const;

  // fill the array pVolumes with all contained volumes of pDriveInfo, pDriveInfo must point to
  // a harddisk device otherwise the function does nothing, return the number of found volumes
  int GetVolumes(const CDriveInfo* pDriveInfo, CDriveInfo** pVolumes, int volumeCount) const;
};
//---------------------------------------------------------------------------
#endif
