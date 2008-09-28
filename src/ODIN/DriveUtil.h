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
 
#include <string>
#include <vector>
//---------------------------------------------------------------------------
// Some ntdll.dll stuff - we use some undocumented NT system calls to read
// the /device directory and get all the disk devices
//
#pragma once

typedef long NTSTATUS;

struct UNICODE_STRING {
    WORD Length;
    WORD MaximumLength;
    WCHAR *Buffer;
};

struct OBJECT_ATTRIBUTES {
  DWORD           Length;
  HANDLE          RootDirectory;
  UNICODE_STRING *ObjectName;
  DWORD           Attributes;
  void           *SecurityDescriptor;
  void           *SecurityQualityOfService;
};

struct OBJDIR_INFORMATION {
  UNICODE_STRING  ObjectName;
  UNICODE_STRING  ObjectTypeName;
  char           *Data;
};

struct IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID Pointer;
  };
  ULONG_PTR Information;
};

extern bool HaveNTCalls;

typedef NTSTATUS (__stdcall *NtOpenDirectoryObject_t)(HANDLE *DirObjectHandle, ACCESS_MASK DesiredAccess,
                             OBJECT_ATTRIBUTES *ObjectAttributes);
typedef NTSTATUS (__stdcall *NtQueryDirectoryObject_t)(HANDLE DirObjectHandle, OBJDIR_INFORMATION *DirObjInformation,
                             DWORD BufferLength, DWORD GetNextIndex, DWORD IgnoreInputIndex, DWORD *ObjectIndex, DWORD *DataWritten);
typedef NTSTATUS (__stdcall *NtOpenSymbolicLinkObject_t)(HANDLE *SymLinkObjHandle, ACCESS_MASK DesiredAccess,
                             OBJECT_ATTRIBUTES *ObjectAttributes);
typedef NTSTATUS (__stdcall *NtQuerySymbolicLinkObject_t)(HANDLE SymLinkObjHandle, UNICODE_STRING *LinkName, DWORD *DataWritten);
typedef NTSTATUS (__stdcall *NtCreateFile_t)(HANDLE *FileHandle, ACCESS_MASK DesiredAccess, OBJECT_ATTRIBUTES *ObjectAttributes,
                            IO_STATUS_BLOCK *IoStatusBlock, LARGE_INTEGER *AllocationSize, ULONG FileAttributes, ULONG ShareAccess,
                            ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
typedef NTSTATUS (__stdcall *RtlInitUnicodeString_t)(UNICODE_STRING* DestinationString, PCWSTR SourceString);
typedef  BOOL (__stdcall *GetVolumeNameForVolumeMountPointW_t)(LPCWSTR lpszVolumeMountPoint, LPWSTR lpszVolumeName, DWORD cchBufferLength);

bool GetNTLinkDestination(LPCWSTR Source, std::wstring& linkDest);
void GetNTDirectoryObjectContents(LPCWSTR Directory, std::vector<std::wstring>&);
bool GetVolumeNameForMountPoint(LPCWSTR lpszVolumeMountPoint, LPWSTR lpszVolumeName, DWORD cchBufferLength);
long NTOpen(HANDLE* hFile, LPCWSTR FileName, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions);

#define OBJ_INHERIT          0x00000002
#define OBJ_PERMANENT        0x00000010
#define OBJ_EXCLUSIVE        0x00000020
#define OBJ_CASE_INSENSITIVE 0x00000040
#define OBJ_OPENIF           0x00000080
#define OBJ_OPENLINK         0x00000100
#define OBJ_VALID_ATTRIBUTES 0x000001F2

#define DIRECTORY_QUERY      0x0001
#define SYMBOLIC_LINK_QUERY  0x0001

#define NT_SUCCESS(x) (x>=0)

#define FILE_SUPERSEDE                          0x00000000
#define FILE_OPEN                               0x00000001
#define FILE_CREATE                             0x00000002
#define FILE_OPEN_IF                            0x00000003
#define FILE_OVERWRITE                          0x00000004
#define FILE_OVERWRITE_IF                       0x00000005
#define FILE_MAXIMUM_DISPOSITION                0x00000005

#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080
#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_FOR_RECOVERY                  0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800
#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000
#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000
#define FILE_COPY_STRUCTURED_STORAGE            0x00000041
#define FILE_STRUCTURED_STORAGE                 0x00000441
#define FILE_VALID_OPTION_FLAGS                 0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00000036
