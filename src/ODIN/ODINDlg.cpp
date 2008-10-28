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
 
/////////////////////////////////////////////////////////////////////////////
//
// Main dialog class for ODIN
//
///////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include <atldlgs.h>
#include <Dbt.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <sstream>
//#include <atlmisc.h>
#include "ODINDlg.h"
#include "DriveList.h"
#include "aboutdlg.h"
#include "InternalException.h"
#include "WriteThread.h"
#include "ReadThread.h"
#include "CompressionThread.h"
#include "DecompressionThread.h"
#include "CompressedRunLengthStream.h"
#include "OSException.h"
#include "ImageStream.h"
#include "OptionsDlg.h"

using namespace std;

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

// section name in .ini file for configuration values
IMPL_SECTION(CODINDlg, L"MainWindow")

/////////////////////////////////////////////////////////////////////////////
//
// class CODINSplitManagerCallback
//

void CODINSplitManagerCallback::GetFileName(unsigned fileNo, std::wstring& fileName)
{
  wchar_t buf[10];
  wsprintf(buf, L"%04u", fileNo);

  int lastDotPos = fileName.rfind(L'.');
  if (lastDotPos < 0)
    fileName += buf;
  else {
    wstring ext = fileName.substr(lastDotPos); // get extension
    fileName = fileName.substr(0, lastDotPos);
    fileName += buf;
    fileName += ext;
  }
}

int CODINSplitManagerCallback::AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, wstring& newName)
{
  WTL::CString msg;
  WTL::CString filter;
  int res;
  wchar_t* buffer;
  msg.FormatMessage(IDS_PROVIDE_FILENAME, fileNo);
  filter.LoadString(IDS_FILTERSTRING);

  const int bufsize = filter.GetLength()+7;
  buffer = new wchar_t[bufsize];
  wcsncpy_s(buffer, bufsize, filter, filter.GetLength()+1);
  memcpy(buffer+filter.GetLength()+1, L"\0*.*\0\0", 12);

  CFileDialog fileDlg ( true, NULL, missingFileName, OFN_FILEMUSTEXIST,
            buffer, fhWnd );
 
  if ( res = fileDlg.DoModal() ) {
    if (res == IDOK) {
      newName = fileDlg.m_szFileName;
    }
  }
  delete buffer;
  return res;
}


/////////////////////////////////////////////////////////////////////////////
//
// class CODINDlg
//

CODINDlg::CODINDlg()
: fSourceSize(0), fWndPosX(L"XPos", -1000000), fWndPosY(L"YPos", -1000000),
  fLastOperationWasBackup(L"LastOperationWasBackup", true),
  fLastDriveIndex(L"LastDriveIndex", 1),
  fLastImageFile(L"LastImageFile", L""),
  fColumn0Width(L"VolumeColumn0Width", 30),
  fColumn1Width(L"VolumeColumn1Width", 220),
  fColumn2Width(L"VolumeColumn2Width", 90),
  fColumn3Width(L"VolumeColumn3Width", 80),
  fSplitCB(m_hWnd),
  fVerifyRun(false),
  fTimer(0)
{
  fMode = fLastOperationWasBackup ? modeBackup : modeRestore;
}

CODINDlg::~CODINDlg()
{
  fLastOperationWasBackup = fMode == modeBackup;
}

void CODINDlg::Init() 
{
  fSourceSize = 0;
  fMaxTargetSize = 0;
  fBytesProcessed = 0;
  fSpeed = 0;
  fVerifyRun = false;
}

void CODINDlg::InitControls()
{
  //CContainedWindowT<CButton> saveButton, restoreButton;
  CButton saveButton( GetDlgItem(IDC_RADIO_BACKUP) );
  CButton restoreButton( GetDlgItem(IDC_RADIO_RESTORE) );

  if (fLastImageFile().length() > 0)
      fFiles.push_back(fLastImageFile().c_str());

  if (fMode == modeBackup) {
    saveButton.SetCheck(BST_CHECKED);
    restoreButton.SetCheck(BST_UNCHECKED);
  } else {
    saveButton.SetCheck(BST_UNCHECKED);
    restoreButton.SetCheck(BST_CHECKED);
  }

  // set list view style to enable row selection
  // DWORD exStyle = fVolumeList.GetExtendedListViewStyle();
  fVolumeList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

  // Fill controls with data
  // fill file list box
	CComboBox in(GetDlgItem(IDC_COMBO_FILES));
  WTL::CString strBrowse;
  strBrowse.LoadString(IDS_BROWSE);

  int count = 0;
	in.ResetContent();
	in.AddString(strBrowse);
	list<wstring>::iterator it;

	it = fFiles.begin();
	while(it != fFiles.end())
	{
		in.AddString(it->c_str());
		it++;
    ++count;
	}
  in.SetCurSel(count);

  // init progress bar to percent range
  CProgressBarCtrl progress = GetDlgItem(IDC_PROGRESS_PERCENT);
  progress.SetRange32(0, 100);

  // fill volume list box
  FillDriveList();

  // get template for additional volume info in text box:
  fVolumeInfoTemplate.LoadString(IDS_VOLUME_INFO);
  
  UpdateFileInfoBoxAndResetProgressControls();
  
  WTL::CString dlgTitle;
  dlgTitle.LoadString(IDS_DIALOGTITLE);
  SetWindowText(dlgTitle);
}

// fill the control with the list view with all available drives 
void CODINDlg::FillDriveList()
{
  list<wstring>::iterator it;
  int count = 0;
  const size_t BUFSIZE=80;
  wchar_t buffer[BUFSIZE];
  WTL::CString driveTypeString;

	fVolumeList.DeleteAllItems();
	
  count = fOdinManager.GetDriveCount();
  for (int i=0; i<count; i++) {
    CDriveInfo* di = fOdinManager.GetDriveInfo(i);
    fVolumeList.InsertItem(i, di->GetMountPoint().c_str());
    fVolumeList.SetItemText(i, 1, di->GetDisplayName().c_str());
    MakeByteLabel(di->GetBytes(), buffer, BUFSIZE);
    fVolumeList.SetItemText(i, 2, buffer);
    GetDriveTypeString(di->GetDriveType(), driveTypeString);
    fVolumeList.SetItemText(i, 3, driveTypeString);
    int imageIndex = di->GetMountPoint().length() > 0 ? GetImageIndexAndUpdateImageList(di->GetMountPoint().c_str()) : -1;
    fVolumeList.SetItem(i, 0, LVIF_IMAGE, NULL, imageIndex, 0, 0, 0);
  }
}

