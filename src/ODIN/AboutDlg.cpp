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
// aboutdlg.cpp : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include <atlctrls.h>
#include <atlctrlx.h>
#include <string>
#include <sstream>
#include "AboutDlg.h"
#include "resource.h"

using namespace std;
const wchar_t creditsText[] = L"The zlib data compression library written by Mark Adler and Jean-loup Gailly \
http://www.zlib.net/.\n\
\n\
The bzip2 data compression library written by Julian Seward (Copyright © 1996-2007\
Julian Seward) http://www.bzip.org/.\n\
\n\
Some ideas, design aspects code fragments taken from Selfimage \
http://selfimage.excelcia.org/";

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());
  CStatic versionBox(GetDlgItem(IDC_VERNO));

  GetVersionFromResource();
  versionBox.SetWindowTextW(versionNumber.c_str());

  CStatic creditsBox(GetDlgItem(IDC_CREDITS));
  /* Not painted correctly if multiline*/
  /*
  fHyperCtrl.SubclassWindow(creditsBox.m_hWnd);
  fHyperCtrl.SetHyperLinkExtendedStyle ( HLINK_USETAGS);
  fHyperCtrl.SetLabel(creditsText);
  fHyperCtrl.SetHyperLink ( creditsUlr );
  */
  creditsBox.SetWindowText(creditsText);

  CStatic homepageBox(GetDlgItem(IDC_HYPER));
  fHyperHomepage.SubclassWindow(homepageBox.m_hWnd);
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
  
LRESULT CAboutDlg::OnLicenseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	wchar_t licenseFile[] = L"\\license.html";
  wchar_t dir[MAX_PATH];
  wchar_t fullPath[MAX_PATH];
  DWORD res = GetModuleFileName(NULL, dir, MAX_PATH);
  if (res > 0 && res < MAX_PATH) {
    res = PathRemoveFileSpec(dir);
    if (res) {
      wcscpy_s(fullPath, MAX_PATH, dir);
      wcscat_s(fullPath, MAX_PATH, licenseFile);
      HINSTANCE h = ShellExecute(this->m_hWnd, L"open", fullPath, NULL, dir, SW_SHOW);
      if (h>(HINSTANCE)32) {
        return 0;
      }
    }
  } 
  AtlMessageBox(this->m_hWnd, IDS_LICENSEFILEMISSING, IDS_ERROR, MB_ICONEXCLAMATION | MB_OK);
  return 0;
}

void CAboutDlg::GetVersionFromResource() 
{
  LPWSTR fileEntry = L"\\StringFileInfo\\040904B0\\FileVersion"; // L"FILEVERSION"
  int i1, i2, i3, i4;

  HMODULE hRes = ATL::_AtlBaseModule.GetResourceInstance();
  HRSRC hVersion = FindResource(hRes, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
  if (hVersion != NULL)
  {
    HGLOBAL hGlobal = LoadResource( hRes, hVersion ); 
    if ( hGlobal != NULL)  
    {  
  
      LPVOID versionInfo  = LockResource(hGlobal);  
      if (versionInfo != NULL)
      {
        DWORD i, vLen;
        wostringstream versionStringTmp;
        void* retbuf=NULL;
        LPCWSTR tmp;
        if (VerQueryValue(versionInfo,fileEntry,&retbuf,(UINT *)&vLen)) {
          versionNumber = (LPCWSTR)retbuf;
          tmp = (LPCWSTR) retbuf;
          i1 = _wtoi(tmp);
          i=0;
          while (i < vLen && *++tmp != L',')
            i++;
          ++tmp;
          i2 = _wtoi(tmp);
          while (i < vLen && *++tmp != L',')
            i++;
          ++tmp;
          i3 = _wtoi(tmp);
          while (i < vLen && *++tmp != L',')
            i++;
          ++tmp;
          i4 = _wtoi(tmp);
          versionStringTmp << i1 << L"." << i2 << L"." << i3 << L"." << i4;
          versionNumber = versionStringTmp.str();
        }
      }
      UnlockResource( hGlobal );  
      FreeResource( hGlobal );
    }
  }
}

