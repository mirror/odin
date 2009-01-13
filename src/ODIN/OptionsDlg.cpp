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
#include <atldlgs.h>
#include <atlctrls.h>
#include <atlmisc.h>
#include "OptionsDlg.h"
#include "VSSWrapper.h"

COptionsDlg::COptionsDlg(COdinManager& odinManager)
  : fOdinManager(odinManager)
{
  fOptionWasChanged = false;
}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::Init()
{
  UpdateGetSplitFileMode();
  UpdateGetCompressionMode();
  UpdateGetSaveMode();
}

void COptionsDlg::Commit()
{
  if (fOptionWasChanged) {
    UpdateSetSplitFileMode();
    UpdateSetCompressionMode();
    UpdateSetSaveMode();
  }
}

void COptionsDlg::UpdateGetSplitFileMode()
{
  CButton splitButton( GetDlgItem(IDC_BT_SPLIT_CHUNK) );
  CButton noSplitButton( GetDlgItem(IDC_BT_NO_SPLIT) );
  CEdit editSplitSize ( GetDlgItem(IDC_ED_CHUNK_SIZE) );
  unsigned __int64 splitChunkSize = fOdinManager.GetSplitSize() / 1024Ui64 / 1024Ui64;
  const size_t bufferSize = 20;
  wchar_t buffer[bufferSize];

  // get split file mode
  fSplitFiles = splitChunkSize > 0;
  _ui64tow_s(splitChunkSize, buffer, bufferSize, 10);
  editSplitSize.SetWindowText(buffer);

  if (fSplitFiles) {
    noSplitButton.SetCheck(BST_UNCHECKED);
    splitButton.SetCheck(BST_CHECKED);
    editSplitSize.EnableWindow(TRUE);
  } else {
    splitButton.SetCheck(BST_UNCHECKED);
    noSplitButton.SetCheck(BST_CHECKED);
    editSplitSize.EnableWindow(FALSE);
  }
}

void COptionsDlg::UpdateSetSplitFileMode()
{
  CButton splitButton( GetDlgItem(IDC_BT_SPLIT_CHUNK) );
  CButton noSplitButton( GetDlgItem(IDC_BT_NO_SPLIT) );
  CEdit editSplitSize ( GetDlgItem(IDC_ED_CHUNK_SIZE) );
  unsigned __int64 splitChunkSize;

  // get split file mode
  if (splitButton.GetCheck() == BST_CHECKED) {
    splitChunkSize = GetChunkSizeNumber();
    fOdinManager.SetSplitSize(splitChunkSize);
  }
  else if (noSplitButton.GetCheck() == BST_CHECKED) {
    fOdinManager.SetSplitSize(0);
  }
  else
    ATLASSERT(false);
}

void COptionsDlg::UpdateGetCompressionMode()
{
  // get compression mode
  CButton noCompButton( GetDlgItem(IDC_BT_NO_COMPRESSION) );
  CButton gZipCompButton( GetDlgItem(IDC_BT_COMPRESSION_GZIP) );
  CButton bZip2CompButton( GetDlgItem(IDC_BT_COMPRESSION_BZIP2) );

  fCompressionMode = fOdinManager.GetCompressionMode();
  switch (fCompressionMode) {
    case noCompression:
      noCompButton.SetCheck(BST_CHECKED);
      gZipCompButton.SetCheck(BST_UNCHECKED);
      bZip2CompButton.SetCheck(BST_UNCHECKED);
      break;
    case compressionGZip:
      noCompButton.SetCheck(BST_UNCHECKED);
      gZipCompButton.SetCheck(BST_CHECKED);
      bZip2CompButton.SetCheck(BST_UNCHECKED);
      break;
    case compressionBZIP2:
      noCompButton.SetCheck(BST_UNCHECKED);
      gZipCompButton.SetCheck(BST_UNCHECKED);
      bZip2CompButton.SetCheck(BST_CHECKED);
      break;
  }
}

void COptionsDlg::UpdateSetCompressionMode()
{
  // set compression mode
  CButton noCompButton( GetDlgItem(IDC_BT_NO_COMPRESSION) );
  CButton gZipCompButton( GetDlgItem(IDC_BT_COMPRESSION_GZIP) );
  CButton bZip2CompButton( GetDlgItem(IDC_BT_COMPRESSION_BZIP2) );

  if (noCompButton.GetCheck() == BST_CHECKED)
    fCompressionMode = noCompression;
  else if (gZipCompButton.GetCheck() == BST_CHECKED)
    fCompressionMode = compressionGZip;
  else if (bZip2CompButton.GetCheck() == BST_CHECKED)
    fCompressionMode = compressionBZIP2;
  else
    ATLASSERT(false);

   fOdinManager.SetCompressionMode(fCompressionMode);
}

