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
#include <winioctl.h>
#pragma hdrstop
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

#include "DriveList.h"
#include "DriveUtil.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

//---------------------------------------------------------------------------
// simple class to measure time for code timings

  // Our first choice is to use the Windows 2000/XP device listing, as it will allow access to unmounted partitions.
  // To get a listing of drives, we get a listing of the virtual \Device directory.  There are a lot more entries in
  // the \Device virtual directory than drives, so we need to also check each result to see if it's what we want.
  

class CStopWatch {
public:
  void Start() {
    QueryPerformanceCounter(&startCounter);
  }

  void Stop() {
    QueryPerformanceCounter(&stopCounter);
  }

  DWORD GetElapsedTime() {
    return ((DWORD) (stopCounter.QuadPart - startCounter.QuadPart) + (countsPerMs>>1)) / countsPerMs;
  }

  void TraceTime(LPCWSTR timerText) {
    ATLTRACE(" Time for %S is: %ums\n", timerText, GetElapsedTime());
  }
private:
  static DWORD Init();
  LARGE_INTEGER startCounter, stopCounter;
  static DWORD countsPerMs;
};

DWORD CStopWatch::countsPerMs = Init();

DWORD CStopWatch::Init() {
  LARGE_INTEGER perfFreq;
  QueryPerformanceFrequency (&perfFreq);
  return (DWORD)((perfFreq.QuadPart + 500) / 1000);
}

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TDriveInfo class implementation
//
// An instantiation of this class represents a single drive/partition.


//---------------------------------------------------------------------------
// Constructor
//
CDriveInfo::CDriveInfo(wstring displayName, wstring deviceName, wstring mountPoint, bool isDisk, int containedVolumes)
{
  fDisplayName = displayName;
  fDeviceName  = deviceName;
  fMountPoint  = mountPoint;
  fDriveType   = driveUnknown;
  fUsedBytes = fBytes = fSectors = fBytesPerSector = fSectorsPerTrack = fClusterSize = 0;
  fReadable = fWritable = fKnownType = false;
  fPartitionType = PARTITION_ENTRY_UNUSED;
  fContainedVolumes = containedVolumes;
  fIsDisk = isDisk;
}


