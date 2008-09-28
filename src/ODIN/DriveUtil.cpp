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
 
// Do not use precompiled headers for this file as there are conflicts between SDK and DDK.
#include "stdafx.h"

//#include <excpt.h>
//#include <ntddk.h>
#include <string>
#include <vector>
#include <WinDef.h>
#include <WinBase.h>
// #include <winternl.h>
//#include <Ntddk.h>
//#include <Wdm.h>
// #include <ntstatus.h>

using namespace std;

#include "DriveUtil.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

//---------------------------------------------------------------------------
// global helper functions
//
void InitializeUtilityFunctions(void);

class DummyClass
{
	public:
	DummyClass() {
		InitializeUtilityFunctions();
	}
};
static DummyClass doInit;

bool HaveNTCalls;

static NtOpenDirectoryObject_t     NtOpenDirectoryObject     = NULL;
static NtQueryDirectoryObject_t    NtQueryDirectoryObject    = NULL;
static NtOpenSymbolicLinkObject_t  NtOpenSymbolicLinkObject  = NULL;
static NtQuerySymbolicLinkObject_t NtQuerySymbolicLinkObject = NULL;
static RtlInitUnicodeString_t      RtlInitUnicodeString      = NULL;
static NtCreateFile_t              NtCreateFile              = NULL;
static GetVolumeNameForVolumeMountPointW_t GetVolumeNameForMountPointW = NULL;


void InitializeUtilityFunctions(void) 
{
  HMODULE ntdll;
  HMODULE kernel32dll;
  OSVERSIONINFO Version;

  // We used to just depend on the pressence of ntdll.dll to tell if this is running on an NT-based system
  // but things like SysInternal's NTFS for Windows 98 adds this DLL, so now we actually do the official version
  // test and go from there.  This will also filter out Windows NT 4.0, which probably wouldn't work either.
  Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&Version);
  if (Version.dwMajorVersion >= 5 && Version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    ntdll = GetModuleHandle(L"ntdll.dll");
    kernel32dll = GetModuleHandle(L"kernel32.dll");
    if (ntdll && kernel32dll) {
      NtOpenDirectoryObject     = (NtOpenDirectoryObject_t)    GetProcAddress(ntdll, "NtOpenDirectoryObject");
      NtQueryDirectoryObject    = (NtQueryDirectoryObject_t)   GetProcAddress(ntdll, "NtQueryDirectoryObject");
      NtOpenSymbolicLinkObject  = (NtOpenSymbolicLinkObject_t) GetProcAddress(ntdll, "NtOpenSymbolicLinkObject");
      NtQuerySymbolicLinkObject = (NtQuerySymbolicLinkObject_t)GetProcAddress(ntdll, "NtQuerySymbolicLinkObject");
      NtCreateFile              = (NtCreateFile_t)             GetProcAddress(ntdll, "NtCreateFile");
      RtlInitUnicodeString      = (RtlInitUnicodeString_t)     GetProcAddress(ntdll, "RtlInitUnicodeString");
      GetVolumeNameForMountPointW= (GetVolumeNameForVolumeMountPointW_t)GetProcAddress(kernel32dll, "GetVolumeNameForVolumeMountPointW");
      if (NtOpenDirectoryObject && NtQueryDirectoryObject && NtOpenSymbolicLinkObject && NtCreateFile && RtlInitUnicodeString && GetVolumeNameForMountPointW) {
        HaveNTCalls = true;
        ATLTRACE("InitializeUtilityFunctions() HaveNTCalls=true;\n");
      }
      else {
        HaveNTCalls = true;
        ATLTRACE("InitializeUtilityFunctions() HaveNTCalls=false;\n");
      }

    }  // if (ntdll)
  }  // if (Version.dwMajorVersion >= 5)
}  // void __fastcall InitializeUtilityFunctions(void)

bool GetNTLinkDestination(LPCWSTR Source, wstring& linkDest)
{
  UNICODE_STRING usName;
  OBJECT_ATTRIBUTES oa;
  NTSTATUS nStatus;
  HANDLE hLink;
  wchar_t *buffer;
  bool retval = false;

  RtlInitUnicodeString(&usName, Source);
  oa.Length = sizeof(OBJECT_ATTRIBUTES);
  oa.RootDirectory = 0;
  oa.ObjectName = &usName;
  oa.Attributes = OBJ_CASE_INSENSITIVE;
  oa.SecurityDescriptor = NULL;
  oa.SecurityQualityOfService = NULL;
  nStatus = NtOpenSymbolicLinkObject(&hLink, SYMBOLIC_LINK_QUERY, &oa);
  if (NT_SUCCESS(nStatus)) {
    buffer = (wchar_t *)malloc(2048);
    usName.Length = 0;
    usName.MaximumLength = 1024;
    usName.Buffer = buffer;
    nStatus = NtQuerySymbolicLinkObject(hLink, &usName, NULL);
    usName.Buffer[usName.Length/2] = 0;
    if (NT_SUCCESS(nStatus))
      linkDest = usName.Buffer;
    free(buffer);
	retval = true;
  }  // if (NTSUCCESS(nStatus))
  return retval;
}

