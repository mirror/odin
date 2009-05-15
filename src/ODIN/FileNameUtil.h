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
// Collection of utility functions around file naming conventions, file
// name manipulations and files
//
///////////////////////////////////////////////////////////////////////////// 

#pragma once
#include <string>
class CDriveInfo;
class ISplitManagerCallback;

class CFileNameUtil {
public:

  static bool IsFileWritable(LPCWSTR fileName);
  static bool IsFileReadable(LPCWSTR fileName);
  static void GetFreeBytesOfDisk(LPCWSTR dir, unsigned __int64* freeBytesAvailable, unsigned __int64* totalNumberOfBytes);
  static void GetDriveFromFileName(const std::wstring& fileName, std::wstring& drive);
  static void GetDirFromFileName(const std::wstring& fileName, std::wstring& dir);
  static void GenerateFileNameForEntireDiskBackup(std::wstring &volumeFileName /*out*/, LPCWSTR imageFileNamePattern, const std::wstring& partitionDeviceName);
  static void GenerateFileNameForMBRBackupFile(std::wstring &volumeFileName);
  static void GenerateDeviceNameForVolume(std::wstring& volumeFileName, unsigned diskNo, unsigned volumeNo);
  static bool TestIsHardDiskImage(const wchar_t* fileName);
  static void GetSplitFilePattern(ISplitManagerCallback& splitCB, LPCWSTR path, std::wstring& filePattern);
  static void GetEntireDiskFilePattern(LPCWSTR path, std::wstring& filePattern);
  static __int64 GetFileSizeByName(LPCWSTR fileName);
  static void RemoveTrailingNumberFromFileName(std::wstring& fileName); 


};
