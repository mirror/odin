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
#include <string>
#include "Exception.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

LPCWSTR Exception::sExceptionCategoriesDescription[] = {
  L"Windows Exception",           // OSException
  L"Compression Exception",       // CompressionException
  L"Internal Exception",          // InternalException
  L"File Format Exception",       // FileFormatException
  L"Volume Shadow Service Exception", // VSSException
  L"Command Line Exception",      // CommandLineException
};

LPCWSTR Exception::sUnknownCategory = L"unknown category";

void Exception::ExpandParameters(const wchar_t** params, int len) {
  const wchar_t* placeHolders[] = { L"{0}", L"{1}", L"{2}", L"{3}"};
  const wchar_t* pattern;
  std::wstring tmp1, tmp2;
  const int placeHolderLen = sizeof(placeHolders) / sizeof(placeHolders[0]);
  for (int i=0; i<len && i<placeHolderLen; i++) {
    pattern=placeHolders[i];
    size_t index;
    size_t phlen = wcslen(placeHolders[i]);
    while ((index = fMessage.find(pattern)) != std::string::npos) {
      tmp1 = fMessage.substr(0, index);
      tmp2 = fMessage.substr(index+phlen, fMessage.length()-index-phlen);
      fMessage = tmp1 + params[i] + tmp2;
    }
  }
}

//---------------------------------------------------------------------------

