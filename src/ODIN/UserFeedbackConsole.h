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
// Interface class that asks a user for feedback. Concrete subclasses implement
// the interface either by e.g. MessageBox for a GUI environment or command
// line input and output for a console environment
//
///////////////////////////////////////////////////////////////////////////// 

#pragma once
#include "UserFeedback.h"

class CUserFeedbackConsole : public IUserFeedback {
public:
  CUserFeedbackConsole(bool force=false);

  virtual TFeedbackResult UserMessage(TMessageOption option, TFeedbackMode mode, const wchar_t* message);

private:
  boolean fForce;
};
