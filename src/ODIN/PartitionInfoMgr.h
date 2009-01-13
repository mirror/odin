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
// Class to store, manage and retriebe the partition information of a disk
//
///////////////////////////////////////////////////////////////////////////// 

class CPartitionInfoMgr {

public:

  typedef struct {
    GUID guid;                            // UUID to identify file format
    WORD versionMajor;                    // major version of file format
    WORD versionMinor;                    // minor version of file format
    DWORD partitionCount;                 // number of partition entries in file
    unsigned __int64 diskExtent;          // size of disk
    DWORD bootLoaderOffset;               // offset in file where boot loader is located
    DWORD bootLoaderLength;               // length of boot loader in bytes
    DWORD partInfoOffset;                 // offset in file where partion information is located
    DWORD partInfoLength;                 // length of partition information in bytes
  } TPartitionInfoFileHeader;

  typedef struct {
    BYTE bootLoader[512];                 // Boot loader code of MBR
    // Note: we can't use 440 Bytes here, because you can only read entire sectors
  } TBootLoader;

  CPartitionInfoMgr();
  ~CPartitionInfoMgr();

  // Read drive layout and partition table from a disk device
  void ReadPartitionInfoFromDisk(const wchar_t* driveName);

  // Write drive layout and partition table to a disk device
  // CAUTION: This makes the whole exsting content of the disk unreadable!!!
  void WritePartitionInfoToDisk(const wchar_t* driveName);
  
   // Read drive layout and partition table from a file
  void ReadPartitionInfoFromFile(const wchar_t* fileName);

   // Write drive layout and partition table to a file
  void WritePartitionInfoToFile(const wchar_t* fileName);

  unsigned GetPartitionCount() const;

  // Mark all partitions as writable so that they can be written to the disk
  void MakeWritable();

  bool IsValidFileHeader(const wchar_t* fileName);

  unsigned __int64 GetDiskSize() {
    return fDiskSize;
  }
  

private:
  CPartitionInfoMgr(BYTE*buffer); // constructor only use for unit tests
  BYTE* GetBuffer() {             // only for unit tests
    return fBuffer;
  } 
  void ClearPartitionInfo(int startIndex=0);      // only for Unit tests, extremely dangerous!!

  bool IsValidFileHeader(const TPartitionInfoFileHeader& header) const;
  bool IsSupportedVersion(const TPartitionInfoFileHeader& header) const;
  void PrepareFileHeader(TPartitionInfoFileHeader& header);
  DWORD CalcBufferSize() const;
  void DumpPartitionInfo();
  void Reset();

  unsigned fPartitionCount;
  unsigned __int64 fDiskSize;
  BYTE* fBuffer;
  TBootLoader fBootLoader;

  static const unsigned sMaxSupportedPartitions; 
  static const GUID sMagicFileHeaderGUID;
  static const WORD sVerMajor;
  static const WORD sVerMinor;

  friend class PartitionInfoMgrTest;
};

