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
// Implements IUserFeedback interface for console applications
//
///////////////////////////////////////////////////////////////////////////// 

#pragma once

#include "stdafx.h"
#include "UserFeedbackConsole.h"
#include <iostream>
#include <istream>

using namespace std;

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG


CUserFeedbackConsole::CUserFeedbackConsole(bool force) {
  fForce = force;
}

CUserFeedbackConsole::TFeedbackResult CUserFeedbackConsole::UserMessage(TMessageOption option, TFeedbackMode mode, const wchar_t* message) {
  wcout << (option == IUserFeedback::TError ? L"Error: " : L"Warning: ") << message << endl;
  bool notCorrectInput = true;
  const int bufsize = 256;
  wchar_t input[bufsize];

  if (mode == IUserFeedback::TConfirm) {
      wcout << L"Press <return> to continue: ";
      if (!fForce)
        wcin.getline(input, bufsize); 
      return TUnknown;

  } else {
    while (notCorrectInput) {
      if (mode == IUserFeedback::TYesNo) {
        wcout << L"Press (y)es or (n)o: ";
      } else if (mode == IUserFeedback::TOkCancel) {
        wcout << L"Press (y) for OK or (n) for Cancel: "; 
      }
      if (!fForce)
        wcin.getline(input, bufsize); 
      else 
        input[0] = L'Y';

      if (input[0] == 'y' || input[0] == 'Y' || input[0] == 'n' || input[0] == 'N')
        notCorrectInput = false;
    }
    if (input[0] == 'y' || input[0] == 'Y')
      return mode == IUserFeedback::TOkCancel ? TOk : TYes;
    else
      return mode == IUserFeedback::TOkCancel ? TCancel : TNo;
  }
}