void CODINDlg::RefreshDriveList()
{
  fOdinManager.RefreshDriveList();
  fOdinManager.GetDriveNameList(fDriveNames);
	InitControls();
}

void CODINDlg::MakeByteLabel(unsigned __int64 byteCount, LPWSTR buffer, size_t bufsize)
{
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

void CODINDlg::GetPartitionFileSystemString(int partType, WTL::CString& fsString)
{
  switch (partType) {
    case PARTITION_FAT_12:
      fsString.LoadString(IDS_PARTITION_FAT12);
      break;
    case PARTITION_FAT_16:
      fsString.LoadString(IDS_PARTITION_FAT16);
      break;
    case PARTITION_FAT32:
    case PARTITION_FAT32_XINT13:
      fsString.LoadString(IDS_PARTITION_FAT32);
      break;
    case PARTITION_IFS:
      fsString.LoadString(IDS_PARTITION_NTFS);
      break;
    default:
      fsString.LoadString(IDS_PARTITION_UNKNOWN);
  }
}

void CODINDlg::GetDriveTypeString(TDeviceType driveType, WTL::CString& driveTypeString)
{
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
}

int CODINDlg::GetImageIndexAndUpdateImageList (LPCWSTR drive)
{
  SHFILEINFO sfi = {0};
  HIMAGELIST ilShell;

	ilShell = (HIMAGELIST)SHGetFileInfo( drive, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
	if (!ilShell)
		return -1;

  if (fVolumeList.GetImageList(TVSIL_NORMAL).m_hImageList != ilShell)
	{
		fVolumeList.SetImageList(ilShell, LVSIL_SMALL);
	}

  return sfi.iIcon;
}


void CODINDlg::BrowseFiles(WORD wID)
{
	CComboBox combo(GetDlgItem(wID)); 
	int index = combo.GetCurSel();
  SaveComment();

	if (index == 0)
	{
		LPCTSTR sSelectedFile;
    WTL::CString fileDescr, defaultExt;
    defaultExt.LoadString(IDS_DEFAULTEXT);
    fileDescr.LoadString(IDS_FILEDESCR);
    // replace separator chars from resource string
    int len = fileDescr.GetLength();
    for (int i=0; i<len; i++)
      if (fileDescr[i] == L'|')
        fileDescr.SetAt(i, L'\0');
    // open file dialog
		CFileDialog fileDlg ( fMode == modeRestore, defaultExt, NULL,
      fMode == modeRestore ? OFN_FILEMUSTEXIST|OFN_HIDEREADONLY : OFN_HIDEREADONLY, fileDescr);
		 
	  if ( IDOK == fileDlg.DoModal() )
	  {
		  sSelectedFile = fileDlg.m_szFileName;
		  index = combo.FindString(0, sSelectedFile);
		  if (CB_ERR == index)
		  {
			  index = combo.AddString(sSelectedFile);
			  fFiles.push_back(sSelectedFile);

		  }
		  combo.SetCurSel(index);
      UpdateFileInfoBoxAndResetProgressControls();
	  }
	}
}


bool CODINDlg::CheckUniqueFileName(LPCTSTR path, bool useConfirmMessage)
{
	wstring filePattern(path);
  wstring dir;
  WIN32_FIND_DATA res;
	HANDLE h;
	list<wstring> files;
  list <wstring>::iterator itBegin, itEnd;
  int off=0, count = 0, msgRes;
  bool success = true;

  fSplitCB.GetFileName(0, filePattern);
  off = filePattern.rfind(L'\\');
  if (off != string::npos)
    dir = filePattern.substr(0, off+1);
  off=0;
  // count number of '0'
  off = filePattern.find(L'0', 0);
  while (filePattern.find(L'0', off+count) != string::npos)
    ++count;
  filePattern = filePattern.replace(off, count, count, L'?');

	count = 0;
  h = FindFirstFile(filePattern.c_str(), &res);
	if (h == INVALID_HANDLE_VALUE) 
    return true;
	else 
	{
    files.push_back(res.cFileName);
		++count;
		while ( FindNextFile(h, &res) )
		{
      files.push_back(res.cFileName);
		  ++count;
		}
		FindClose(h);

		if (count > 0)
		{
      if (useConfirmMessage) {
        wostringstream msg;
        WTL::CString prefix, postfix;
                   
				prefix.Format(IDS_ASK_DELETE_FILES, dir.c_str(), count);
        postfix.LoadString(IDS_ASK_CONTINUE);

		    msg << (LPCWSTR)prefix << endl;
		    int displayCount = min(10, count);
		    itBegin = files.begin();
		    for (int i=0; i<displayCount; i++)
		    {
			    msg << *itBegin++ << endl;
		    }
		    if (count > displayCount)
			    msg << _T("...") << endl;
		    msg << (LPCWSTR)postfix << endl;
  		  msgRes = AtlMessageBox(m_hWnd, msg.str().c_str(), IDS_WARNING, MB_ICONEXCLAMATION | MB_OKCANCEL);
      } else {
        msgRes = IDOK;
      }

		  if (msgRes == IDOK)
		  {
			  // delete files in list
			  for (itBegin = files.begin(), itEnd = files.end(); itBegin != itEnd && success; itBegin++)
			  {
				  wstring file = dir + (*itBegin).c_str();
				  success = DeleteFile(file.c_str()) != FALSE;
				  if (!success)
				  {
            WTL::CString msg;
					  msg.Format(IDS_CANNOTDELETEFILE, file.c_str());
					  AtlMessageBox(m_hWnd, (LPCWSTR)msg, IDS_ERROR, MB_ICONEXCLAMATION | MB_OKCANCEL);
					  break;
				  }
			  }
		  }
		  else
			  success = false;
		}
	}    
	return success;
}

void CODINDlg::WaitUntilDone()
{
  // wait until threads are completed without blocking the user interface
  unsigned threadCount = fOdinManager.GetThreadCount();
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  bool ok = fOdinManager.GetThreadHandles(threadHandleArray, threadCount);
  if (!ok)
    return;
  ATLTRACE("Wait until done entered.\n");

  while (TRUE) {
    DWORD result = MsgWaitForMultipleObjects(threadCount, threadHandleArray, FALSE, INFINITE, QS_ALLEVENTS);
    if (result >= WAIT_OBJECT_0 && result < (DWORD)threadCount) {
      ATLTRACE("event arrived: %d, thread id: %x\n", result, threadHandleArray[result]);
      if (--threadCount == 0)  {
        fBytesProcessed = fOdinManager.GetBytesProcessed();
        UpdateStatus(false);
        ATLTRACE(" All worker threads are terminated now\n");
        //ATLTRACE(" Total Bytes written: %lu\n", m_allThreads[1]->GetBytesWritten());
        //ATLTRACE(" Total Bytes read: %lu\n", m_allThreads[0]->GetBytesRead());
        bool wasVerifyRun = fVerifyRun; // will be deleted in DeleteProcessingInfo()!
        DeleteProcessingInfo(); // work is finished
        if (wasVerifyRun) {
          CheckVerifyResult();
          fVerifyRun = false;
        }
        break;
      }
      // setup new array with the remaining threads:
      for (unsigned i=result; i<threadCount; i++)
        threadHandleArray[i] = threadHandleArray[i+1];
    }
    else if (result  == WAIT_OBJECT_0 + threadCount)
    {
      // ATLTRACE("windows msg arrived\n");
      // process windows messages
      MSG msg ;
      while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        ::DispatchMessage(&msg) ;
    }
    else if ( WAIT_FAILED) {
      ATLTRACE("MsgWaitForMultipleObjects failed, last error is: %d\n", GetLastError());
      break;
    }
    else
      ATLTRACE("unusual return code from MsgWaitForMultipleObjects: %d\n", result);
  }
  delete [] threadHandleArray;
  ATLTRACE("Wait until done exited.\n");
}

void CODINDlg::DeleteProcessingInfo()
{
  wstring msgCopy;
  LPCWSTR msg = fOdinManager.GetErrorMessage();
   
  KillTimer(fTimer);
  fTimer = 0;

  // we have an error message copy it before thread gets destroyed and then display it.
  if (msg != NULL)
    msgCopy = msg;

  fOdinManager.Terminate();

  if (msg != NULL)
    	AtlMessageBox(m_hWnd, msgCopy.c_str(), IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);

  Init();

  if (::IsWindow(m_hWnd)) {   // may be destroyed if user pressed Cancel button
    EnableControlsAfterProcessingComplete();
  }
}

bool CODINDlg::CheckThreadsForErrors()
{
  return fOdinManager.WasError();
}

void CODINDlg::UpdateStatus(bool bInit)
{
  static LARGE_INTEGER startTimer, performanceFrequency;
  static __int64 nnLastTotal;
  LARGE_INTEGER timer;
  int nProgressPercent;
  CStatic label;
  __int64 ddTimeDifference;
  unsigned __int64 nnBytesTotal = fOdinManager.GetTotalBytesToProcess();
  unsigned __int64 nnBytesLeft = nnBytesTotal - fBytesProcessed;

  if (bInit) {
    QueryPerformanceFrequency(&performanceFrequency);
    QueryPerformanceCounter(&startTimer);
    nnLastTotal = 0;
  } else if (nnBytesTotal ) {
    nProgressPercent = (int) ((fBytesProcessed * 100 + (nnBytesTotal/2)) / nnBytesTotal);

    const size_t BUFSIZE=80;
    wchar_t buffer[BUFSIZE];

    swprintf(buffer, BUFSIZE, L"%d%%", nProgressPercent);
    label = GetDlgItem(IDC_LABEL_PERCENT);
    label.SetWindowText(buffer);
    
    CProgressBarCtrl progress = GetDlgItem(IDC_PROGRESS_PERCENT);
    progress.SetPos(nProgressPercent);

    MakeByteLabel(nnBytesTotal, buffer, BUFSIZE);
    label = GetDlgItem(IDC_LABEL_BYTES_TOTAL);
    label.SetWindowText(buffer);
    MakeByteLabel(fBytesProcessed, buffer, BUFSIZE);
    label = GetDlgItem(IDC_LABEL_BYTES_PROCESSED);
    label.SetWindowText(buffer);

    QueryPerformanceCounter(&timer);
    
    // get elapsed time
    ddTimeDifference = timer.QuadPart - startTimer.QuadPart;
    // convert to seconds
    ddTimeDifference = (ddTimeDifference + (performanceFrequency.QuadPart>>1) )/ performanceFrequency.QuadPart;
    WTL::CString timeLabel;
    DWORD dwTimeDifference = (DWORD) ddTimeDifference;
    timeLabel.Format(L"%02u:%02u:%02u", dwTimeDifference/3600, dwTimeDifference % 3600 / 60, dwTimeDifference % 60);
    label = GetDlgItem(IDC_LABEL_TIME_ELAPSED);
    label.SetWindowText(timeLabel);

    // calculate speed
    if (ddTimeDifference>0)
      MakeByteLabel(fBytesProcessed/ddTimeDifference, buffer, BUFSIZE);
    WTL::CString labelSpeed (buffer);
    labelSpeed += L"/s";
    label = GetDlgItem(IDC_LABEL_SPEED);
    label.SetWindowText(labelSpeed);

    // calculate (estimate) time left
    if (fBytesProcessed > 0) {
      __int64 totalTime = (ddTimeDifference * nnBytesTotal + (fBytesProcessed>>1)) / fBytesProcessed;
      __int64 timeLeft = totalTime - ddTimeDifference;
      DWORD dwTimeLeft = (DWORD) timeLeft;
      timeLabel.Format(L"%02u:%02u:%02u", dwTimeLeft/3600, dwTimeLeft % 3600 / 60, dwTimeLeft % 60);

      label = GetDlgItem(IDC_LABEL_TIME_LEFT);
      label.SetWindowText(timeLabel);
    }
  }
} 

void CODINDlg::ResetProgressControls()
{
  CStatic label;
  CProgressBarCtrl progress = GetDlgItem(IDC_PROGRESS_PERCENT);
  progress.SetPos(0);
  label = GetDlgItem(IDC_LABEL_PERCENT);
  label.SetWindowText(NULL);
  label = GetDlgItem(IDC_LABEL_BYTES_TOTAL);
  label.SetWindowText(NULL);
  label = GetDlgItem(IDC_LABEL_BYTES_PROCESSED);
  label.SetWindowText(NULL);
  label = GetDlgItem(IDC_LABEL_TIME_ELAPSED);
  label.SetWindowText(NULL);
  label = GetDlgItem(IDC_LABEL_TIME_LEFT);
  label.SetWindowText(NULL);
  label = GetDlgItem(IDC_LABEL_SPEED);
  label.SetWindowText(NULL);
}

__int64 CODINDlg::GetFileSizeByName(LPCWSTR fileName)
{
  // OFSTRUCT of;
	__int64 nnSize = 0;
	LARGE_INTEGER size;
//  HANDLE h = OpenFile(fileName, &of, OF_READ | OF_SHARE_DENY_WRITE);
  HANDLE h = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
  if (h != INVALID_HANDLE_VALUE)
  {
    ::GetFileSizeEx(h, &size);
	  CloseHandle(h);
  }
 
  return (__int64) nnSize;
}

bool CODINDlg::IsFileReadable(LPCWSTR fileName)
{
  HANDLE h = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
  if (h != INVALID_HANDLE_VALUE)
  {
	  CloseHandle(h);
  }
 
  return h != INVALID_HANDLE_VALUE;
}

bool CODINDlg::IsFileWritable(LPCWSTR fileName)
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

void CODINDlg::GetDirFromFileName(const wstring& fileName, wstring& dir)
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
  int pos = absPath.rfind(L'\\');
  if (pos == string::npos) {
      ATLTRACE("Error in getting current directory for abs path %S", absPath);
      dir.clear();
  } else {
    dir = absPath.substr(0, pos);
  }
}

void CODINDlg::GetDriveFromFileName(const wstring& fileName, wstring& drive)
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
  int pos = absPath.find(L'\\');
  if (pos == string::npos) {
      ATLTRACE("Error in getting current directory for abs path %S", absPath);
      drive.clear();
  } else {
    drive = absPath.substr(0, pos+1);
  }
}

