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
#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)

#include <string>

#define THROWEX(cls, param) throw cls(param, __WFILE__, __LINE__)

class Exception
{
public:
  const static int cNoExceptionTypes = 6;
  typedef enum ExceptionCategory {OSException, CompressionException, InternalException, FileFormatException, 
    VSSException, CmdLineException };

  Exception(ExceptionCategory cat)
    : fCategory(cat)
  {
    InitMessageString();
  }

  Exception(ExceptionCategory cat, LPCWSTR file, int line)
    : fCategory(cat), fFile(file), fLine(line)
  {
    InitMessageString();
  }

  virtual ~Exception()
  {
  }

  LPCWSTR GetCategoryStr() {
    if (fCategory >= 0 && fCategory < cNoExceptionTypes) // sizeof(sExceptionCategoriesDescription) / sizeof(sExceptionCategoriesDescription[0]) )
      return sExceptionCategoriesDescription[fCategory];
    else 
      return sUnknownCategory;
  }
  
  virtual LPCWSTR GetMessage()
  {
     return fMessage.c_str(); // default implementation returns fMessage, subclasses can do different
  }

  void LogMessage() {
    ATLTRACE("Exception was thrown from file: %S, line: %d: %S\n", fFile, fLine, GetMessage());
  }

private:
  void InitMessageString()
  {
    fMessage = std::wstring(L"Exception of type ") + GetCategoryStr() + L" thrown, cause: ";
  }

protected:
  std::wstring fMessage;

  void ExpandParameters(const wchar_t** params, int len);

private:
  int fCategory;
  LPCWSTR fFile;
  int fLine;

  static LPCWSTR sExceptionCategoriesDescription[];
  static LPCWSTR sUnknownCategory;
};

#endif
