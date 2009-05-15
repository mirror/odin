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
#include <algorithm>
//#include <atlmisc.h>
#include "UserFeedback.h"
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
#include "PartitionInfoMgr.h"
#include "OptionsDlg.h"
#include "Util.h"
#include "FileNameUtil.h"
#include "MultiPartitionHandler.h"

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

  size_t lastDotPos = fileName.rfind(L'.');
  if (lastDotPos < 0)
    fileName += buf;
  else {
    wstring ext = fileName.substr(lastDotPos); // get extension
    fileName = fileName.substr(0, lastDotPos);
    fileName += buf;
    fileName += ext;
  }
}

size_t CODINSplitManagerCallback::AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, wstring& newName)
{
  WTL::CString msg;
  WTL::CString filter;
  size_t res;
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
  fColumn4Width(L"VolumeColumn4Width", 80),
  fSplitCB(m_hWnd),
  fVerifyRun(false),
  fTimer(0),
  fWasCancelled(false),
  fFeedback(m_hWnd),
  fChecker(fFeedback, fOdinManager)
{
  fMode = fLastOperationWasBackup ? modeBackup : modeRestore;
  ResetRunInformation();
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

  if (!fStatusBar.IsWindow())
    fStatusBar.Create(this->m_hWnd);
   
}

// fill the control with the list view with all available drives 
void CODINDlg::FillDriveList()
{
  list<wstring>::iterator it;
  int count = 0;
  const size_t BUFSIZE=80;
  wchar_t buffer[BUFSIZE];
  wstring driveTypeString;

	fVolumeList.DeleteAllItems();
	
  count = fOdinManager.GetDriveCount();
  for (int i=0; i<count; i++) {
    CDriveInfo* di = fOdinManager.GetDriveInfo(i);
    fVolumeList.InsertItem(i, di->GetMountPoint().c_str());
    fVolumeList.SetItemText(i, 1, di->GetDisplayName().c_str());
    MakeByteLabel(di->GetBytes(), buffer, BUFSIZE);
    fVolumeList.SetItemText(i, 2, buffer);
    fVolumeList.SetItemText(i, 3, di->GetVolumeName().c_str());
    GetDriveTypeString(di->GetDriveType(), driveTypeString);
    fVolumeList.SetItemText(i, 4, driveTypeString.c_str());
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
  ReadCommentFromDialog();

	if (index == 0)
	{
		BrowseFilesWithFileOpenDialog();
	}
}

void CODINDlg::BrowseFilesWithFileOpenDialog()
{
  CComboBox combo(GetDlgItem(IDC_COMBO_FILES)); 
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
	 
  if ( IDOK == fileDlg.DoModal() ) {
	  sSelectedFile = fileDlg.m_szFileName;
	  int index = combo.FindString(0, sSelectedFile);
	  if (CB_ERR == index) {
		  index = combo.AddString(sSelectedFile);
		  fFiles.push_back(sSelectedFile);
	  }
	  combo.SetCurSel(index);
    UpdateFileInfoBoxAndResetProgressControls();
  }
}

/*
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
        DeleteProcessingInfo(fWasCancelled); // work is finished
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
*/

void CODINDlg::OnThreadTerminated()
{
}

void CODINDlg::OnFinished()
{
  fBytesProcessed = fOdinManager.GetBytesProcessed();
  UpdateStatus(false);
  bool wasVerifyRun = fVerifyRun; // will be deleted in DeleteProcessingInfo()!
  DeleteProcessingInfo(fWasCancelled); // work is finished
  if (wasVerifyRun) {
    CheckVerifyResult();
    fVerifyRun = false;
  }
}

void CODINDlg::OnAbort()
{
  DeleteProcessingInfo(true);
}

void CODINDlg::OnPartitionChange(int i, int n)
{
  CString statusText;

  if (fRestoreRun) {
    if (n > 1)
      statusText.FormatMessageW(IDS_STATUS_RESTORE_DISK_PROGRESS, i+1, n);
    else
      statusText.LoadString(IDS_STATUS_RESTORE_PARTITION_PROGRESS);
  } else if (fBackupRun) {
    if (n > 1)
      statusText.FormatMessageW(IDS_STATUS_BACKUP_DISK_PROGRESS, i+1, n);
    else
      statusText.LoadString(IDS_STATUS_BACKUP_PARTITION_PROGRESS);
  }
  fStatusBar.SetWindowTextW(statusText);
}

void CODINDlg::OnPrepareSnapshotBegin()
{
    CString statusText;
    statusText.LoadString(IDS_STATUS_TAKE_SNAPSHOT);
    fStatusBar.SetWindowTextW(statusText);
}

void CODINDlg::OnPrepareSnapshotReady()
{
    OnPartitionChange(0, 1);
}

void CODINDlg::DeleteProcessingInfo(bool wasCancelled)
{
  wstring msgCopy;
  LPCWSTR msg = fOdinManager.GetErrorMessage();
  CString statusText;
   
  KillTimer(fTimer);
  fTimer = 0;

  // we have an error message copy it before thread gets destroyed and then display it.
  if (msg != NULL)
    msgCopy = msg;

//  fOdinManager.Terminate(wasCancelled);

  if (msg != NULL)
    	AtlMessageBox(m_hWnd, msgCopy.c_str(), IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);

  Init();
  if (wasCancelled)
    statusText.LoadStringW(IDS_STATUS_OPERATION_CANCELLED);
  else
    statusText.LoadString(IDS_STATUS_OPERATION_COMPLETED);
  fStatusBar.SetWindowTextW(statusText);

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
  // CButton verifyButton(GetDlgItem(ID_BT_VERIFY));
  CEdit commentTextField(GetDlgItem(IDC_EDIT_FILE));
  commentTextField.SetReadOnly(FALSE);
  // verifyButton.EnableWindow(FALSE);
  if (fComment.length() == 0) {
    WTL::CString hint;
    hint.LoadString(IDS_ENTERCOMMENT);
    commentTextField.SetWindowText(hint);
  } else
    commentTextField.SetWindowText(fComment.c_str());
    commentTextField.SetSelAll();
}

void CODINDlg::ReadCommentFromDialog()
{
  CEdit commentTextField(GetDlgItem(IDC_EDIT_FILE));
  // save comment if there is one
  WTL::CString hint;
  hint.LoadString(IDS_ENTERCOMMENT);
  ReadWindowText(commentTextField, fComment);
  if (fComment.compare(hint) == 0)
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
  // CButton verifyButton(GetDlgItem(ID_BT_VERIFY));

  ReadWindowText(comboFiles, fileName);
  try {
    bool isMBRFile = CFileNameUtil::TestIsHardDiskImage(fileName.c_str());
    if (isMBRFile) {
      wstring tmpFileName, volumeDeviceName;
      CFileNameUtil::GenerateDeviceNameForVolume(volumeDeviceName, 0, 1);
      CFileNameUtil::GenerateFileNameForEntireDiskBackup(tmpFileName, fileName.c_str(), volumeDeviceName);
      fileName = tmpFileName;
    }
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
    // verifyButton.EnableWindow(TRUE);
  } catch (EWinException& e) {
    if (e.GetErrorCode() == EWinException::fileOpenError) {
      text.Format(IDS_CANNOTOPENFILE, fileName.c_str());
      // verifyButton.EnableWindow(FALSE);
    } 
  } catch (Exception& e) {
    // verifyButton.EnableWindow(FALSE);
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
  BOOL ok = TRUE;
  int index = fVolumeList.GetSelectedIndex();
  CDriveInfo* pDriveInfo = fOdinManager.GetDriveList()->GetItem(index);
  bool isHardDisk = pDriveInfo->IsCompleteHardDisk();
  wstring fileName;
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));

  ReadWindowText(comboFiles, fileName);

  if (isHardDisk && fOdinManager.GetSaveOnlyUsedBlocksOption()) {
    wstring mbrFileName, volumeFileName, filePattern;
    CDriveInfo **pContainedVolumes = NULL;
    mbrFileName = fileName;
    CFileNameUtil::GenerateFileNameForMBRBackupFile(mbrFileName);
    DeleteFile(mbrFileName.c_str());

    int subPartitions = pDriveInfo->GetContainedVolumes();
    pContainedVolumes = new CDriveInfo* [subPartitions];
    int res = fOdinManager.GetDriveList()->GetVolumes(pDriveInfo, pContainedVolumes, subPartitions);

    // check  if there are file names in conflict with files created during backup
    for (int i=0; i<subPartitions; i++) {
      CFileNameUtil::GenerateFileNameForEntireDiskBackup(volumeFileName, fileName.c_str(), pContainedVolumes[i]->GetDeviceName());
      CFileNameUtil::GetEntireDiskFilePattern(volumeFileName.c_str(), filePattern);
      fChecker.CheckUniqueFileName(volumeFileName.c_str(), filePattern.c_str(), false);
    }
    delete [] pContainedVolumes;
  } else {
    if (fOdinManager.GetSplitSize() > 0) {
      wstring filePattern;
      CFileNameUtil::GetSplitFilePattern(fSplitCB, fileName.c_str(), filePattern);
      fChecker.CheckUniqueFileName(fileName.c_str(), filePattern.c_str(), false);
    } else {
      DeleteFile(fileName.c_str());
    }
  }
}



void CODINDlg::GetNoFilesAndFileSize(LPCWSTR fileName, unsigned& fileCount,  unsigned __int64& fileSize, bool& isEntireDriveImageFile)
{
  wstring splitFileName(fileName);
  fileCount = 0;
  fileSize = 0;

  fSplitCB.GetFileName(0, splitFileName);

  if (!CFileNameUtil::IsFileReadable(fileName) && CFileNameUtil::IsFileReadable(splitFileName.c_str())) {
    CFileImageStream fileStream;
    fileStream.Open(splitFileName.c_str(), IImageStream::forReading);
    fileStream.ReadImageFileHeader(false);
    fileCount = fileStream.GetImageFileHeader().GetFileCount();  
    fileSize = fileStream.GetImageFileHeader().GetFileSize();
    isEntireDriveImageFile = fileStream.GetImageFileHeader().GetVolumeType() == CImageFileHeader::volumeHardDisk;
    fileStream.Close();
  }
}
void CODINDlg::ResetRunInformation()
{
  fVerifyRun = fBackupRun = fRestoreRun = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// message handler methods

LRESULT CODINDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
  POINT ptWin = {fWndPosX, fWndPosY };
  WTL::CString drive, name, size, label, type;
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
  label.LoadStringW(IDS_VOLNAME);
  type.LoadStringW(IDS_TYPE);
  fVolumeList = GetDlgItem(IDC_LIST_VOLUMES);
  fVolumeList.AddColumn(drive, 0);
  fVolumeList.AddColumn(name, 1);
  fVolumeList.AddColumn(size, 2);
  fVolumeList.AddColumn(label, 3);
  fVolumeList.AddColumn(type, 4);
  fVolumeList.SetColumnWidth(0, fColumn0Width);
  fVolumeList.SetColumnWidth(1, fColumn1Width);
  fVolumeList.SetColumnWidth(2, fColumn2Width);
  fVolumeList.SetColumnWidth(3, fColumn3Width);
  fVolumeList.SetColumnWidth(4, fColumn4Width);

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
  ResetRunInformation();
  ReadWindowText(comboFiles, fileName);

  try {

    UpdateStatus(true);
    fTimer = SetTimer(cTimerId, 1000); 
    fWasCancelled = false;

    if (fMode == modeBackup) {
      bool ok = true;
      ReadCommentFromDialog();
      fOdinManager.SetComment(fComment.c_str());
      if ( fOdinManager.GetSplitSize() > 0) {
        wstring fileNameWithNumber(fileName);
        CFileNameUtil::RemoveTrailingNumberFromFileName(fileNameWithNumber);
        fSplitCB.GetFileName(0, fileNameWithNumber);
        comboFiles.SetWindowText(fileNameWithNumber.c_str());
      }
      // check if save possible:
      IUserFeedback::TFeedbackResult res = fChecker.CheckConditionsForSavePartition(fileName, index);
      if (res == IUserFeedback::TOk || res == IUserFeedback::TYes) {
        DisableControlsWhileProcessing();
        fBackupRun=true;
        CMultiPartitionHandler::BackupPartitionOrDisk(index, fileName.c_str(),fOdinManager, &fSplitCB, this, fFeedback );
      } 
	  } else if (fMode == modeRestore) {
      unsigned noFiles = 0;
      unsigned __int64 totalSize = 0;
      IUserFeedback::TFeedbackResult res = fChecker.CheckConditionsForRestorePartition(fileName, fSplitCB, index, noFiles, totalSize);
      if (res == IUserFeedback::TOk || res == IUserFeedback::TYes) {
        DisableControlsWhileProcessing();
        fRestoreRun=true;
        CMultiPartitionHandler::RestorePartitionOrDisk(index, fileName.c_str(), fOdinManager, &fSplitCB, this);
      } 
    } else {
      ATLTRACE("Internal Error neither backup nor restore state");
      return 0;
    }
  } catch (Exception& e) {
    wstring msg = e.GetMessage();
    int res = AtlMessageBox(m_hWnd, msg.c_str(), IDS_ERROR, MB_ICONEXCLAMATION | MB_OKCANCEL);
    DeleteProcessingInfo(true);
  }
  catch (...) {
    // We've encountered an exception sometime before we started the operation, so close down everything
    // as gracefully as we can.
    DeleteProcessingInfo(true);
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
      fWasCancelled = true;
      fOdinManager.CancelOperation();
      /*
      DeleteProcessingInfo(true);
      ResetProgressControls();
      if (fMode==modeBackup)
        CleanupPartiallyWrittenFiles();
      */
    }
  } else {
    DeleteProcessingInfo(true);
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
    ReadCommentFromDialog();
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
    DeleteProcessingInfo(true);
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
  fColumn4Width = fVolumeList.GetColumnWidth(4);
  
  // get name of last image file
  wstring fileName;
  CComboBox comboFiles(GetDlgItem(IDC_COMBO_FILES));
  ReadWindowText(comboFiles, fileName);
	int index = comboFiles.GetCurSel();
	if (index != 0) // do not save Browse... label, but save when index is -1
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
  CString statusText;

  ReadWindowText(comboFiles, fileName);
  fCrc32FromFileHeader = 0;

  try {
    UpdateStatus(true);
    DisableControlsWhileProcessing();

    fTimer = SetTimer(cTimerId, 1000); 
    fVerifyRun = true;
    bool run = CMultiPartitionHandler::VerifyPartitionOrDisk(fileName.c_str(), fOdinManager, 
                   fCrc32FromFileHeader, &fSplitCB, this, fFeedback);
    if (run) {
     statusText.LoadString(IDS_STATUS_VERIFY_PROGRESS);
     fStatusBar.SetWindowText(statusText);
    }
    else
      fVerifyRun = false;

  } catch (Exception& e) {
    wstring msg = e.GetMessage();
    int res = AtlMessageBox(m_hWnd, msg.c_str(), IDS_ERROR, MB_ICONEXCLAMATION | MB_OKCANCEL);
    DeleteProcessingInfo(true);
  }
  catch (...) {
    // We've encountered an exception sometime before we started the operation, so close down everything
    // as gracefully as we can.
    DeleteProcessingInfo(true);
    throw;
  }
  return 0;
}

LRESULT CODINDlg::OnBnClickedBtOptions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  COptionsDlg optionsDlg(fOdinManager);
  size_t res = optionsDlg.DoModal();

  return 0;
}

LRESULT CODINDlg::OnBnClickedBtBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  BrowseFilesWithFileOpenDialog();
  return 0;
}