void CODINDlg::GetFreeBytesOfDisk(LPCWSTR dir, unsigned __int64* freeBytesAvailable, unsigned __int64* totalNumberOfBytes)
{
  ULARGE_INTEGER  freeBytesAvailable2, totalNumberOfFreeBytes, totalNumberOfBytes2;
  BOOL ok = GetDiskFreeSpaceEx(dir, &freeBytesAvailable2, &totalNumberOfBytes2, &totalNumberOfFreeBytes);
  CHECK_OS_EX_PARAM1(ok, EWinException::generalFileError, dir);
  if (ok) {
    *freeBytesAvailable = freeBytesAvailable2.QuadPart;
    *totalNumberOfBytes = totalNumberOfBytes2.QuadPart;
  }
}

int CODINDlg::CheckConditionsForSavePartition(const wstring& fileName, int index)
{
  wstring targetDrive, dir;
  int res = IDOK;
  unsigned __int64 freeBytesAvailable, totalNumberOfBytes;
  const size_t cBufferSize = 64;
  wchar_t buffer[cBufferSize];
  WTL::CString msgStr;

  if (index < 0) {
    AtlMessageBox(m_hWnd, IDS_VOLUME_NOSEL, IDS_WARNING, MB_OK);
    return IDCANCEL;
  }

  // check if target file already exists and warn that it will be overwritten
  if (IsFileReadable(fileName.c_str())) {
    msgStr.Format(IDS_FILE_EXISTS, fileName.c_str());
    res = AtlMessageBox(m_hWnd, (LPCWSTR)msgStr, IDS_WARNING, MB_OKCANCEL);
    if (res != IDOK)
      return res;
  }

  // check if target file can be written to
  if (!IsFileWritable(fileName.c_str())) {
    msgStr.Format(IDS_CANTWRITEFILE, fileName.c_str());
    AtlMessageBox(m_hWnd, (LPCWSTR)msgStr, IDS_ERROR, MB_OK);
    return IDCANCEL;
  }

  // check available disk space
  GetDirFromFileName(fileName, dir);
  ATLTRACE("Getting dir from file : %S\n", dir.c_str());
  GetFreeBytesOfDisk(dir.c_str(), &freeBytesAvailable, &totalNumberOfBytes);
  MakeByteLabel(freeBytesAvailable, buffer, cBufferSize);
  ATLTRACE("Number of bytes available on disk (for this user): %S\n", buffer);
  MakeByteLabel(totalNumberOfBytes, buffer, cBufferSize);
  ATLTRACE("Total number of bytes on disk : %S\n", buffer);

  unsigned __int64 bytesToSave = fOdinManager.GetDriveInfo(index)->GetUsedSize();
  if (bytesToSave == 0)
    fOdinManager.GetDriveInfo(index)->GetBytes();


  if (freeBytesAvailable < bytesToSave) {
    res = AtlMessageBox(m_hWnd, IDS_NOTENOUGHSPACE, IDS_ERROR, MB_ICONEXCLAMATION | MB_YESNO);
    if (res != IDYES)
      return res;
  } else
    res = IDOK;

  // check if file size of image possibly exceeds 4GB limit on FAT32:
  GetDriveFromFileName(fileName, targetDrive);
  int targetIndex = fOdinManager.GetDriveList()->GetIndexOfDrive(targetDrive.c_str());
  bool isFAT = targetIndex >= 0 && 
              (fOdinManager.GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_FAT32 ||
               fOdinManager.GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_FAT32_XINT13 ||
               fOdinManager.GetDriveInfo(targetIndex)->GetPartitionType() == PARTITION_FAT_16);
  if (isFAT && bytesToSave > (2i64<<32)-1 && fOdinManager.GetSplitSize() == 0) {
    res = AtlMessageBox(m_hWnd, IDS_4GBLIMITEXCEEDED, IDS_WARNING, MB_ICONEXCLAMATION | MB_YESNO);
    if (res != IDYES)
      return res;
    else
      res = IDOK;
  }

  // warning: do not youse same drive for source and destination
  wstring sourceDrive = fOdinManager.GetDriveInfo(index)->GetMountPoint();
  if (sourceDrive.compare(targetDrive) == 0) {
    res = AtlMessageBox(m_hWnd, IDS_NOTSAMEDRIVE, IDS_ERROR, MB_ICONEXCLAMATION | MB_OKCANCEL);
    if (res != IDOK)
      return res;
  }

  // warning: do not backup windows partition
  wstring sysDir;
  wchar_t systemDir[MAX_PATH];
  int count = GetSystemDirectory(systemDir, MAX_PATH);
  sysDir = systemDir;
  GetDriveFromFileName(sysDir, targetDrive);
  if (sourceDrive.compare(targetDrive) == 0) {
    res = AtlMessageBox(m_hWnd, IDS_NOWINDIRBACKUP, IDS_ERROR, MB_ICONEXCLAMATION | MB_OKCANCEL);
  }

  // internal implementation limitation: file size of split size must be bigger than fReadBlockSize
  if (fOdinManager.GetSplitSize() > 0 && fOdinManager.GetSplitSize() < fOdinManager.GetReadBlockSize()) {
    const int BUFSIZE=80;
    wchar_t buffer[BUFSIZE];
    WTL::CString msgStr;
    MakeByteLabel( fOdinManager.GetReadBlockSize(), buffer, BUFSIZE);
    msgStr.Format(IDS_SPLITSIZETOOSMALL, buffer);
    res = AtlMessageBox(m_hWnd, (LPCWSTR)msgStr, IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);
    res = IDCANCEL;
  }

  return res;
}

