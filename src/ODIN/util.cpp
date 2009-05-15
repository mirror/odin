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
// Collection of utility functions
//
///////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include "Util.h"
#include "atlmisc.h"
#include "resource.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

void MakeByteLabel(unsigned __int64 byteCount, LPWSTR buffer, size_t bufsize) {
  unsigned labelVal1, labelVal2;
  LPCWSTR labelSuffix;

  if (byteCount >= 1099511627776LL) {
    labelVal1 = (unsigned)(byteCount / 1099511627776LL);
    unsigned __int64 tmp = ((byteCount % 1099511627776LL)); // same as: labelVal2 / (1023 << 20)
    labelVal2 = (unsigned)((tmp & (1023 << 30)) >> 30);
    labelSuffix = L"TB";
  } else if (byteCount >= 1073741824LL) {
    labelVal1 = (unsigned)(byteCount / 1073741824LL);
    labelVal2 = ((unsigned)(byteCount % 1073741824LL));
    labelVal2 = (labelVal2 & (1023 << 20)) >> 20; // same as: labelVal2 = labelVal2 / (1023 << 10);
    labelSuffix = L"GB";
  } else if (byteCount >= 1048576LL) {
    labelVal1 = (unsigned)(byteCount / 1048576LL);
    labelVal2 = ((unsigned)byteCount) % 1048576;
    labelVal2 = (labelVal2 & (1023 << 10)) >> 10; 
    labelSuffix = L"MB";
  } else if (byteCount >= 1024LL) {
    labelVal1 = (unsigned)(byteCount / 1024LL);
    labelVal2 = ((unsigned)byteCount) & 1023L;
    labelSuffix = L"KB";
  } else {
    labelVal1 = ((unsigned)byteCount);
    labelVal2 = 0;
    labelSuffix = L"B";
  } 
  labelVal2 = labelVal2 * 1000 / 1024;
  swprintf(buffer, bufsize, L"%u.%03u%s", labelVal1, labelVal2, labelSuffix);
}

void GetDriveTypeString(enum TDeviceType driveType, std::wstring& driveTypeStr)
{
  WTL::CString driveTypeString;
  switch (driveType) {
    case driveFixed:
      driveTypeString.LoadString(IDS_DRIVE_TYPE_FIXED);
      break;
    case driveRemovable:
      driveTypeString.LoadString(IDS_DRIVE_TYPE_REMOVABLE);
      break;
    case driveFloppy:
      driveTypeString.LoadString(IDS_DRIVE_TYPE_FLOPPY);
      break;
    default:
      driveTypeString.LoadString(IDS_DRIVE_TYPE_UNKNOWN);
  }
  
  driveTypeStr = (LPCWSTR)driveTypeString;
}
