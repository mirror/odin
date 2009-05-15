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
 
// ODIN.cpp : main source file for ODIN.exe
//

#include "stdafx.h"
#include <xdebug>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlmisc.h>
#include "resource.h"
#include "ODINDlg.h"
#include "Config.h"
#include "CommandLineProcessor.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

#ifdef DEBUG
void SetMemoryDebuggingFlags() {
  // Get current flag
  int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

  // Turn on leak-checking bit.
  tmpFlag |=_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_LEAK_CHECK_DF;

  // Turn off CRT block checking bit.
  tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;

  // Set flag to the new value.
  _CrtSetDbgFlag( tmpFlag );
}
#endif

CAppModule _Module;
static const wchar_t iniFileName[] = L"ODIN.ini";
static bool sInit = CfgFileInitialize(iniFileName);

static wchar_t initialMessage[] = L"\
This program is licensed under the terms of the GNU General Public License \r\n\
as published by the Free Software Foundation, version 3 of the License. \r\n\
This program is distributed in the hope that it will be useful, but WITHOUT \r\n\
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY \r\n\
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public \r\n\
License in the about box for more details.\r\n\
\r\n\
WARNING: This program is used for backup/restore of disk volumes. By \r\n\
using this program you can destroy the complete contents of your hard disk. \r\n\
Be careful and be sure that you understand the impact of any performed \r\n\
operation. Otherwise do not use this program!\r\n\
\r\n\
WARNING: This program is in early stage of development and not released. \r\n\
Do not use it currently for volumes containing meaningful data and be \r\n\
sure that a backup exists created by another software than this one.";

class CSplashDlg : public CDialogImpl<CSplashDlg>
{
public:
	enum { IDD = IDD_DIALOG_SPLASH};
  CSplashDlg();
  ~CSplashDlg()
  {}
  bool MustDisplay() {
    return !fNotDisplayAgain;
  }
  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  BEGIN_MSG_MAP(CSplashDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  END_MSG_MAP()
private:
  DECLARE_SECTION()
  DECLARE_ENTRY(bool, fNotDisplayAgain) // true if dialog should not be displayed again

};

// section name in .ini file for configuration values
IMPL_SECTION(CSplashDlg, L"DialogOptions")

CSplashDlg::CSplashDlg()
  : fNotDisplayAgain(L"NotShowSplashDlg", false)
{
}

LRESULT CSplashDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  CStatic msgBox( GetDlgItem(IDC_EDIT_MESSAGE) );

  msgBox.SetWindowTextW(initialMessage);

  return 0L;
}

LRESULT CSplashDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CButton displayAgainButton( GetDlgItem(IDC_CHECK_NOTDISPLAYAGAIN) );
  fNotDisplayAgain = displayAgainButton.GetCheck() == TRUE;

  EndDialog(IDOK);
  return 0;
}

LRESULT CSplashDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  EndDialog(IDCANCEL);
  return 0;
}


#if 1 // modal window

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
#ifdef DEBUG
  _CrtMemState stateBegin, stateEnd;
  SetMemoryDebuggingFlags();
  _CrtMemCheckpoint(&stateBegin);
#endif
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);

	ATLASSERT(SUCCEEDED(hRes));


	INT_PTR nRet = 0;
	// BLOCK: Run application
	{
    // Process command line
    CCommandLineProcessor cp;
    bool hasCommandLine = lpstrCmdLine != NULL && *lpstrCmdLine!=L'\0';
    if (hasCommandLine)
    {
      bool consoleApp = cp.InitConsole(hasCommandLine);
      if (consoleApp) {
        // Process command line
        nRet = cp.ParseAndProcess();
      }
    } else {
      // Open main window
      CSplashDlg splashDlg;
      if (splashDlg.MustDisplay())
        splashDlg.DoModal();

		  CODINDlg dlgMain;
		  nRet = dlgMain.DoModal();
	  }
    }
	_Module.Term();
	::CoUninitialize();

#ifdef DEBUG
 _CrtMemCheckpoint(&stateEnd);
 _CrtMemDumpAllObjectsSince( &stateBegin );
#endif
	return (int) nRet;
}

#else // modal window

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
#ifdef DEBUG
  _CrtMemState stateBegin, stateEnd;
  SetMemoryDebuggingFlags();
  _CrtMemCheckpoint(&stateBegin);
#endif
  HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();
#ifdef DEBUG
 _CrtMemCheckpoint(&stateEnd);
 _CrtMemDumpAllObjectsSince( &stateBegin );
#endif

	return nRet;
}
#endif

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
  CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);
	CODINDlg dlgMain;

  if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

//---------------------------------------------------------------------------------