//---------------------------------------------------------------------------
// Refresh all the drive information
//
void CDriveInfo::Refresh(void)
{
  HANDLE hDrive;
  PARTITION_INFORMATION_EX partition;
  DISK_GEOMETRY geometry;
  DWORD nCount;
  BOOL bSuccess;
  long ntStatus;
  // CStopWatch watch;

  // watch.Start();
  if (!HaveNTCalls)
    hDrive = CreateFile(fDeviceName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_BACKUP_SEMANTICS, NULL);
  else
    ntStatus = NTOpen(&hDrive, fDeviceName.c_str(), GENERIC_READ, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_SEQUENTIAL_ONLY);
  // watch.Stop();
  // watch.TraceTime(L"Open device in refresh");
  if (ntStatus == 0) {
/* * /
struct _DriveLayout16 {
DRIVE_LAYOUT_INFORMATION_EX driveLayout;
PARTITION_INFORMATION_EX partitionInfo[15];
} driveLayout16;

//DWORD size = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + 15 * sizeof(PARTITION_INFORMATION_EX);
//DRIVE_LAYOUT_INFORMATION_EX* pBuf = (DRIVE_LAYOUT_INFORMATION_EX*)new BYTE[size]; 
bSuccess = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, &driveLayout16, sizeof(driveLayout16), &nCount, NULL);
if (!bSuccess)
  bSuccess = GetLastError();
//delete pBuf;
/**/
    bSuccess = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geometry, sizeof(geometry), &nCount, NULL);
    if (bSuccess) {
      DeviceIoControl(hDrive, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partition, sizeof(partition), &nCount, NULL);
      CloseHandle(hDrive);
      if (fDriveType == driveFloppy)
        fBytes = geometry.Cylinders.QuadPart * geometry.TracksPerCylinder * geometry.SectorsPerTrack * geometry.BytesPerSector;
      else
        fBytes = partition.PartitionLength.QuadPart;
      fBytesPerSector = geometry.BytesPerSector;
      fSectors = fBytes / fBytesPerSector;
      fSectorsPerTrack = geometry.SectorsPerTrack;
      fReadable = true;
      if (geometry.MediaType == Unknown)
        fDriveType = driveUnknown;
      else if (geometry.MediaType == FixedMedia)
        fDriveType = driveFixed;
      else if (geometry.MediaType == RemovableMedia)
        fDriveType = driveRemovable;
      else
        fDriveType = driveFloppy;

      if (partition.PartitionStyle == PARTITION_STYLE_MBR) {
          fPartitionType = (int) partition.Mbr.PartitionType;
          fKnownType = partition.Mbr.RecognizedPartition != 0;
      } else {// partition.PartitionStyle == PARTITION_STYLE_GPT or PARTITION_STYLE_RAW
        fBytes = 0; fSectors = 0; fBytesPerSector = 0; fSectorsPerTrack = 0; fDriveType = driveUnknown;
        fKnownType = fReadable = false;
      }
    } else { 
      fBytes = 0; fSectors = 0; fBytesPerSector = 0; fSectorsPerTrack = 0; fDriveType = driveUnknown;
      fKnownType = fReadable = false;
    }
  }

  //watch.Start();
  // Is this partition writable?  In Windows, only unmounted partitions can be written to.
  if (fDriveType == driveFloppy)
    fWritable = true;
  else {  
    if (!HaveNTCalls)
      hDrive = CreateFile(fDeviceName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    else
      ntStatus = NTOpen(&hDrive, fDeviceName.c_str(), GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_SEQUENTIAL_ONLY);
               // Note: be aware of SHARE flags, if FILE_SHARE_WRITE is omitted this call takes for a pluggable USB drive up to 10 seconds!
    if (ntStatus == 0) {
      fWritable = true;
      CloseHandle(hDrive);
    } else
      fWritable = false;
  }
  //watch.Stop();
  //watch.TraceTime(L"check write");

  //watch.Start();
  // Try to get allocated size of volume
  if (fMountPoint.length() > 0) {
    ULARGE_INTEGER  freeBytesAvailable, totalNumberOfFreeBytes, totalNumberOfBytes;
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD numberOfFreeClusters;
    DWORD totalNumberOfClusters;

    BOOL ok = GetDiskFreeSpace(fMountPoint.c_str(), &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters);
    if (ok) {
      fClusterSize = bytesPerSector * sectorsPerCluster;
      ATLTRACE("cluster size from GetDiskFreeSpace() for %S is %u\n", fMountPoint.c_str(), fClusterSize);
    } else {
      ATLTRACE("Failed to call GetDiskFreeSpace() for: %S\n", fMountPoint.c_str());
    }
    ok = GetDiskFreeSpaceEx(fMountPoint.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes);
    if (ok) {
      fUsedBytes = totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
    }
  }
  //watch.Stop();
  //watch.TraceTime(L"GetDiskFreeSpaceEx()");

  // Begin Test code just to see how to get cluster size
  if (fMountPoint.length() > 0) {
      DWORD sectorsPerCluster;
      DWORD bytesPerSector;
      DWORD numberOfFreeClusters;
      DWORD totalNumberOfClusters;

    BOOL ok = GetDiskFreeSpace(fMountPoint.c_str(), &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters);
    if (ok) {
      DWORD clusterSize = bytesPerSector * sectorsPerCluster;
      ATLTRACE("cluster size from GetDiskFreeSpace() for %S is %u\n", fMountPoint.c_str(), clusterSize);
    } else {
      ATLTRACE("Failed to call GetDiskFreeSpace() for: %S\n", fMountPoint.c_str());
    }
  // End Test code just to see how to get cluster size
  }
}


//---------------------------------------------------------------------------
// TDriveList class implementation
//
// A list of drives/partitions based on (but not derived from) TList

