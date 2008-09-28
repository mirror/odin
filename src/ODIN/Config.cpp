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
#include <ShlObj.h>
#include "Config.h"
#include "InternalException.h"

//////////////////////////////////////////////////////////////////////////////
// static declarations
//////////////////////////////////////////////////////////////////////////////

CIniWrapper CConfigEntryStatics::sIni;
extern const wchar_t defaultSectionName[] = L"Standard";
CStringTable CConfigEntryStatics::sStringTable(2048);

  // return TRUE if the executable was run from a USB-Stick, Floppy etc
bool ProcessLoadedFromRemovableDrive() {
  const int bufferSize = MAX_PATH;
  wchar_t buffer[bufferSize];
  DWORD res;

  res = GetModuleFileName(NULL, buffer, bufferSize);
  if (res > 0) {
    if (PathStripToRoot(buffer)) {
      if (GetDriveType(buffer) == DRIVE_REMOVABLE) {
        return true;
      }
    }
  }
  return false;
}

  // fill dir with the directory from where the process was loaded
BOOL GetModuleDir(LPTSTR dir, DWORD dirLen) {
  DWORD res = GetModuleFileName(NULL, dir, dirLen);
  BOOL ok;
  if (res > 0 && res < dirLen) {
    ok = PathRemoveFileSpec(dir);
  } else
    ok = FALSE;
  return ok;
}

bool CfgFileInitialize(const wchar_t* iniFileName, bool forceModuleDir) {
  wchar_t buffer[MAX_PATH];

  // if process was load from an USB stick store ini file where .exe was loaded. If it was loaded from
  // a hard disk store it in the usual AppData location
  if (forceModuleDir || ProcessLoadedFromRemovableDrive()) {
    GetModuleDir(buffer, MAX_PATH);
  } else {
    HRESULT hr = SHGetFolderPathAndSubDir(NULL, CSIDL_FLAG_CREATE|CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, L"ODIN", buffer);
    if (hr != S_OK) {
      _wgetcwd(buffer, MAX_PATH);
    }
  }
  wcscat_s(buffer, MAX_PATH, L"\\");
  wcscat_s(buffer, MAX_PATH, iniFileName);
  CConfigEntryStatics::sIni.SetPathName(buffer);
  return true;
}

//////////////////////////////////////////////////////////////////////////////
// class CStringTable
//////////////////////////////////////////////////////////////////////////////

CStringTable::CStringTable(int capacity) {
  fCapacity = capacity;
  fStringTable = new wchar_t[fCapacity];
}

CStringTable::~CStringTable() {
  delete fStringTable;
}

// Add a string to a table and return a reference to the location in the store
// return NULL if no mor space is available
const wchar_t* CStringTable::AddString(const wchar_t* str) {
  size_t len = wcslen(str)+1;
  if (len>fCapacity-fSize)
    THROW_INT_EXC(EInternalException::internalStringTableOverflow);
  else {
    wchar_t* p=fStringTable+fSize;
    wcsncpy_s(p, fCapacity-fSize, str, len);
    fSize += len;
    return p;
  }
}

//////////////////////////////////////////////////////////////////////////////
// global helper functions
//////////////////////////////////////////////////////////////////////////////
/*
int GetIniValue(const wchar_t* key, const wchar_t* section, int defaultVal) {
  return sIni.GetInt(section, key, defaultVal);
}

void SetIniValue(const wchar_t* key, const wchar_t* section, int value) {
  sIni.WriteInt(section, key, value);
}
*/
//////////////////////////////////////////////////////////////////////////////
// class ConfigEntryInt
//////////////////////////////////////////////////////////////////////////////

