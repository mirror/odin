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
#include "resource.h"
#include "ODINManager.h"
#include <string>

/////////////////////////////////////////////////////////////////////////////
//
// dialog class for asking for a file
//
/////////////////////////////////////////////////////////////////////////////

class COptionsDlg : public CDialogImpl<COptionsDlg>
{
  public:
	enum { IDD = IDD_OPTIONS };

	COptionsDlg(COdinManager& odinManager);
	~COptionsDlg();
  void Commit();

  BEGIN_MSG_MAP(COptionsDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_HANDLER(IDC_BT_ALL_BLOCKS, BN_CLICKED, OnBnClickedBtAllBlocks)
    COMMAND_HANDLER(IDC_BT_COMPRESSION_BZIP2, BN_CLICKED, OnBnClickedBtCompressionBzip2)
    COMMAND_HANDLER(IDC_BT_COMPRESSION_GZIP, BN_CLICKED, OnBnClickedBtCompressionGzip)
    COMMAND_HANDLER(IDC_BT_NO_COMPRESSION, BN_CLICKED, OnBnClickedBtNoCompression)
    COMMAND_HANDLER(IDC_BT_NO_SPLIT, BN_CLICKED, OnBnClickedBtNoSplit)
    COMMAND_HANDLER(IDC_BT_SPLIT_CHUNK, BN_CLICKED, OnBnClickedBtSplitChunk)
 		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_HANDLER(IDC_BT_USED_BLOCKS, BN_CLICKED, OnBnClickedBtUsedBlocks)
    COMMAND_HANDLER(IDC_BT_SNAPSHOT, BN_CLICKED, OnBnClickedBtVSSSnapshot)
  END_MSG_MAP()

private:
  void Init();
  void UpdateGetSplitFileMode();
  void UpdateSetSplitFileMode();
  void UpdateGetCompressionMode();
  void UpdateSetCompressionMode();
  void UpdateGetSaveMode();
  void UpdateSetSaveMode();

  unsigned __int64 GetChunkSizeNumber();

  COdinManager& fOdinManager;
  TCompressionFormat fCompressionMode;
  bool fSplitFiles;
  typedef enum { modeOnlyUsedBlock, modeUsedBlocksAndSnapshot, modeAllBlocks } TBackupMode;
  TBackupMode fBackupMode;
  bool fOptionWasChanged;

public:
  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtAllBlocks(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtCompressionBzip2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtCompressionGzip(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtNoCompression(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtNoSplit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtSplitChunk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtUsedBlocks(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnBnClickedBtVSSSnapshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