//---------------------------------------------------------------------------
// Constructor
//
CDriveList::CDriveList(bool ShowProgress)
{
  //CStopWatch watch, watchAll;
  vector<wstring> devices;
  map<wstring, wstring> volumes;

  // Our first choice is to use the Windows 2000/XP device listing, as it will allow access to unmounted partitions.
  // To get a listing of drives, we get a listing of the virtual \Device directory.  There are a lot more entries in
  // the \Device virtual directory than drives, so we need to also check each result to see if it's what we want.
  
  // watch.Start();
  GetNTDirectoryObjectContents(L"\\Device", devices);
  // watch.Stop();
  // watch.TraceTime(L"GetNTDirectoryObjectContents()");

  if (devices.size() > 0) {
    // Before enumerating the devices to search for drives, cross reference all the volume links to drive letters
    // we need this later to determine which drive letters each harddisk device is mounted as.
    wchar_t *buffer = new wchar_t [1024];

    for (wchar_t d = L'C'; d <= L'Z'; d++) {
      wstring drive;
      drive = d;
      drive += L":\\";
      if (GetVolumeNameForMountPoint(drive.c_str(), buffer, 1000)) {
        wstring volume = buffer;
        wstring param = L"\\??\\" + volume.substr(4, volume.length()-5);
        wstring volumeLink;
        bool ok = GetNTLinkDestination(param.c_str(), volumeLink);
        if (ok)
          volumes[volumeLink] = drive;
      } 
    }  
    delete buffer;

    // watchAll.Start();
    // Now we enumerate all the devices in \Device
    for (unsigned n = 0; n < devices.size(); n++) {
      wstring deviceName = devices[n], displayName, mountPoint;
      size_t length = deviceName.length();
      //ATLTRACE(" Found device name [%d]: %S Length: %d\n", n, deviceName.c_str(), Length);
      if ((length == 15 && deviceName.find(L"Floppy") == 8) || (length == 17 && deviceName.find(L"Harddisk") == 8)) {
        displayName = deviceName;
        mountPoint = volumes[deviceName];
        if (!mountPoint.length() == 0)
          displayName += L" (" + mountPoint + L")";
        if (deviceName.find(L"Harddisk") != string::npos) {
          vector<wstring> partitions;
          int partitionCount = 0;
          GetNTDirectoryObjectContents(deviceName.c_str(), partitions);
          deviceName += L"\\Partition0";
          //deviceName = partitions[0];
          displayName += L" (entire disk)";
          // If this is a hard disk, check for and add any partitions
          if (partitions.size() > 0) {
            for (unsigned i = 0; i < partitions.size(); i++) {
              wstring partitionDeviceName = partitions[i], partitionDisplayName, partitionMountPoint;
              ATLTRACE(" Found partition device name [%d]: %S Length: %d\n", i, partitionDeviceName.c_str(), partitionDeviceName.length());
              if ((partitionDeviceName.find(L"Partition0") == string::npos) && (partitionDeviceName.find(L"Partition") == 18) && (partitionDeviceName.length() == 28) ) {
                partitionCount++; // we found a real partition
                CDriveInfo *partition;
                wstring volumeLink;
                partitionDisplayName = partitionDeviceName;
                bool ok = GetNTLinkDestination(partitionDeviceName.c_str(), volumeLink);
                partitionMountPoint = volumes[volumeLink];
                if (partitionMountPoint.length() > 0)
                  partitionDisplayName += L" ("+ partitionMountPoint + L")";
                partition = new CDriveInfo(partitionDisplayName, partitionDeviceName, partitionMountPoint, false);
                fDriveList.push_back(partition);
              }  
            }  
          } // if
          bool skipEntireDiskPartion = false;
          if (partitionCount == 1) {
            // if we have only one partition and this is same size as partition0 then it is an unpartioned device
            // like a memory stick and we omit the entire disk as this is redundant
            PARTITION_INFORMATION_EX partition;
            unsigned __int64 bytes1, bytes2;
            DWORD nCount;
            long ntStatus;
            HANDLE hDrive;

            ntStatus = NTOpen(&hDrive, deviceName.c_str(), GENERIC_READ, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_SEQUENTIAL_ONLY);
            DeviceIoControl(hDrive, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partition, sizeof(partition), &nCount, NULL);
            bytes1 = partition.PartitionLength.QuadPart;
            CloseHandle(hDrive);
            // find correct partition
            for (unsigned i = 0; i < partitions.size(); i++) {
              wstring partitionDeviceName = partitions[i];
              if ((partitionDeviceName.find(L"Partition0") == string::npos) && (partitionDeviceName.find(L"Partition") == 18) && (partitionDeviceName.length() == 28) ) {
                ntStatus = NTOpen(&hDrive, partitionDeviceName.c_str(), GENERIC_READ, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_SEQUENTIAL_ONLY);
                DeviceIoControl(hDrive, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partition, sizeof(partition), &nCount, NULL);
                bytes2 = partition.PartitionLength.QuadPart;
                CloseHandle(hDrive);
              }
            }
            skipEntireDiskPartion = bytes1==bytes2; // does root partition and first partions have same size?
          }
          if (!skipEntireDiskPartion) {
            CDriveInfo *drive = new CDriveInfo(displayName, deviceName, mountPoint, true, partitionCount);
            fDriveList.push_back(drive);
          }
        } // find hard disk
        else { // not a hard disk must be a floppy
          CDriveInfo *drive = new CDriveInfo(displayName, deviceName, mountPoint, false);
          fDriveList.push_back(drive);
        }
      } // if 
    } // for
    // watchAll.Stop();
    // watchAll.TraceTime(L"checking all devices");


  // Refresh all the drives - this can take some time as we attempt to lock each drive for writing to see which
  // drives can and can't be written to.  We optionally display a progress dialog for this part.
    // watchAll.Start();
    for (unsigned n=0; n < fDriveList.size(); n++) {
      fDriveList[n]->Refresh();
    }  
    // watchAll.Stop();
    // watchAll.TraceTime(L"Refreshing all devices");
  }  
}

//---------------------------------------------------------------------------
// Destructor
//
CDriveList::~CDriveList()
{
  for (unsigned n = 0; n < fDriveList.size(); n++)
    delete fDriveList[n];
} 

//---------------------------------------------------------------------------

  // return index of list for given drive letter or -1 if not found
int CDriveList::GetIndexOfDrive(LPCWSTR drive) const
{
  wstring driveStr(drive);
  
  if (driveStr[driveStr.length()-1] != L'\\')
    driveStr += L'\\';
  transform(driveStr.begin(), driveStr.end(), driveStr.begin(), (int (*)(int))toupper);
  for (size_t i=0; i<fDriveList.size(); i++)
    if (fDriveList[i]->GetMountPoint() == driveStr)
      return (int)i;
  return -1;
}
