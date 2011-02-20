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
// ODINC.cpp : Defines the entry point for the console application.
//

#ifndef _WIN32_WINNT            
#define _WIN32_WINNT 0x0500     
#endif

#include <stdio.h>
#include <wchar.h>
#include <windows.h>

int wmain(int argc, wchar_t* argv[])
{
  const int cBufLen = 1024;
  int n;
  wchar_t cmdLine[cBufLen];
  wcsncpy_s(cmdLine, cBufLen, GetCommandLine(), cBufLen);
  size_t len = wcslen(cmdLine);
  for (n=0; n<(int)len; n++)
    cmdLine[n] = towupper(cmdLine[n]);
  wchar_t* pos = wcsstr(cmdLine, L"ODINC.EXE");
  wchar_t* pos2 = wcsstr(cmdLine, L"ODINC");
  // get again because we do not want pass toupper 
  wcsncpy_s(cmdLine, cBufLen, GetCommandLine(), cBufLen);
  if (pos != NULL) {
    memcpy(pos, L"odin.exe ", 9*sizeof(wchar_t));
    n=8;
  } else if (pos2 != NULL) {
    memcpy(pos2, L"odin ", 5*sizeof(wchar_t));
    n=4;
    pos = pos2;
  }
  else {
    wprintf(L"You must call this program with odinc oder odinc.exe as name.\n");
    return 1;
  }

  pos+=n;
  do {
   *pos=*(pos++ +1);
  } while (*(pos+1)==L' ');

  SECURITY_ATTRIBUTES secAttr;
  secAttr.nLength = sizeof(secAttr);
  secAttr.lpSecurityDescriptor = NULL;
  secAttr.bInheritHandle = TRUE;
  STARTUPINFO  startupInfo;
  ZeroMemory(&startupInfo, sizeof(startupInfo));
  PROCESS_INFORMATION procInfo;
  ZeroMemory(&procInfo, sizeof(procInfo));
  BOOL ok = CreateProcess(NULL, cmdLine, &secAttr, &secAttr, TRUE, 0, NULL, NULL, &startupInfo, &procInfo);
  if (!ok) {
    wprintf(L"CreateProcess() for ODIN failed.\n");
    return 1;
  }
  WaitForSingleObject(procInfo.hProcess,INFINITE); // wait until ODIN terminates
  return 0;
}