int CODINDlg::CheckConditionsForRestorePartition(const wstring& fileName, const std::wstring splitFileName, int index, 
     unsigned& noFiles, unsigned __int64& totalSize)
{
  unsigned __int64 targetPartitionSize, partitionSizeToSave;
  CFileImageStream fileStream;
  int res;
  int volType;
  WTL::CString msgStr;
  wstring openName = fileName;

  noFiles = 0;
  totalSize = 0;
  if (index < 0) {
    AtlMessageBox(m_hWnd, IDS_VOLUME_NOSEL, IDS_ERROR, MB_OK);
    return IDCANCEL;
  }

  res = CheckConditionsForVerifyPartition(fileName, splitFileName, noFiles, totalSize, volType, &partitionSizeToSave);
  if (res==IDOK) {
      
    // prevent restoring a hard disk to a partition or a partition to a hard disk
    if ((fOdinManager.GetDriveInfo(index)->IsCompleteHardDisk() && volType != CImageFileHeader::volumeHardDisk) ||
        (!fOdinManager.GetDriveInfo(index)->IsCompleteHardDisk() && volType != CImageFileHeader::volumePartition)) {
      AtlMessageBox(m_hWnd, IDS_WRONGPARTITIONTYPE, IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);
      res = IDCANCEL;
      return res;
    }

    targetPartitionSize = fOdinManager.GetDriveInfo(index)->GetBytes();
    if (targetPartitionSize < partitionSizeToSave) {
      AtlMessageBox(m_hWnd, IDS_IMAGETOOBIG, IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);
      res = IDCANCEL;
    }
    else if (targetPartitionSize > partitionSizeToSave) {
      res = AtlMessageBox(m_hWnd, IDS_IMAGETOOSMALL, IDS_WARNING, MB_ICONEXCLAMATION | MB_YESNO);
    }
    else
      res = IDOK;
  }

  if (res == IDOK) {
    WTL::CString msg;
    msg.Format(IDS_ERASE_DRIVE, fOdinManager.GetDriveInfo(index)->GetMountPoint().c_str());
    res = AtlMessageBox(m_hWnd, (LPCWSTR)msg, IDS_WARNING, MB_ICONEXCLAMATION | MB_OKCANCEL);
    if (res != IDOK)
      return res;
  }
  

  // warning: do not restore to windows partition
  wstring sysDir, sysDrive;
  wchar_t systemDir[MAX_PATH];
  int count = GetSystemDirectory(systemDir, MAX_PATH);
  sysDir = systemDir;
  GetDriveFromFileName(sysDir, sysDrive);
  wstring targetDrive = fOdinManager.GetDriveInfo(index)->GetMountPoint();
  if (sysDrive.compare(targetDrive) == 0) {
    res = AtlMessageBox(m_hWnd, IDS_NORESTORETOWINDOWS, IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);
    res = IDCANCEL;
  }

  // warn user to reboot system if a complete disk is restored
  if (fOdinManager.GetDriveInfo(index)->IsCompleteHardDisk()) {
    res = AtlMessageBox(m_hWnd, IDS_REBOOTAFTERRESTOREDISK, IDS_WARNING, MB_ICONEXCLAMATION | MB_OK);
  }

  return res;
}

