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

#include "stdafx.h"
#include "FileNameUtil.h"
#include <algorithm>
#include "OSException.h"
#include "SplitManagerCallback.h"

using namespace std;

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

bool CFileNameUtil::IsFileReadable(LPCWSTR fileName)
{
  HANDLE h = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
  if (h != INVALID_HANDLE_VALUE)
  {
	  CloseHandle(h);
  }
 
  return h != INVALID_HANDLE_VALUE;
}

bool CFileNameUtil::IsFileWritable(LPCWSTR fileName)
{
  bool fileExists = false;

  DWORD res = GetFileAttributes(fileName);
  if (res != INVALID_FILE_ATTRIBUTES)
    fileExists = true;

  HANDLE h = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
  if (h != INVALID_HANDLE_VALUE)
  {
	  CloseHandle(h);
    if (!fileExists)
      DeleteFile(fileName);
  }
 
  return h != INVALID_HANDLE_VALUE;
}


void CFileNameUtil::GetDirFromFileName(const wstring& fileName, wstring& dir)
{
  wchar_t buffer[MAX_PATH];
  LPWSTR fileNameComp;
  DWORD len;

  len = GetFullPathName(fileName.c_str(), MAX_PATH, buffer, &fileNameComp);
  if (len>MAX_PATH) {
    ATLTRACE("Internal error: not enough space to get full path");
    dir.clear();
  }
  wstring absPath(buffer, len);
  size_t pos = absPath.rfind(L'\\');
  if (pos == string::npos) {
      ATLTRACE("Error in getting current directory for abs path %S", absPath);
      dir.clear();
  } else {
    dir = absPath.substr(0, pos);
  }
}

void CFileNameUtil::GetDriveFromFileName(const wstring& fileName, wstring& drive)
{
  wchar_t buffer[MAX_PATH];
  LPWSTR fileNameComp;
  DWORD len;

  len = GetFullPathName(fileName.c_str(), MAX_PATH, buffer, &fileNameComp);
  if (len>MAX_PATH) {
    ATLTRACE("Internal error: not enough space to get full path");
    drive.clear();
  }
  wstring absPath(buffer, len);
  size_t pos = absPath.find(L'\\');
  if (pos == string::npos) {
      ATLTRACE("Error in getting current directory for abs path %S", absPath);
      drive.clear();
  } else {
    drive = absPath.substr(0, pos+1);
  }
  // convert always to uppercase to be compatible with drive info
  transform(drive.begin(), drive.end(), drive.begin(), (int (*)(int))toupper);
}

void CFileNameUtil::GetFreeBytesOfDisk(LPCWSTR dir, unsigned __int64* freeBytesAvailable, unsigned __int64* totalNumberOfBytes)
{
  ULARGE_INTEGER  freeBytesAvailable2, totalNumberOfFreeBytes, totalNumberOfBytes2;
  BOOL ok = GetDiskFreeSpaceEx(dir, &freeBytesAvailable2, &totalNumberOfBytes2, &totalNumberOfFreeBytes);
  CHECK_OS_EX_PARAM1(ok, EWinException::generalFileError, dir);
  if (ok) {
    *freeBytesAvailable = freeBytesAvailable2.QuadPart;
    *totalNumberOfBytes = totalNumberOfBytes2.QuadPart;
  }
}

void CFileNameUtil::GenerateFileNameForEntireDiskBackup(wstring &volumeFileName /*out*/, LPCWSTR imageFileNamePattern, const wstring& partitionDeviceName)
{
  wstring tmp(imageFileNamePattern);
  size_t pos = partitionDeviceName.rfind(L"Partition");
  size_t posDot = tmp.rfind(L'.');
  if (posDot == string::npos)
    posDot = tmp.length();
  wstring ext(tmp.substr(posDot));
  ext = L".img";
  volumeFileName = tmp.substr(0, posDot);
  volumeFileName += L"-";
  volumeFileName += partitionDeviceName.substr(pos);
  volumeFileName += ext;
}

