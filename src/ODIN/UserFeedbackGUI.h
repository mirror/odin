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
// Implements IUserFeedback interface for Windows GUI
//
///////////////////////////////////////////////////////////////////////////// 

#pragma once

#include "stdafx.h"
#include "UserFeedback.h"
#include "resource.h"
#include <atlmisc.h>

class CUserFeedbackGUI : public IUserFeedback {
public:
  CUserFeedbackGUI(HWND hWnd);

  virtual TFeedbackResult UserMessage(TMessageOption option, TFeedbackMode mode, const wchar_t* message);

private:
  TFeedbackResult ConvertMessageBoxResult(int result);
  int ConvertMessageBoxOption(TFeedbackMode mode);
  unsigned ConvertMessageType(TMessageOption option);
  HWND fHWnd;

};

inline CUserFeedbackGUI::CUserFeedbackGUI(HWND hWnd) {
  fHWnd = hWnd;
}

inline CUserFeedbackGUI::TFeedbackResult CUserFeedbackGUI::ConvertMessageBoxResult(int result) {
  switch (result) {
    case IDOK:
      return TOk;
    case IDCANCEL:
      return TCancel;
    case IDYES:
      return TYes;
    case IDNO:
      return TNo;
    default:
      return TUnknown;
  }
}

inline int CUserFeedbackGUI::ConvertMessageBoxOption(CUserFeedbackGUI::TFeedbackMode mode) {
  switch (mode) {
    case TYesNo:
      return MB_ICONEXCLAMATION | MB_YESNO;
    case TOkCancel:
      return MB_ICONEXCLAMATION | MB_OKCANCEL;
    case TOk:
      return MB_ICONEXCLAMATION | MB_OK;
    default:
      return MB_ICONEXCLAMATION | MB_OK;
  }
}

inline unsigned CUserFeedbackGUI::ConvertMessageType(TMessageOption option) {
  switch (option) {
    case TWarning:
      return IDS_WARNING;
    case TError:
      return IDS_ERROR;
    default:
      return IDS_ERROR;
  }
}


inline CUserFeedbackGUI::TFeedbackResult CUserFeedbackGUI::UserMessage(TMessageOption option, TFeedbackMode mode, const wchar_t* message) {
  int msgRes = AtlMessageBox(fHWnd, message, ConvertMessageType(option), ConvertMessageBoxOption(mode));
  return ConvertMessageBoxResult(msgRes);
}