int CODINDlg::CheckConditionsForVerifyPartition(const std::wstring& fileName, const std::wstring splitFileName, 
  unsigned& noFiles, unsigned __int64& totalSize, int& volType, unsigned __int64* partitionSizeToSave)
{
  CFileImageStream fileStream;
  int res=IDOK;
  WTL::CString msgStr;
  wstring openName = fileName;

  noFiles = 0;
  totalSize = 0;

  if (!IsFileReadable(openName.c_str()))
    openName = splitFileName; // try multiple files mode

  if (!IsFileReadable(openName.c_str()))
  {
    msgStr.Format(IDS_CANTREADFILE, fileName.c_str());
    AtlMessageBox(m_hWnd, (LPCWSTR)msgStr, IDS_ERROR, MB_OK);
    return IDCANCEL;
  }

  fileStream.Open(openName.c_str(), IImageStream::forReading);
  fileStream.ReadImageFileHeader(false);
  if (!fileStream.GetImageFileHeader().IsValidFileHeader())
  {
    AtlMessageBox(m_hWnd, IDS_WRONGFILEFORMAT, IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);
    res = IDCANCEL;
  }
  if (partitionSizeToSave)
    *partitionSizeToSave = fileStream.GetImageFileHeader().GetVolumeSize();
  noFiles = fileStream.GetImageFileHeader().GetFileCount();
  totalSize = fileStream.GetImageFileHeader().GetFileSize();
  fCrc32FromFileHeader = fileStream.GetCrc32Checksum();
  volType = fileStream.GetImageFileHeader().GetVolumeType();

  return res;
}