void COptionsDlg::UpdateGetSaveMode()
{
  CButton saveAllBlocksButton( GetDlgItem(IDC_BT_ALL_BLOCKS) );
  CButton saveOnlyUsedBlocksButton( GetDlgItem(IDC_BT_USED_BLOCKS) );
  CButton snapshotButton( GetDlgItem(IDC_BT_SNAPSHOT) );

  // get save mode
  bool saveOnlyUsedBlocks = fOdinManager.GetSaveOnlyUsedBlocksOption();
  bool makeSnapshot = fOdinManager.GetTakeSnapshotOption();
  if (saveOnlyUsedBlocks) {
    if (makeSnapshot) {
        saveAllBlocksButton.SetCheck(BST_UNCHECKED);
        saveOnlyUsedBlocksButton.SetCheck(BST_UNCHECKED);
        snapshotButton.SetCheck(BST_CHECKED);
     } else {
        saveAllBlocksButton.SetCheck(BST_UNCHECKED);
        saveOnlyUsedBlocksButton.SetCheck(BST_CHECKED);
        snapshotButton.SetCheck(BST_UNCHECKED);
     }
  } else {
    saveAllBlocksButton.SetCheck(BST_CHECKED);
    saveOnlyUsedBlocksButton.SetCheck(BST_UNCHECKED);
    snapshotButton.SetCheck(BST_UNCHECKED);
  }
  
  // disable snapshot option if not available on this platform
  if (!CVssWrapper::VSSIsSupported()) {
    snapshotButton.EnableWindow(FALSE);
    if (makeSnapshot) {
      saveOnlyUsedBlocksButton.SetCheck(BST_CHECKED);
      snapshotButton.SetCheck(BST_UNCHECKED);
    }
  }
}

void COptionsDlg::UpdateSetSaveMode()
{
  CButton saveAllBlocksButton( GetDlgItem(IDC_BT_ALL_BLOCKS) );
  CButton saveOnlyUsedBlocksButton( GetDlgItem(IDC_BT_USED_BLOCKS) );
  CButton snapshotButton( GetDlgItem(IDC_BT_SNAPSHOT) );

  if (saveOnlyUsedBlocksButton.GetCheck() == BST_CHECKED) {
    fOdinManager.SetSaveOnlyUsedBlocksOption(true);
    fOdinManager.SetTakeSnapshotOption(false);
  } else if (snapshotButton.GetCheck() == BST_CHECKED ) {
    fOdinManager.SetSaveOnlyUsedBlocksOption(true);
    fOdinManager.SetTakeSnapshotOption(true);
  } else {
    fOdinManager.SetSaveOnlyUsedBlocksOption(false);
    fOdinManager.SetTakeSnapshotOption(false);
  }
}

unsigned __int64 COptionsDlg::GetChunkSizeNumber()
{
  CEdit editSplitSize ( GetDlgItem(IDC_ED_CHUNK_SIZE) );
  unsigned __int64 splitChunkSize;
  WTL::CString s;
  const int cBufferLen = 100;
  LPWSTR buf = s.GetBuffer(cBufferLen);
  editSplitSize.GetWindowText(buf, cBufferLen);
  s.ReleaseBuffer();
  splitChunkSize  = _wtol(s) * 1024Ui64 * 1024Ui64;
  return splitChunkSize;
}

LRESULT COptionsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  Init();
  return 0;
}

LRESULT COptionsDlg::OnBnClickedBtUsedBlocks(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  fOptionWasChanged = true;
  return 0;
}

LRESULT COptionsDlg::OnBnClickedBtAllBlocks(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  fOptionWasChanged = true;
  return 0;
}

LRESULT COptionsDlg::OnBnClickedBtCompressionBzip2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  fOptionWasChanged = true;
  return 0;
}

LRESULT COptionsDlg::OnBnClickedBtCompressionGzip(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  fOptionWasChanged = true;
  return 0;
}

LRESULT COptionsDlg::OnBnClickedBtNoCompression(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  fOptionWasChanged = true;
  return 0;
}

LRESULT COptionsDlg::OnBnClickedBtNoSplit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CEdit editSplitSize ( GetDlgItem(IDC_ED_CHUNK_SIZE) );
  editSplitSize.EnableWindow(FALSE);

  fOptionWasChanged = true;
  return 0;
}

LRESULT COptionsDlg::OnBnClickedBtSplitChunk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CEdit editSplitSize ( GetDlgItem(IDC_ED_CHUNK_SIZE) );
  editSplitSize.EnableWindow(TRUE);

  fOptionWasChanged = true;
  return 0;
}

LRESULT COptionsDlg::OnBnClickedBtVSSSnapshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  fOptionWasChanged = true;
  return 0;
}

LRESULT COptionsDlg::OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  // check if edit field has changed value
  CButton splitButton( GetDlgItem(IDC_BT_SPLIT_CHUNK) );
  if (splitButton.GetCheck() == BST_CHECKED) {
    unsigned __int64 splitChunkSize = GetChunkSizeNumber();
    if (splitChunkSize != fOdinManager.GetSplitSize())
      fOptionWasChanged = true;
  }

  if (fOptionWasChanged)
    Commit();
  EndDialog(IDOK);
  return 0;
}

LRESULT COptionsDlg::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  fOptionWasChanged = false;
  EndDialog(IDCANCEL);
  return 0;
}


