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
#include <string>

// ISplitManagerCallback is a class that provides the names for a file chunk and offers the user 
// to provide a file (e.g. by iinserting a CD) before it is read

class ISplitManagerCallback {
public:
  virtual void GetFileName(unsigned fileNo, std::wstring& fileName) = 0;
  virtual int AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, std::wstring& newName) = 0;
};