void CFileNameUtil::GenerateFileNameForMBRBackupFile(wstring &volumeFileName)
{
    // generate file name for MBR
    wstring mbrFileName = volumeFileName;
    transform(mbrFileName.begin(), mbrFileName.end(), mbrFileName.begin(), tolower);
    const wstring mbrExt(L".mbr");
    if (mbrFileName.length()-4  == mbrFileName.rfind(mbrExt))
      return;
    
    mbrFileName = volumeFileName;
    size_t pos = mbrFileName.rfind(L'.');
    if (mbrFileName.length() - pos == 4) {
      if (mbrFileName.substr(pos) != mbrExt) {
        mbrFileName = mbrFileName.substr(0, pos);
        mbrFileName += mbrExt;
      } 
    } else {
        mbrFileName += mbrExt;
    }
    volumeFileName = mbrFileName;
}

void CFileNameUtil::GenerateDeviceNameForVolume(wstring& volumeFileName, unsigned diskNo, unsigned volumeNo)
{
  const int bufSize = 10;
  wchar_t buffer[bufSize];
  volumeFileName = L"\\Device\\Harddisk";
  _ui64tow_s(diskNo, buffer, bufSize, 10);
  volumeFileName += buffer;
  volumeFileName += L"\\Partition";
  _ui64tow_s(volumeNo, buffer, bufSize, 10);
  volumeFileName += buffer;
}

bool CFileNameUtil::TestIsHardDiskImage(const wchar_t* fileName)
{
  bool isHardDisk;
  wstring mbrName(fileName);

  if (mbrName.rfind(L".mbr") != mbrName.length() - 4)
    GenerateFileNameForMBRBackupFile(mbrName);
  isHardDisk = IsFileReadable(mbrName.c_str());

  return isHardDisk;
}

void CFileNameUtil::GetSplitFilePattern(ISplitManagerCallback& splitCB, LPCWSTR path, wstring& filePattern)
{
  wstring dir;
  size_t off;
  size_t count = 0;

  filePattern = path;
  splitCB.GetFileName(0, filePattern);
  off = filePattern.rfind(L'\\');
  if (off != string::npos)
    dir = filePattern.substr(0, off+1);
  // count number of '0'
  off = filePattern.find(L'0', off);
  while (filePattern.find(L'0', off+count) != string::npos)
    ++count;
  filePattern = filePattern.replace(off, count, count, L'?');
}

void CFileNameUtil::GetEntireDiskFilePattern(LPCWSTR path, wstring& filePattern)
{
  wstring dir;
  size_t off;
  size_t count = 0;

  filePattern = path;
  off = filePattern.rfind(L'\\');
  if (off == string::npos)
    off = 0;
  
  // find '0'
  off = filePattern.find_first_of(L"0123456789", off);
  filePattern = filePattern.replace(off, string::npos, 1, L'*');
}

__int64 CFileNameUtil::GetFileSizeByName(LPCWSTR fileName)
{
  __int64 nnSize = 0;
  LARGE_INTEGER size;

  HANDLE h = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
  if (h != INVALID_HANDLE_VALUE)
  {
    ::GetFileSizeEx(h, &size);
	  CloseHandle(h);
  }
 
  return (__int64) nnSize;
}

void CFileNameUtil::RemoveTrailingNumberFromFileName(wstring& fileName)
{
  const int noTrailingDigits = 4; // name is generated in the form c:\Folder\NameNNNN.ext n=4


  size_t lastDotPos = fileName.rfind(L'.');
  if (lastDotPos < 0)
    lastDotPos = fileName.length();
  
  if ( fileName[lastDotPos-1] >= L'0' && fileName[lastDotPos-1] <= L'9'&& 
       fileName[lastDotPos-2] >= L'0' && fileName[lastDotPos-2] <= L'9'&& 
       fileName[lastDotPos-3] >= L'0' && fileName[lastDotPos-3] <= L'9'&& 
       fileName[lastDotPos-4] >= L'0' && fileName[lastDotPos-4] <= L'9' ) {
    wstring baseName = fileName.substr(0, lastDotPos-4);
    baseName +=fileName.substr(lastDotPos, fileName.length());
    fileName = baseName;
  }
}
