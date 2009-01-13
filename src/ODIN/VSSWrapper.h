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
// wrapper class to encapsulate the details of Volume Shadow Copy Service
//
///////////////////////////////////////////////////////////////////////////// 
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <string>
#include <vector>
#include "vsbackup_xp.h"

class CWriterComponent;
class CWriter;

class CVssWrapper {
  
public:
  CVssWrapper();
  ~CVssWrapper();
  void PrepareSnapshot(const wchar_t** volumeNames, int volumeCount);
  void ReleaseSnapshot(bool wasAborted);
  const wchar_t* GetSnapshotDeviceName(int driveIndex);

  // return true if VSS service is supported on this platform or false if not
  static bool VSSIsSupported();
  static bool Is32BitProcessRunningInWow64();

private:
  void Init();
  void CalculateSourcePath(LPCTSTR wszSnapshotDevice, LPCTSTR wszBackupSource, LPCTSTR wszMountPoint, std::wstring& output);
  void Cleanup();
  bool ShouldAddComponent(CWriterComponent& component);
  bool EndsWith(LPCTSTR wsz, size_t maxLength, TCHAR wchar);
  void CombinePath(LPCTSTR wszPath1, LPCTSTR wszPath2, std::wstring& output);
  void ReportBackupResultsToWriters(bool successful);

  CComPtr<IVssBackupComponentsXP> fBackupComponentsXP; 
  CComPtr<IVssBackupComponents> fBackupComponents; 
  std::vector<CWriter> fWriters;

  GUID fSnapshotSetId;
  bool runsOnWinXP;
  HMODULE fVssDLL;
  bool fSnapshotCreated; 
  bool fAbnormalAbort; 
  bool fAbortFromException;
  GUID *fSnapshotIds;
  std::wstring *fSnapshotDeviceNames;
  unsigned fSnapshotCount;
};
