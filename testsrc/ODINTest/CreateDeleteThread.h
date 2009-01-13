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

/////////////////////////////////////////////////////////////////////////////
//
// A helper class to test the VSS service. This thread simply creates and
// deletes files in the target volume. If VSS operates correctly this must
// not influence or corrupt the backup process
//
///////////////////////////////////////////////////////////////////////////// 

#include <string>
#include "..\..\src\ODIN\Thread.h"

#pragma once
#ifndef __CREATEDELETETHREAD_H__
#define __CREATEDELETETHREAD_H__

class CCreateDeleteThread : public CThread {

public:
  CCreateDeleteThread(const wchar_t* partitionName  );
  virtual ~CCreateDeleteThread();
  virtual DWORD Execute();

private:
  void CreateRandomFiles();
  void CreateRandomFile(const wchar_t* folderName, unsigned number, unsigned size);
  void DeleteExistingFiles(const wchar_t* folder);
  void FindFirstRootDirectory();

  std::wstring fPartitionName;
  std::wstring fFolderToDeleteName;
};

#endif