void CODINDlg::UpdateFileInfoBoxAndResetProgressControls()
{
  UpdateFileInfoBox();
  ResetProgressControls();
}

void CODINDlg::UpdateFileInfoBox()
{
  if (fMode == modeRestore)
    ReadImageFileInformation();
  else
    EnterCommentModeForEditBox();
}

void CODINDlg::EnterCommentModeForEditBox()
{
  CButton verifyButton(GetDlgItem(ID_BT_VERIFY));
  CEdit commentTextField(GetDlgItem(IDC_EDIT_FILE));
  commentTextField.SetReadOnly(FALSE);
  verifyButton.EnableWindow(FALSE);
  if (fComment.length() == 0) {
    WTL::CString hint;
    hint.LoadString(IDS_ENTERCOMMENT);
    commentTextField.SetWindowText(hint);
  } else
    commentTextField.SetWindowText(fComment.c_str());
    commentTextField.SetSelAll();
}

void CODINDlg::SaveComment()
{
  CEdit commentTextField(GetDlgItem(IDC_EDIT_FILE));
  // save comment if there is one
  WTL::CString hint;
  hint.LoadString(IDS_ENTERCOMMENT);
  ReadWindowText(commentTextField, fComment);
  if (fComment.compare(fComment) == 0)
    fComment.empty();
}

void CODINDlg::ReadImageFileInformation()
{
  // get information from file header and update dialog to show this information
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));
  // CStatic volumeInfoTextField(GetDlgItem(IDC_TEXT_FILE));
  CEdit volumeInfoTextField(GetDlgItem(IDC_EDIT_FILE));
  WTL::CString text;
  CFileImageStream imageStream;
  wstring fileName;
  CButton verifyButton(GetDlgItem(ID_BT_VERIFY));

  ReadWindowText(comboFiles, fileName);
  try {
    imageStream.Open(fileName.c_str(), IImageStream::forReading);
    const size_t BUFSIZE=80;
    wchar_t buffer[BUFSIZE];
    wchar_t creationDateStr[BUFSIZE];
    wchar_t creationTimeStr[BUFSIZE];
    imageStream.ReadImageFileHeader(false);
    // get comment
    LPCWSTR comment = imageStream.GetComment();

    // get get size of contained volume
    const CImageFileHeader& header = imageStream.GetImageFileHeader();
    unsigned __int64 size = header.GetVolumeSize();
    MakeByteLabel(size, buffer, BUFSIZE);

    // get creation date
    CFindFile finder;
    if ( finder.FindFile (fileName.c_str()))
    {
      FILETIME creationDate;
	    SYSTEMTIME stUTC, stLocal;

      BOOL ok = finder.GetCreationTime(&creationDate);
      if (ok) {
	      // Convert the last-write time to local time.
        FileTimeToSystemTime(&creationDate, &stUTC);
        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stLocal, NULL,  creationDateStr, BUFSIZE);
        GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stLocal, NULL,  creationTimeStr, BUFSIZE);
      }
    }
    finder.Close();
    text.Format(IDS_HEADERTEMPLATE, creationDateStr, creationTimeStr, buffer, comment);
    verifyButton.EnableWindow(TRUE);
  } catch (EWinException& e) {
    if (e.GetErrorCode() == EWinException::fileOpenError) {
      text.Format(IDS_CANNOTOPENFILE, fileName.c_str());
      verifyButton.EnableWindow(FALSE);
    } 
  } catch (Exception& e) {
    verifyButton.EnableWindow(FALSE);
    wstring msg = e.GetMessage();
    ATLTRACE("can not open file: %S, error: %S", fileName.c_str(), msg.c_str());  
  }

  volumeInfoTextField.SetReadOnly(TRUE);
  volumeInfoTextField.SetWindowText(text);

  imageStream.Close();
}

void CODINDlg::CheckVerifyResult()
{
  DWORD verifyCrc32 = fOdinManager.GetVerifiedChecksum();
  if (fCrc32FromFileHeader == verifyCrc32)
    AtlMessageBox(this->m_hWnd,IDS_VERIFY_OK, IDS_VERIFY_TITLE, MB_OK);
  else
    AtlMessageBox(this->m_hWnd,IDS_VERIFY_FAILED, IDS_VERIFY_TITLE, MB_OK|MB_ICONEXCLAMATION);
}

void CODINDlg::DisableControlsWhileProcessing()
{
  CButton saveButton( GetDlgItem(IDC_RADIO_BACKUP) );
  CButton restoreButton( GetDlgItem(IDC_RADIO_RESTORE) );
  CButton okButton(GetDlgItem(IDOK));
  CButton verifyButton(GetDlgItem(ID_BT_VERIFY));
  CButton optionsButton(GetDlgItem(ID_BT_OPTIONS));
  CButton cancelButton(GetDlgItem(IDCANCEL));
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));
  CListViewCtrl listBoxVolumes(GetDlgItem(IDC_LIST_VOLUMES));
  CEdit commentTextField(GetDlgItem(IDC_EDIT_FILE));
  WTL::CString text;

  saveButton.EnableWindow(FALSE);
  restoreButton.EnableWindow(FALSE);
  okButton.EnableWindow(FALSE);
  verifyButton.EnableWindow(FALSE);
  optionsButton.EnableWindow(FALSE);
  comboFiles.EnableWindow(FALSE);
  listBoxVolumes.EnableWindow(FALSE);
  commentTextField.EnableWindow(FALSE);

  text.LoadString(IDS_CANCEL);
  cancelButton.SetWindowText(text);
}

