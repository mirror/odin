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
#include "OSException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

using namespace std;

// error message strings (should later be moved to a separate non compiled file)
LPCWSTR EWinException::sMessages[] = {
  L"no error",                       // noCode not to be used
  L"test message WinException",      // testError          
  L"Failed to open file: {0}",       // fileOpenError          
  L"Failed to open partition or volume {0}", //volumeOpenError
  L"DeviceIOControl operation with function code {0} failed", // ioControlError
  L"Failed to close handle", // closeHandleError
  L"Failed to read from file: {0}", // readFileError
  L"Failed to write to file: {0}", // writeFileError, 
  L"Failed to read from volume: {0}", // readVolumeError, 
  L"Failed to write to volume: {0}", // writeVolumeError
  L"Failed to seek in file or volume {0}", // seekError 
  L"Problem with file or directory \"{0}\"", // generalFileError 
};


void EWinException::AppendWindowsMessage()
{
  if (fErrorCode != 0) {
    wchar_t *buffer = NULL;
    DWORD len;
    len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL, fErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, NULL );
    if (len > 0)
      fMessage += std::wstring(buffer, 0, len-2);
    if (buffer != NULL)
      LocalFree(buffer);
  }
}

void EWinException::AppendNtStatusMessage()
{
  const int STATUS_SHARING_VIOLATION = 0xc0000043; /* (kernel error code STATUS_SHARING_VIOLATION) */
  // documentation see MS KnowledgeBase article id 259693 
  if (fErrorCode != 0) {
    if (fErrorCode == STATUS_SHARING_VIOLATION) {
      // this is probably the most common error code which has quite a strange message text. 
      // "A file can not be opened because the share flags are incompatible."
      // So we redirect that to get something better understandable:
      // The process can not access the file because it is used by another process.
      int savedErrorCode = fErrorCode;
      fErrorCode = ERROR_SHARING_VIOLATION;
      AppendWindowsMessage();
      fErrorCode = savedErrorCode;
    } else {
      HMODULE handle = LoadLibrary(L"NTDLL.DLL");
      wchar_t *buffer = NULL;
      DWORD len;
       
      len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
              handle, fErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, NULL);
      if (len > 0)
        fMessage += std::wstring(buffer, 0, len-2);
      if (buffer != NULL)
        LocalFree(buffer);
      FreeLibrary(handle);
    }
  }
}

void EWinException::ExpandParameters(const wchar_t** params, int len)
{
  const wchar_t* placeHolders[] = { L"{0}", L"{1}", L"{2}", L"{3}"};
  const wchar_t*pattern;
  wstring tmp1, tmp2;
  const int placeHolderLen = sizeof(placeHolders) / sizeof(placeHolders[0]);
  for (int i=0; i<len && i<placeHolderLen; i++) {
    pattern=placeHolders[i];
    size_t index;
    size_t phlen = wcslen(placeHolders[i]);
    while ((index = fMessage.find(pattern)) != string::npos) {
      tmp1 = fMessage.substr(0, index);
      tmp2 = fMessage.substr(index+phlen, fMessage.length()-index-phlen);
      fMessage = tmp1 + params[i] + tmp2;
    }
  }
}

void OSExceptionTest()
{
}

