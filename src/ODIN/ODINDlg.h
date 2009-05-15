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
// Main dialog class for ODIN
//
///////////////////////////////////////////////////////////////////////////// 
#include "resource.h"
#include <list>
#include <string>
#ifndef _WTL_SUPPORT_SDK_ATL3
#include <atltime.h>
#endif
// #include <atlddx.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atlctrlx.h>
#include "OdinManager.h"
#include "Config.h"
#include "DriveList.h"
#include "SplitManagerCallback.h"
#include "ParamChecker.h"
#include "UserFeedbackGUI.h"

using namespace std;

class CODINSplitManagerCallback : public ISplitManagerCallback 
{
public:
  CODINSplitManagerCallback(HWND hWnd)
  {
    fhWnd = hWnd;
  }
  virtual void GetFileName(unsigned fileNo, std::wstring& fileName);
  virtual size_t AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, std::wstring& newName);

  void SetBaseName(LPCWSTR baseName)
  {
    fBaseName = baseName;
  }

private:
  wstring fBaseName; // file name patttern where number string is appended to
  HWND fhWnd; // parent window handle for message box
};

class CODINDlg : public CDialogImpl<CODINDlg>
  , IWaitCallback
		/*, public CDialogResize<CODINDlg>, public CUpdateUI<CODINDlg>, public CMessageFilter, public CIdleHandler,*/ 
{
public:
	enum { IDD = IDD_ODIN_MAIN };
	typedef enum {modeBackup, modeRestore} TOperationMode;

private:
  COdinManager fOdinManager;
  std::list<std::wstring> fFiles;
  std::list<std::wstring> fDriveNames;
  TOperationMode fMode;
  CListViewCtrl  fVolumeList;
  WTL::CString fVolumeInfoTemplate;
  CMultiPaneStatusBarCtrl fStatusBar;

  __int64 fSourceSize;
  __int64 fMaxTargetSize;
  __int64 fBytesProcessed;
  unsigned fSpeed; // bytes per second of write speed
  unsigned fThreadCount; // number of threads in use
  UINT_PTR fTimer;
  CODINSplitManagerCallback fSplitCB;
  DWORD fCrc32FromFileHeader; // checksum contained in file header of image file 
  bool fVerifyRun;   // current operation is a run to verify integrity of a file
  bool fBackupRun;   // current operation is a run to for backup
  bool fRestoreRun;   // current operation is a run to for restore
  std::wstring fComment; // a comment stored when backing up a file
  bool fWasCancelled;
  static const UINT cTimerId = 111;
  CUserFeedbackGUI fFeedback;
  CParamChecker fChecker;

  DECLARE_SECTION()
  DECLARE_ENTRY(int, fWndPosX) // x position of dialog window
  DECLARE_ENTRY(int, fWndPosY) // y postition of dialog window
  DECLARE_ENTRY(bool, fLastOperationWasBackup) // save if backup or restore was done last
  DECLARE_ENTRY(int, fLastDriveIndex) // last drive that was used
  DECLARE_ENTRY(int, fColumn0Width)   // width og column 0 in volume list control
  DECLARE_ENTRY(int, fColumn1Width)   // width og column 1 in volume list control
  DECLARE_ENTRY(int, fColumn2Width)   // width og column 2 in volume list control
  DECLARE_ENTRY(int, fColumn3Width)   // width og column 3 in volume list control
  DECLARE_ENTRY(int, fColumn4Width)   // width og column 4 in volume list control
  DECLARE_ENTRY(std::wstring, fLastImageFile) // path to last image file that was used

  void Init();
  void InitControls();
  void RefreshDriveList();
  void FillDriveList();
  void BrowseFiles(WORD wID);
  void BrowseFilesWithFileOpenDialog();
  void GetPartitionFileSystemString(int partType, WTL::CString& fsString);
  int GetImageIndexAndUpdateImageList (LPCWSTR drive);

  void DeleteProcessingInfo(bool wasCancelled);
  bool CheckThreadsForErrors();
  void UpdateStatus(bool bInit);
  void ResetProgressControls();
  void GetNoFilesAndFileSize(LPCWSTR fileName, unsigned& fileCount,  unsigned __int64& fileSize, bool& isEntireDriveImageFile);
  void ReadWindowText(CWindow& wnd, std::wstring& str) {
    int count;
    WTL::CString s;
		count = wnd.GetWindowTextLength() + 1;
		wnd.GetWindowText(s.GetBuffer(count), count);
    str = s;
  }
  void UpdateFileInfoBox();
  void UpdateFileInfoBoxAndResetProgressControls();
  void ReadImageFileInformation();
  void EnterCommentModeForEditBox();
  void ReadCommentFromDialog();
  void CheckVerifyResult();
  void DisableControlsWhileProcessing();
  void EnableControlsAfterProcessingComplete();
  void CleanupPartiallyWrittenFiles();
  void ResetRunInformation();
  virtual void OnThreadTerminated();
  virtual void OnFinished();
  virtual void OnAbort();
  virtual void OnPartitionChange(int i, int n);
  virtual void OnPrepareSnapshotBegin();
  virtual void OnPrepareSnapshotReady();
public:

	CODINDlg();
	~CODINDlg();

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnDeviceChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

public:
 	BEGIN_MSG_MAP(CODINDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	MESSAGE_HANDLER(WM_DEVICECHANGE, OnDeviceChanged)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_HANDLER(IDC_RADIO_BACKUP, BN_CLICKED, OnBnClickedRadioBackup)
    COMMAND_HANDLER(IDC_RADIO_RESTORE, BN_CLICKED, OnBnClickedRadioRestore)
    COMMAND_HANDLER(IDC_COMBO_FILES, CBN_SELCHANGE, OnCbnSelchangeComboFiles)
	COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
    NOTIFY_HANDLER(IDC_LIST_VOLUMES, LVN_ITEMCHANGED, OnLvnItemchangedListVolumes)
    // CHAIN_MSG_MAP(CDialogResize<CODINDlg>)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    COMMAND_HANDLER(IDC_COMBO_FILES, CBN_KILLFOCUS, OnCbnKillfocusComboFiles)
    COMMAND_HANDLER(ID_BT_VERIFY, BN_CLICKED, OnBnClickedBtVerify)
    COMMAND_HANDLER(ID_BT_OPTIONS, BN_CLICKED, OnBnClickedBtOptions)
    COMMAND_HANDLER(ID_BT_BROWSE, BN_CLICKED, OnBnClickedBtBrowse)
  END_MSG_MAP()
  
/* does not draw correctly 
  BEGIN_DLGRESIZE_MAP(CODINDlg)
    DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_LIST_VOLUMES, DLSZ_SIZE_X|DLSZ_SIZE_Y)
    // DLGRESIZE_CONTROL(IDC_LABEL, DLSZ_SIZE_X |DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_LABEL_BYTES_PROCESSED, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_LABEL_TIME_ELAPSED, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_LABEL_TIME_LEFT, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_LABEL_SPEED, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_LABEL_PERCENT, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_LABEL_BYTES_TOTAL, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(ID_APP_ABOUT, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_GROUP_DISKVOL, DLSZ_SIZE_X|DLSZ_SIZE_Y)
    DLGRESIZE_CONTROL(IDC_GROUP_FILE, DLSZ_SIZE_X|DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_COMBO_FILES, DLSZ_SIZE_X |DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_TEXT_VOLUME, DLSZ_SIZE_X |DLSZ_MOVE_Y)
  END_DLGRESIZE_MAP()
*/
  LRESULT OnBnClickedRadioBackup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedRadioRestore(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnCbnSelchangeComboFiles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnLvnItemchangedListVolumes(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
  LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnStnClickedTextFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnCbnKillfocusComboFiles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtVerify(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtOptions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};