void CODINDlg::EnableControlsAfterProcessingComplete()
{
  CButton saveButton( GetDlgItem(IDC_RADIO_BACKUP) );
  CButton restoreButton( GetDlgItem(IDC_RADIO_RESTORE) );
  CButton okButton(GetDlgItem(IDOK));
  CButton verifyButton(GetDlgItem(ID_BT_VERIFY));
  CButton optionsButton(GetDlgItem(ID_BT_OPTIONS));
  CButton cancelButton(GetDlgItem(IDCANCEL));
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));
  CListViewCtrl listBoxVolumes(GetDlgItem(IDC_LIST_VOLUMES));
  CEdit commentTextField(GetDlgItem(IDC_EDIT_FILE));
  WTL::CString text;

  saveButton.EnableWindow(TRUE);
  restoreButton.EnableWindow(TRUE);
  okButton.EnableWindow(TRUE);
  verifyButton.EnableWindow(TRUE);
  optionsButton.EnableWindow(TRUE);
  comboFiles.EnableWindow(TRUE);
  listBoxVolumes.EnableWindow(TRUE);
  commentTextField.EnableWindow(TRUE);
  text.LoadString(IDS_EXIT);
  cancelButton.SetWindowText(text);
}

void CODINDlg::CleanupPartiallyWrittenFiles()
{
  wstring fileName;
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));

  ReadWindowText(comboFiles, fileName);
  if (fOdinManager.GetSplitSize() > 0) {
    CheckUniqueFileName(fileName.c_str(), false);
  } else {
    DeleteFile(fileName.c_str());
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// message handler methods

LRESULT CODINDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
  POINT ptWin = {fWndPosX, fWndPosY };
  WTL::CString drive, name, size, type;
  // DlgResize_Init();
  
  if (MonitorFromPoint(ptWin, MONITOR_DEFAULTTONULL) == NULL) {
    // center the dialog on the screen
	  CenterWindow();
  }
  else { // point is valid
    SetWindowPos(NULL, fWndPosX, fWndPosY, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER|SWP_NOSIZE);
  }

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_ODIN_MAIN), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_ODIN_MAIN), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

  drive.LoadStringW(IDS_DRIVE);
  name.LoadStringW(IDS_NAME);
  size.LoadStringW(IDS_SIZE);
  type.LoadStringW(IDS_TYPE);
  fVolumeList = GetDlgItem(IDC_LIST_VOLUMES);
  fVolumeList.AddColumn(drive, 0);
  fVolumeList.AddColumn(name, 1);
  fVolumeList.AddColumn(size, 2);
  fVolumeList.AddColumn(type, 3);
  fVolumeList.SetColumnWidth(0, fColumn0Width);
  fVolumeList.SetColumnWidth(1, fColumn1Width);
  fVolumeList.SetColumnWidth(2, fColumn2Width);
  fVolumeList.SetColumnWidth(3, fColumn3Width);

  RefreshDriveList();
  
  CStatic volumeInfoTextField(GetDlgItem(IDC_TEXT_VOLUME));
  WTL::CString text;
  text.LoadString(IDS_VOLUME_NOSEL);
  volumeInfoTextField.SetWindowText(text);

  bHandled = TRUE;
  return 0L;
}

LRESULT CODINDlg::OnDeviceChanged(UINT /*uMsg*/, WPARAM nEventType, LPARAM lParam, BOOL& bHandled)
{
	if (nEventType == DBT_DEVICEARRIVAL || nEventType == DBT_DEVICEREMOVECOMPLETE)
	{
  	DEV_BROADCAST_VOLUME *volume = (DEV_BROADCAST_VOLUME *)lParam;
    if (volume->dbcv_devicetype == DBT_DEVTYP_VOLUME) {
      CStatic textBox;
      WTL::CString msgText;
      EnableWindow(FALSE);
      msgText.LoadStringW(IDS_WAITUPDATEDRIVES);
      CRect rect(130, 150, 350, 210);
      HWND h = textBox.Create(this->m_hWnd, &rect, msgText, WS_VISIBLE| WS_CHILD| WS_BORDER|WS_CLIPCHILDREN|WS_GROUP|WS_TABSTOP|SS_LEFT|SS_SUNKEN, 0);
      textBox.SetWindowPos(this->m_hWnd,0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
      textBox.SetWindowTextW(msgText);
      CWaitCursor waitCursor;
      RefreshDriveList();
      EnableWindow(TRUE);
      textBox.DestroyWindow();
    }
  }

  bHandled = TRUE;
  return 0L;
}

LRESULT CODINDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));
  int index = fVolumeList.GetSelectedIndex();
  wstring fileName;

  bHandled = TRUE;

  ReadWindowText(comboFiles, fileName);

  try {

    UpdateStatus(true);
    fTimer = SetTimer(cTimerId, 1000); 

    if (fMode == modeBackup) {
      bool ok = true;
      // check if save possible:
      int res = CheckConditionsForSavePartition(fileName, index);
      if (res == IDOK || res == IDYES) {
        if (fOdinManager.GetSplitSize() > 0) {
          // check  if there are file names in conflict with files created during backup
          ok = CheckUniqueFileName(fileName.c_str(), true);
        } else if (IsFileReadable(fileName.c_str())) {
          ok = DeleteFile(fileName.c_str()) == TRUE;
          if (!ok) {
            WTL::CString msg;
				    msg.Format(IDS_CANNOTDELETEFILE, fileName.c_str());
				    AtlMessageBox(m_hWnd, (LPCWSTR)msg, IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);
          }
        }

        if (ok) {
          DisableControlsWhileProcessing();
          SaveComment();
          fOdinManager.SetComment(fComment.c_str());
          // save drive to file
          fOdinManager.SavePartition(index, fileName.c_str(), &fSplitCB);
        }
      } else {
        return 0;
      }
	  } else if (fMode == modeRestore) {
      wstring splitFileName;
      unsigned noFiles = 0;
      unsigned __int64 totalSize = 0;
      splitFileName = fileName;
      fSplitCB.GetFileName(0, splitFileName);
      int res = CheckConditionsForRestorePartition(fileName, splitFileName, index, noFiles, totalSize);
      if (res == IDOK || res == IDYES) {
        DisableControlsWhileProcessing();
  		  // restore file to drive
        fOdinManager.RestorePartition(fileName.c_str(), index, noFiles, totalSize, &fSplitCB);
      } else {
        return 0;
      }
	  } else {
      ATLTRACE("Internal Error neither backup nor restore state");
      return 0;
    }

    // now wait until all threads are finished, but do not block the UI.
    WaitUntilDone();
  } catch (Exception& e) {
    wstring msg = e.GetMessage();
    int res = AtlMessageBox(m_hWnd, msg.c_str(), IDS_ERROR, MB_ICONEXCLAMATION | MB_OKCANCEL);
    DeleteProcessingInfo();
  }
  catch (...) {
    // We've encountered an exception sometime before we started the operation, so close down everything
    // as gracefully as we can.
    DeleteProcessingInfo();
    throw;
  }

  fVolumeList.SetFocus();
  return 0;
}