//---------------------------------------------------------------------------
// Wrapper for the Windows NT-based GetVolumeNameForVolumeMountPointA()
// function.  It is wrapped here to keep all dynamic function calls here in
// one unit.  We dynamically get the function because it doesn't exist in
// Windows 98.  It would be nice if delayed loading could keep this from
// throwing an error when run on Windows 98, but you can't specify a DLL
// entry point to delay load, only an entire DLL.  Because this function is
// in kernel32.dll, there is no way to prevent it from tripping the delayed
// loading.
//
bool GetVolumeNameForMountPoint(LPCWSTR lpszVolumeMountPoint, LPWSTR lpszVolumeName, DWORD cchBufferLength)
{
	return GetVolumeNameForMountPointW(lpszVolumeMountPoint, lpszVolumeName, cchBufferLength) != FALSE;
}  // bool GetVolumeNameForMountPoint(LPCSTR lpszVolumeMountPoint, LPSTR lpszVolumeName, DWORD cchBufferLength)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Get the contents of an NT directory object - this uses some undocumented
// calls from ntdll.dll
//
void GetNTDirectoryObjectContents(LPCWSTR Directory, vector<wstring>& entries)
{
  UNICODE_STRING usDir;
  OBJECT_ATTRIBUTES oa;
  HANDLE hDeviceDir;
  NTSTATUS nStatus;
  OBJDIR_INFORMATION *DirInfo;
  DWORD index;
  wstring Error;

  if (!HaveNTCalls)
    return;

  RtlInitUnicodeString(&usDir, Directory);
  oa.Length = sizeof(OBJECT_ATTRIBUTES);
  oa.ObjectName = &usDir;
  oa.Attributes = OBJ_CASE_INSENSITIVE;
  oa.SecurityDescriptor = NULL;
  oa.SecurityQualityOfService = NULL;
  oa.RootDirectory = 0;
  nStatus = NtOpenDirectoryObject(&hDeviceDir, STANDARD_RIGHTS_READ | DIRECTORY_QUERY, &oa);
  if (!NT_SUCCESS(nStatus)) {
    HaveNTCalls = false;
    ATLTRACE("GetNTDirectoryObjectContents() HaveNTCalls=false;\n");
    return;
  }  // if (!NT_SUCCESS(nStatus))
  DirInfo = (OBJDIR_INFORMATION *)malloc(2048);
  index = 0;
  while (NT_SUCCESS(NtQueryDirectoryObject(hDeviceDir, DirInfo, 1024, true, false, &index, NULL)))
  {
      wstring entry = wstring(Directory) + L"\\" + DirInfo->ObjectName.Buffer;
	  entries.push_back(entry);
  }
  CloseHandle(hDeviceDir);
  free(DirInfo);
}   // TStringList * __fastcall GetNTDirectoryObjectContents(WideString Directory)
//---------------------------------------------------------------------------

#define FILE_SUPERSEDED     (0)
#define FILE_OPENED         (1)
#define FILE_CREATED        (2)
#define FILE_OVERWRITTEN    (3)
#define FILE_EXISTS         (4)
#define FILE_DOES_NOT_EXIST	(5)
//---------------------------------------------------------------------------
// This is a wrapper function around the native NtCreateFile() function.
// Officially, NtCreateFile() is undocumented, but it is widely used.
//
long NTOpen(HANDLE* hFile, LPCWSTR FileName, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, 
              ULONG CreateDisposition, ULONG CreateOptions)
{
  UNICODE_STRING usFileName;
  OBJECT_ATTRIBUTES oa;
  IO_STATUS_BLOCK ios;
  NTSTATUS ntStatus;

  if (!HaveNTCalls)
    return -1;
  *hFile = INVALID_HANDLE_VALUE;
  RtlInitUnicodeString(&usFileName, FileName);
  ZeroMemory(&ios, sizeof(ios));
  oa.Length = sizeof(oa);
  oa.RootDirectory = 0;
  oa.ObjectName = &usFileName;
  oa.Attributes = OBJ_CASE_INSENSITIVE;
  oa.SecurityDescriptor = NULL;
  oa.SecurityQualityOfService = NULL;
  ntStatus = NtCreateFile(hFile, DesiredAccess, &oa, &ios, NULL, FileAttributes, ShareAccess, 
              CreateDisposition, CreateOptions, NULL, 0);
  return ntStatus;
} 
//---------------------------------------------------------------------------
