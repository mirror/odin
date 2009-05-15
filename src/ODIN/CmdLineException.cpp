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
#include "CmdLineException.h"
#include <atlmisc.h>
#include "resource.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

using namespace std;

void ECmdLineException::Init(enum ExceptionCode errCode)
{
  WTL::CString msg;
  fErrorCode = errCode;

  switch (errCode) {
    case noCode:
      msg = "No Error";
      break;
    case noSource:
      msg.LoadString(IDS_ERRCMDLINE_NO_SOURCE);
      break;
    case noTarget:
      msg.LoadString(IDS_ERRCMDLINE_NO_TARGET);
      break;
    case noOperation:
      msg.LoadString(IDS_ERRCMDLINE_NO_OPERATION);
      break;
    case wrongCompression:
      msg.LoadString(IDS_ERRCMDLINE_WRONG_COMPRESSION);
      break;
    case unknownOption:
      msg.LoadString(IDS_ERRCMDLINE_UNKNOWN_OPTION);
      break;
    case wrongSource:
      msg.LoadString(IDS_ERRCMDLINE_WRONG_SOURCE);
      break;
    case  wrongTarget:
      msg.LoadString(IDS_ERRCMDLINE_WRONG_TARGET);
      break;
    case wrongIndex:
      msg.LoadString(IDS_ERRCMDLINE_WRONG_INDEX);
      break;
    case backupParamError:
      msg.LoadString(IDS_ERRCMDLINE_BACKUP_PARAM_ERROR);
      break;
    case restoreParamError:
      msg.LoadString(IDS_ERRCMDLINE_RESTORE_PARAM_ERROR);
      break;
    case verifyParamError:
      msg.LoadString(IDS_ERRCMDLINE_VERIFY_PARAM_ERROR);
      break;
    default:
      msg = L"unknown error";
      break;
  }
  fMessage += msg;
  fMessage += L'\n';
  msg.LoadString(IDS_ERRCMDLINE_USAGE);
  fMessage += msg;

}