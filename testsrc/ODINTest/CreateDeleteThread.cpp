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
#include "CreateDeleteThread.h"
#include "cppunit/extensions/HelperMacros.h"

using namespace std;

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
//
// A helper class to test the VSS service. This thread simply creates and
// deletes files in the target volume. If VSS operates correctly this must
// not influence or corrupt the backup process
//
///////////////////////////////////////////////////////////////////////////// 

CCreateDeleteThread::CCreateDeleteThread(const wchar_t* partitionName) {
  fPartitionName = partitionName;
}

CCreateDeleteThread::~CCreateDeleteThread() {
}

DWORD CCreateDeleteThread::Execute() {
  // first we delete a the first found folder in root directory of target partition recursively 
  // and then we create new random files
  FindFirstRootDirectory();
fFolderToDeleteName.clear();
  if (fFolderToDeleteName.length() == 0) {
    wcout << L"Warning volume for VSS test contains no folder to delete: " << fPartitionName << endl;
    fFolderToDeleteName = fPartitionName;
  }
  DeleteExistingFiles(fFolderToDeleteName.c_str());
  CreateRandomFiles();
  wcout << "CreateDeleteThread terminates." << endl;
  return 0;
}

void CCreateDeleteThread::CreateRandomFiles() {

  // start creating a root folder in the target partition, begin to create files
  // with random content in this folder and delete other files from the existing
  // content in this drive
  wstring folderName;
  BOOL res;

   /* initialize random seed: */
  srand ( GetTickCount() );

  for (unsigned i=0; i<10; i++) {
    wostringstream tmp;
    tmp  << fPartitionName << L"\\NewFolder_" << i;
    folderName = tmp.str();
    res = ::CreateDirectory(folderName.c_str(), NULL);
    CPPUNIT_ASSERT_MESSAGE("can't create directoy for VSS test", res);
    // in each directory create 12 test files with sizes of 10.000, 100.000 and 1.000.000 Bytes
    unsigned no = 0;
    CreateRandomFile(folderName.c_str(), no++, 10000);
    CreateRandomFile(folderName.c_str(), no++, 100000);
    CreateRandomFile(folderName.c_str(), no++, 1000000);
    CreateRandomFile(folderName.c_str(), no++, 10000);
    CreateRandomFile(folderName.c_str(), no++, 100000);
    CreateRandomFile(folderName.c_str(), no++, 1000000);
    CreateRandomFile(folderName.c_str(), no++, 10000);
    CreateRandomFile(folderName.c_str(), no++, 100000);
    CreateRandomFile(folderName.c_str(), no++, 1000000);
    CreateRandomFile(folderName.c_str(), no++, 10000);
    CreateRandomFile(folderName.c_str(), no++, 100000);
    CreateRandomFile(folderName.c_str(), no++, 1000000);
  }
}

void CCreateDeleteThread::CreateRandomFile(const wchar_t* folderName, unsigned number, unsigned size) {
  const unsigned blockSize = 256 * 1024; // 256KB
  BYTE* data = new BYTE[blockSize];
  unsigned iterations = size / blockSize + (size%blockSize ? 1 : 0);
  HANDLE hOutput;
  BOOL ok;
  DWORD dwBytesWritten;
  wstring fileName;
  wostringstream tmp;

  tmp  << folderName << L"\\RandomFile_" << number;
  fileName = tmp.str();

  wcout << L"Create VSS test file " << fileName.c_str() << endl;
  hOutput= CreateFile(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
  CPPUNIT_ASSERT_MESSAGE("can't create test file with random data for VSS test", INVALID_HANDLE_VALUE != hOutput);

  for (unsigned i=0; i<iterations; i++) {
    for (unsigned j=0; j<blockSize; j++)
      data[i] = rand() % 256;
    ok = WriteFile(hOutput, data, blockSize, &dwBytesWritten, NULL); 
    CPPUNIT_ASSERT_MESSAGE("can't write data in test file for VSS test", INVALID_HANDLE_VALUE != hOutput &&  blockSize == dwBytesWritten);
  }

  ok = CloseHandle(hOutput);
  CPPUNIT_ASSERT(ok);
  delete [] data;
}

void CCreateDeleteThread::DeleteExistingFiles(const wchar_t* folder) {
  // recursively traverse given folder and delete all files
  // we recursively traverse the directories and simply delete every 5th file
  int i=0;
  WIN32_FIND_DATA fd;
  HANDLE h;
  BOOL ok;
  wstring rootDir(folder);
  rootDir += L"\\*";
  wcout << L"Deleting VSS files in folder " << folder << endl;
  h = FindFirstFile(rootDir.c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) 
  {
    do {
      wstring absFilePath(folder);
      absFilePath += L"\\";
      absFilePath += fd.cFileName;
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (fd.cFileName[0] != L'.' && fd.cFileName[0] != L'$' &&
           _tcsstr( fd.cFileName, L"System Volume Information") == NULL) {
          DeleteExistingFiles(absFilePath.c_str());
        }
      } else {
          if (i++ % 5 == 0) {
            wcout << L"Deleting VSS test file " << absFilePath.c_str() << endl;
            ok = DeleteFile(absFilePath.c_str());
            CPPUNIT_ASSERT_MESSAGE("delete file for VSS test failed", ok);
          }
      }
    } while (FindNextFile(h, &fd) != 0);
    FindClose(h);
  }
  ok = RemoveDirectory(folder);
  // Do not assert here because directory in most cases is not empty and call will fail
  // CPPUNIT_ASSERT_MESSAGE("delete folder for VSS test failed", ok);
  wcout << L"Deleting VSS test folder " << folder << endl;
}

// Find the first root directory in targe volume. This directory will
// then later be deleted
void CCreateDeleteThread::FindFirstRootDirectory() {
  int i=0;
  WIN32_FIND_DATA fd;
  HANDLE h;
  wstring rootDir(fPartitionName);
  rootDir += L"\\*";
  h = FindFirstFile(rootDir.c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) 
  {
    do {
      wstring absFilePath(fPartitionName);
      absFilePath += L"\\";
      absFilePath += fd.cFileName;
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && fd.cFileName[0] != L'.' && fd.cFileName[0] != L'$' &&
          _tcsstr( fd.cFileName, L"System Volume Information") == NULL) {
        fFolderToDeleteName = absFilePath;
        break;
      }
    } while (FindNextFile(h, &fd) != 0);
    FindClose(h);
  }
}