LRESULT CODINDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  if (fOdinManager.IsRunning()) {
    int res = AtlMessageBox(m_hWnd, IDS_CONFIRMCANCEL, IDS_WARNING, MB_ICONEXCLAMATION | MB_YESNO);
    if (res == IDYES) {
      DeleteProcessingInfo();
      ResetProgressControls();
      if (fMode==modeBackup)
        CleanupPartiallyWrittenFiles();
    }
  } else {
    DeleteProcessingInfo();
    BOOL ok  = EndDialog(wID);
    if (!ok)
      ok = GetLastError();
  }

  return 0L;
}


LRESULT CODINDlg::OnBnClickedRadioBackup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
  CButton saveButton( hWndCtl );
  TOperationMode oldMode = fMode;

  if (saveButton.GetCheck())
    fMode = modeBackup;

  if (fMode != oldMode) {
    UpdateFileInfoBoxAndResetProgressControls();
  }
  return 0;
}

LRESULT CODINDlg::OnBnClickedRadioRestore(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
  CButton restoreButton( hWndCtl );
  TOperationMode oldMode = fMode;
  if (restoreButton.GetCheck())
    fMode = modeRestore;
  if (fMode != oldMode) {
    SaveComment();
    UpdateFileInfoBoxAndResetProgressControls();
  }
  return 0;
}

LRESULT CODINDlg::OnCbnSelchangeComboFiles(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  BrowseFiles(wID);

  return 0;
}

LRESULT CODINDlg::OnLvnItemchangedListVolumes(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
  // LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
  const size_t BUFSIZE=80;
  wchar_t buffer[BUFSIZE];
  CStatic volumeInfoTextField(GetDlgItem(IDC_TEXT_VOLUME));

  if (pNMHDR->idFrom == IDC_LIST_VOLUMES && (pNMListView->uChanged & LVIF_STATE) &&
      (pNMListView->uNewState & LVIS_SELECTED)) {
    WTL::CString text;
    int index = fVolumeList.GetSelectedIndex();
    if (index < 0) {
      text.LoadString(IDS_VOLUME_NOSEL);
      volumeInfoTextField.SetWindowText(text);
    } else {
      WTL::CString fsText;
      CDriveInfo* di = fOdinManager.GetDriveInfo(index);
      MakeByteLabel(di->GetUsedSize(), buffer, BUFSIZE);
      GetPartitionFileSystemString(di->GetPartitionType(), fsText);
      text.Format(fVolumeInfoTemplate, di->GetMountPoint().c_str(), buffer, fsText, di->GetClusterSize());
      volumeInfoTextField.SetWindowText(text);
    }
  }

  return 0;
}

LRESULT CODINDlg::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  if (CheckThreadsForErrors()) {
    DeleteProcessingInfo();
    return 0;
  }

  fBytesProcessed = fOdinManager.GetBytesProcessed();
  UpdateStatus(false);

  return 0;
}

LRESULT CODINDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CODINDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  // Get column widths to persits value
  fColumn0Width = fVolumeList.GetColumnWidth(0);
  fColumn1Width = fVolumeList.GetColumnWidth(1);
  fColumn2Width = fVolumeList.GetColumnWidth(2);
  fColumn3Width = fVolumeList.GetColumnWidth(3);
  
  // get name of last image file
  wstring fileName;
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));
  ReadWindowText(comboFiles, fileName);
	int index = comboFiles.GetCurSel();
	if (index > 0) // do not save Browse... label
    fLastImageFile = fileName;
  return 0;
}

LRESULT CODINDlg::OnCbnKillfocusComboFiles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  UpdateFileInfoBoxAndResetProgressControls();

  return 0;
}

LRESULT CODINDlg::OnBnClickedBtVerify(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));
  wstring fileName;
  bHandled = TRUE;

  ReadWindowText(comboFiles, fileName);

  fCrc32FromFileHeader = 0;

  try {
    UpdateStatus(true);
    fTimer = SetTimer(cTimerId, 1000); 

    wstring splitFileName;
    unsigned noFiles = 0;
    unsigned __int64 totalSize = 0;
    int volType;

    splitFileName = fileName;
    fSplitCB.GetFileName(0, splitFileName);
    int res = CheckConditionsForVerifyPartition(fileName, splitFileName, noFiles, totalSize, volType);
    if (res == IDOK) {
      DisableControlsWhileProcessing();
      fVerifyRun = true;
		  // restore file to drive
      fOdinManager.VerifyPartition(fileName.c_str(), noFiles, totalSize, &fSplitCB);
    }
    // now wait until all threads are finished, but do not block the UI.
    WaitUntilDone();
  } catch (Exception& e) {
    wstring msg = e.GetMessage();
    int res = AtlMessageBox(m_hWnd, msg.c_str(), IDS_ERROR, MB_ICONEXCLAMATION | MB_OKCANCEL);
    DeleteProcessingInfo();
  }
  catch (...) {
    // We've encountered an exception sometime before we started the operation, so close down everything
    // as gracefully as we can.
    DeleteProcessingInfo();
    throw;
  }
  return 0;
}

LRESULT CODINDlg::OnBnClickedBtOptions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  COptionsDlg optionsDlg(fOdinManager);
  int res = optionsDlg.DoModal();

  return 0;
}
