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
 
// TODO: clean remaining files in case of testfailure before a new test ist started
// TODO: implement recursive copy

#include "stdafx.h"
#include "..\..\src\ODIN\ODINManager.h"
#include "..\..\src\ODIN\DriveList.h"
#include "..\..\src\ODIN\Exception.h"
#include "..\..\src\ODIN\ImageStream.h"
#include "..\..\src\ODIN\SplitManager.h"
#include "..\..\src\ODIN\CRC32.h"
#include "ODINManagerTest.h"

using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ODINManagerTest );

// Dummy test for initialization
InitClass::InitClass(LPCWSTR fileName)
{
  CfgFileInitialize(fileName, true);
}

// Test setup class:

  // section name in .ini file for configuration values
IMPL_SECTION(ODINManagerTest, L"ODINManagerTest")

ODINManagerTest::ODINManagerTest()
 : dummyInit(L"ODINTest.ini"),
 fEnableTest(L"EnableManagerTest", false),
 fTestDataDir(L"TestDataDir", L"c:\\testdata"),
 fImageDrive(L"ImageDrive", L"e:"),
 fImageSaveDir(L"ImageSaveDir", L"c:\\testout"),
 fSleepTime(L"WaitTimeAfterFormat", 1000)
{
}

/////////////////////////////////////////////////////////////////
// callback class for splitting files

class CSplitManagerCallback : public ISplitManagerCallback 
{
public:
  virtual void GetFileName(unsigned fileNo, std::wstring& fileName) {
    wchar_t buf[10];
    wsprintf(buf, L"%04u", fileNo);
    fileName += buf;
  }

  virtual int AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, std::wstring& newName) {
    return IDCANCEL;
  }
};

// Start/Terminate methods:
void ODINManagerTest::setUp()
{
  if (fEnableTest) {
  wcout << L"Printing configuration for ODINManagertest:" << endl;
  wcout << L"===========================================" << endl;
  wcout << L"Test data dir: " << fTestDataDir().c_str() << endl;
  wcout << L"drive that is backuped and restored: " << fImageDrive().c_str() << endl;
  wcout << L"WARNING: This drive is formatted and all contained data will be lost!" << endl;
  wcout << L"directory where image is stored: " << fImageSaveDir().c_str() << endl;
  wcout << L"Time in ms to wait after formatting drive: " << fSleepTime() << endl;
  
  fMgr.RefreshDriveList();

  // save original options:
  fOldCompressionMode = fMgr.GetCompressionMode();
  fOldUseOnlyUsedBlocks = fMgr.GetSaveOnlyUsedBlocksOption();
  fOldSplitSize = fMgr.GetSplitSize();
  } else {
    wcout << "ODINManagerTest is disabled in ODINManagerTest.ini" << endl;
    wcout << L"WARNING: This test needs a separate drive that is formatted" << endl;
  }
}

void ODINManagerTest::tearDown()
{
  // restore old options:
  fMgr.SetCompressionMode(fOldCompressionMode);
  fMgr.SetSaveOnlyUsedBlocksOption(fOldUseOnlyUsedBlocks);
  fMgr.SetSplitSize(fOldSplitSize);
}

// Test methods:
// Test 1: save and restore FAT16 uncompressed, used blocks
void ODINManagerTest::ODINManagerTest0() 
{
  if (!fEnableTest) 
    return;

  cout << "ODINManagerTest0 FAT16 gzip used blocks..." << endl;
  const wstring testImageFileName(L"FAT16NoCompUsedBlocks.img");
  wstring testImageFileFullPath;
  LPCWSTR volumeLabel = L"FAT16ODINTEST";
  CreateFullPath(testImageFileName, testImageFileFullPath);
  SetBackupOptions(noCompression, true, 0);
  // run test:
  ODINManagerTestTemplate(FAT, volumeLabel, testImageFileFullPath.c_str(), false);

  BOOL ok = ::DeleteFile(testImageFileFullPath.c_str());
  CPPUNIT_ASSERT_MESSAGE("Failed to delete image file after backup/restore", ok);
  cout << "...done." << endl;
}

// Test 2: save and restore FAT32 gzip compressed, used blocks
void ODINManagerTest::ODINManagerTest1()
{
  if (!fEnableTest) 
    return;

  cout << endl << endl;
  cout << "============================================================" << endl;
  cout << "ODINManagerTest1 FAT32 gzip used blocks..." << endl;
  cout << "============================================================" << endl;
  const wstring testImageFileName(L"FAT32GZipUsedBlocks.img");
  wstring testImageFileFullPath;
  LPCWSTR volumeLabel = L"FAT32ODINTEST";
  CreateFullPath(testImageFileName, testImageFileFullPath);
  SetBackupOptions(compressionGZip, true, 0);
  // run test:
  ODINManagerTestTemplate(FAT32, volumeLabel, testImageFileFullPath.c_str(), false);
  BOOL ok = ::DeleteFile(testImageFileFullPath.c_str());
  CPPUNIT_ASSERT_MESSAGE("Failed to delete image file after backup/restore", ok);
  cout << "...done." << endl;
}

// Test 3: save and restore NTFS bzip2 compressed, used blocks
void ODINManagerTest::ODINManagerTest2()
{
  if (!fEnableTest) 
    return;

  cout << endl << endl;
  cout << "============================================================" << endl;
  cout << "ODINManagerTest2 NTFS used blocks bzip2..." << endl;
  const wstring testImageFileName(L"NTFSBZip2UsedBlocks.img");
  wstring testImageFileFullPath;
  LPCWSTR volumeLabel = L"NTFSODINTEST";
  CreateFullPath(testImageFileName, testImageFileFullPath);
  SetBackupOptions(compressionBZIP2, true, 0);
  // run test:
  ODINManagerTestTemplate(NTFS, volumeLabel, testImageFileFullPath.c_str(), false);
  BOOL ok = ::DeleteFile(testImageFileFullPath.c_str());
  CPPUNIT_ASSERT_MESSAGE("Failed to delete image file after backup/restore", ok);
  cout << "...done." << endl;
}

// Test 4: save and restore FAT16 bzip2 compressed, all blocks
void ODINManagerTest::ODINManagerTest3()
{
  if (!fEnableTest) 
    return;

  cout << endl << endl;
  cout << "============================================================" << endl;
  cout << "ODINManagerTest3 FAT16 bzip2 all blocks..." << endl;
  const wstring testImageFileName(L"FAT16BZip2AllBlocks.img");
  wstring testImageFileFullPath;
  LPCWSTR volumeLabel = L"FAT16ODINTEST";
  CreateFullPath(testImageFileName, testImageFileFullPath);
  SetBackupOptions(compressionBZIP2, false, 0);
  // run test:
  ODINManagerTestTemplate(FAT, volumeLabel, testImageFileFullPath.c_str(), false);
  BOOL ok = ::DeleteFile(testImageFileFullPath.c_str());
  CPPUNIT_ASSERT_MESSAGE("Failed to delete image file after backup/restore", ok);
  cout << "...done." << endl;
}

// Test 5: save and restore FAT32 uncompressed, all blocks
void ODINManagerTest::ODINManagerTest4()
{
  if (!fEnableTest) 
    return;

  cout << endl << endl;
  cout << "============================================================" << endl;
  cout << "ODINManagerTest4, FAT32 uncompressed all blocks..." << endl;
  const wstring testImageFileName(L"FAT32UncompAllBlocks.img");
  wstring testImageFileFullPath;
  LPCWSTR volumeLabel = L"FAT32ODINTEST";
  CreateFullPath(testImageFileName, testImageFileFullPath);
  SetBackupOptions(noCompression, false, 0);
  // run test:
  ODINManagerTestTemplate(FAT32, volumeLabel, testImageFileFullPath.c_str(), false);
  BOOL ok = ::DeleteFile(testImageFileFullPath.c_str());
  CPPUNIT_ASSERT_MESSAGE("Failed to delete image file after backup/restore", ok);
  cout << "...done." << endl;
}

// Test 6: save and restore NTFS gzip compressed
void ODINManagerTest::ODINManagerTest5()
{
  if (!fEnableTest) 
    return;

  cout << endl << endl;
  cout << "============================================================" << endl;
  cout << "ODINManagerTest5 NTFS gzip all blocks..." << endl;
  cout << "============================================================" << endl;
  const wstring testImageFileName(L"NTFSGZipAllBlocks.img");
  wstring testImageFileFullPath;
  LPCWSTR volumeLabel = L"NTFSODINTEST";
  CreateFullPath(testImageFileName, testImageFileFullPath);
  SetBackupOptions(compressionGZip, false, 0);
  // run test:
  ODINManagerTestTemplate(NTFS, volumeLabel, testImageFileFullPath.c_str(), false);
  BOOL ok = ::DeleteFile(testImageFileFullPath.c_str());
  CPPUNIT_ASSERT_MESSAGE("Failed to delete image file after backup/restore", ok);
  cout << "...done." << endl;
}

void ODINManagerTest::ODINManagerSplitTest() 
{
  if (!fEnableTest) 
    return;

  cout << endl << endl;
  cout << "============================================================" << endl;
  cout << "ODINManagerSplitTest FAT16 uncompressed all blocks, split files " << endl;
  cout << "============================================================" << endl;
  const wstring testImageFileName(L"FAT16NoCompAllBlocksSplit.img");
  unsigned __int64 maxFileSize = 1024 * 4096; // 1MB
  wstring splitFilePath, testImageFileFullPath;
  LPCWSTR volumeLabel = L"FAT16ODINTEST";
  unsigned fileCount;
  CSplitManagerCallback cb;

  // backup with splt files fail if the target files already exists. Therefore first delete 
  // all files in case that the last test run stopped with an error and left the files
  bool foundFile = true;
  for (unsigned i=0; foundFile; i++) {
    splitFilePath = testImageFileFullPath;
    CSplitManagerCallback cb;
    cb.GetFileName(i, splitFilePath);
    foundFile = ::DeleteFile(splitFilePath.c_str()) != FALSE;
  }

  CreateFullPath(testImageFileName, testImageFileFullPath);
  SetBackupOptions(noCompression, true, maxFileSize);
  // run test:

  ODINManagerTestTemplate(FAT, volumeLabel, testImageFileFullPath.c_str(), true);
  
  CFileImageStream fileStream;
  splitFilePath = testImageFileFullPath;
  cb.GetFileName(0, splitFilePath);
  // wstring splitFileName = testImageFileFullPath;
  fileStream.Open(splitFilePath.c_str(), IImageStream::forReading);
  fileStream.ReadImageFileHeader(false);
  fileCount = fileStream.GetImageFileHeader().GetFileCount(); 
  if (fileCount==0)
    fileCount = 1;
  fileStream.Close();

  for (unsigned i=0; i<fileCount;i++) {
    splitFilePath = testImageFileFullPath;
    CSplitManagerCallback cb;
    cb.GetFileName(i, splitFilePath);
    BOOL ok = ::DeleteFile(splitFilePath.c_str());
    CPPUNIT_ASSERT_MESSAGE("Failed to delete image file after backup/restore", ok);
  }
  cout << "...done." << endl;
}
// Helper methods:

void ODINManagerTest::ODINManagerTestTemplate(TFileSystem fileSystem, LPCWSTR volumeLabel, LPCWSTR imageFile, bool splitFiles)
{
  // Each tests runs in the following pattern
  // Step 1: Format test drive
  // Step 2: Copy test files to drive
  // Step 3: Delete parts of the test files to create some holes in the area of used blocks)
  // Step 4: Calculate CRC32 check sums of all files and store values
  // Step 5: Make a backup image of the image to image save dir
  // Step 6: Format drive (no quick format, be sure that all blocks are deleted)
  // Step 7: restore image from step 5 to drive
  // Step 8: Calculate CRC32 check sums of all files and store values
  // Step 9: Compare CRC values from step 8 with those from step 4
  //         if both list have same number of values and all values are equal test has passed, otherwise test failed

  // for testing backup/restore of a hard disk we would need to create two volumes and repeat steps 1-4
  // and 8-9 for both volumes

  // helper functions:
  // FormatDrive(), CopyTestFiles(), DeleteSomeFiles(), CalcRecursiveCRCs(), BackupDrive(), RestoreDrive()
  // values taken from configuration file:
  // TestFileDir, ImageDrive, ImageSaveDir


  const CDriveList* driveList = fMgr.GetDriveList();
  wstring imageRootPath = fImageDrive() + L"\\";
  // Step 1:
  wcout << L"Formatting test drive before test begin: " << fImageDrive().c_str() << endl;
FormatTestDrive(fileSystem, volumeLabel);
//DeleteAllFiles(fImageDrive().c_str());
  fMgr.RefreshDriveList(); // after reformatting cluster size might have changed!
  int index = driveList->GetIndexOfDrive(fImageDrive().c_str());
  CPPUNIT_ASSERT_MESSAGE("The configured test drive can not be found", index>=0);
  // Step 2:
  wcout << L"Copying test files from " << fTestDataDir().c_str() << endl;
  CopyTestFiles();
  // Step 3:
  wcout << L"Deleting some test files in " << fImageDrive().c_str() << endl;
  DeleteSomeFiles(fImageDrive().c_str());
  // Step 4:
  wcout << L"Calculating CRC32 checksums before creating image." << endl;
  CalcCrc32(imageRootPath.c_str(), fSrcCrc32);
  // Step 5:
  //Sleep(fSleepTime()); // otherwise we get a share conflict, seems that some files are not yet closed
  wcout << L"Create volume image from " << fImageDrive().c_str() << L" and save in " << imageFile << endl;
  CreateDriveImage(index, imageFile, splitFiles);
  // Step 6:
  wcout << L"Formatting test drive before restoring image: " << fImageDrive().c_str() << endl;
//  FormatTestDrive(fileSystem, volumeLabel);
DeleteAllFiles(fImageDrive().c_str());
  // Step 7:
  Sleep(fSleepTime); // otherwise we get a share conflict, seems that some files are not yet closed
  wcout << L"Restore image from: " << imageFile << endl;

  unsigned fileCount = 0;  
  unsigned __int64 fileSize = 0;
  if (splitFiles) {
    CSplitManagerCallback cb;
    CFileImageStream fileStream;
    wstring testImageFileFullPath(imageFile);
    cb.GetFileName(0, testImageFileFullPath);
    fileStream.Open(testImageFileFullPath.c_str(), IImageStream::forReading);
    fileStream.ReadImageFileHeader(false);
    fileCount = fileStream.GetImageFileHeader().GetFileCount();  
    fileSize = fileStream.GetImageFileHeader().GetFileSize();
    fileStream.Close();
    RestoreDriveImage(index, testImageFileFullPath.c_str(), fileCount, fileSize);
  }
  else
    RestoreDriveImage(index, imageFile, fileCount, fileSize);
  // Step 8:
  wcout << L"Calculating CRC32 checksums after restoring image." << endl;
  CalcCrc32(imageRootPath.c_str(), fDestCrc32);
  // Step 9:
  // compare CRC values and check that they are the same
  wcout << L"Comparing results." << endl;
  CPPUNIT_ASSERT(fDestCrc32.size() == fSrcCrc32.size());
  vector <DWORD>::iterator iter1;
  vector <DWORD>::iterator iter2;
  for ( iter1 = fSrcCrc32.begin(), iter2 = fDestCrc32.begin() ; iter1 != fSrcCrc32.end() && iter2 !=fDestCrc32.end() ; 
    iter1++, iter2++) {
    CPPUNIT_ASSERT(*iter1 == *iter2);
  }
  wcout << L"...checksums of all files are equal to original." << endl;
  wcout << endl << endl;
}

void ODINManagerTest::SetBackupOptions(TCompressionFormat mode, bool useOnlyUsedBlocks, unsigned __int64 splitSize)
{
  fMgr.SetCompressionMode(mode);
  fMgr.SetSaveOnlyUsedBlocksOption(useOnlyUsedBlocks);
  fMgr.SetSplitSize(splitSize);
}

/*
void ODINManagerTest::CalcCrc32Old(LPCWSTR rootDir, vector<DWORD>& crc)
{
  CFileInfoArray fia;
  
  crc.clear();  
  fia.AddDir(
     rootDir,               // Directory (use trailing \)
     L"*.*",                                     // Filemask (all files)
     TRUE,                                       // Recurse subdirs
     CFileInfoArray::AP_SORTBYNAME | CFileInfoArray::AP_SORTASCENDING, // Sort by name and ascending
     FALSE                                       // Do not add array entries for directories (only for files)
  );
  wcout << L"CalcCrc32(), found " << fia.GetSize() << L" file(s)." << endl;
  for (size_t i=0;i<fia.GetSize();i++) {
    CFileInfo fi = fia.GetAt(i);
    DWORD dw = fi.GetCRC();
    wcout << (LPCWSTR)(fi.GetFilePath()) << L": " << dw << endl;
    crc.push_back(dw);
  }
}
*/
bool ODINManagerTest::CalcCrc32(LPCWSTR startDir, vector<DWORD>& crc)
{
  // we recursively traverse the directories and simply delete every 5th file
  int i=0;
  WIN32_FIND_DATA fd;
  HANDLE h;
  bool ok = true;
  wstring rootDir(startDir);

  if (rootDir.find(L"System Volume Information" ) != string::npos)
    return true; // ignore windows dir System Volume Information
  rootDir += L"\\*";
  h = FindFirstFile(rootDir.c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) 
  {
    do {
      wstring absFilePath(startDir);
      absFilePath += L"\\";
      absFilePath += fd.cFileName;
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && fd.cFileName[0] != L'.') {
        CalcCrc32(absFilePath.c_str(), crc);
      } else {
          HANDLE hInput;
          DWORD dwBytesToRead = 1024 * 1024;
          DWORD dwBytesRead;
          BYTE *buffer;
          BOOL bReadSuccess;
          CCRC32 crc32;

          hInput = CreateFile(absFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
          if (INVALID_HANDLE_VALUE == hInput)
            return false;
          buffer = new BYTE[dwBytesToRead];
          do
          {
             bReadSuccess = ReadFile(hInput, buffer, dwBytesToRead, &dwBytesRead, NULL);	
             crc32.AddDataBlock(buffer, dwBytesRead);
          } while(bReadSuccess && dwBytesRead == dwBytesToRead);
          delete buffer;
          ok = CloseHandle(hInput) != 0;
          ok = ok && bReadSuccess;
          //wcout << L"calc crc: " << absFilePath.c_str() << L" is: " << crc32.GetResult() << endl;
          crc.push_back(crc32.GetResult());
      }
    } while (FindNextFile(h, &fd) != 0 && ok);
    FindClose(h);
  }
  else
    ok = false;
  return ok;
}

void ODINManagerTest::FormatTestDrive(TFileSystem fileSystem, LPCWSTR volumeLabel)
{
  CDriveFormatter formatter;

  if (! formatter.IsInited()) {
    CPPUNIT_FAIL( "Failed to initialize driveformatting");
  }
  wcout << "Start formatting drive " << fImageDrive().c_str() << L"..." << endl;  
  bool ok = formatter.FormatDrive(fImageDrive().c_str(), fileSystem, L"ODINTEST", FALSE);
  ok = !formatter.WasError();
  wcout << L"  ... done. Result " << (ok ? L"OK" : L"Failed") << endl; 
  CPPUNIT_ASSERT(ok);
}

void ODINManagerTest::CopyTestFiles() {
/*
  SHFILEOPSTRUCT shf;
  wchar_t srcPath[MAX_PATH+1];
  wchar_t destPath[MAX_PATH+1];

  // append second requires \0 char at end of source path
  memset(srcPath, 0, MAX_PATH);
  wcscpy_s(srcPath, MAX_PATH, fTestDataDir().c_str());
  srcPath[fTestDataDir().length()+1] = L'\0';
  memset(destPath, 0, MAX_PATH);
  wcscpy_s(destPath, MAX_PATH, fImageDrive().c_str());
  destPath[fImageDrive().length()+1] = L'\0';

  memset(&shf, 0, sizeof(shf));
  shf.hwnd = NULL;
  shf.wFunc = FO_COPY;
  shf.pFrom = srcPath;
  shf.pTo = destPath;
  shf.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_SILENT;
  shf.fAnyOperationsAborted = FALSE;
  shf.hNameMappings = NULL;

  int res = SHFileOperation(&shf);
  wcout << L"Copy from " << fTestDataDir().c_str() << " to " << fImageDrive().c_str() << " was " << (res==0 ? L"successful" : L"not successful") << endl;
  if (res)
    wcout << L"  Error code is: " << res << endl;
  CPPUNIT_ASSERT(res==0);
*/

  
  //new 
  // we recursively traverse the directories and simply delete every 5th file
  int i=0;
  WIN32_FIND_DATA fd;
  HANDLE h;
  wstring rootDir(fTestDataDir);
  if (rootDir.find(L"System Volume Information" ) != string::npos)
    return; // ignore windows dir System Volume Information
  rootDir += L"\\*";
  h = FindFirstFile(rootDir.c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) 
  {
    do {
      wstring absFilePath(fTestDataDir);
      wstring destFilePath(fImageDrive);
      absFilePath += L"\\";
      absFilePath += fd.cFileName;
      destFilePath += L"\\";
      destFilePath += fd.cFileName;

      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && fd.cFileName[0] != L'.') {
        ; // copy recursively
      } else if (fd.cFileName[0] != L'.') {
        // we could use the API function but for some reason files seem to stay open, so we use our own function
        //BOOL ok= ::CopyFile(absFilePath.c_str(), destFilePath.c_str(), FALSE);
        bool ok = CopyFile(absFilePath.c_str(), destFilePath.c_str());
        wcout << L"copying test file " << absFilePath.c_str() << L" to " << destFilePath.c_str() << endl;
        CPPUNIT_ASSERT_MESSAGE("copying test file failed", ok);
      }
    } while (FindNextFile(h, &fd) != 0);
    FindClose(h);
  }
}

void ODINManagerTest::DeleteSomeFiles(LPCWSTR startDir)
{
  // we recursively traverse the directories and simply delete every 5th file
  int i=0;
  WIN32_FIND_DATA fd;
  HANDLE h;
  wstring rootDir(startDir);
  if (rootDir.find(L"System Volume Information" ) != string::npos)
    return; // ignore windows dir System Volume Information
  rootDir += L"\\*";
  h = FindFirstFile(rootDir.c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) 
  {
    do {
      wstring absFilePath(startDir);
      absFilePath += L"\\";
      absFilePath += fd.cFileName;
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && fd.cFileName[0] != L'.') {
        DeleteSomeFiles(absFilePath.c_str());
      } else {
        if ( i%5 == 4) {
          wcout << L"Deleting test file " << absFilePath.c_str() << endl;
          BOOL ok = DeleteFile(absFilePath.c_str());
          CPPUNIT_ASSERT_MESSAGE("Deleting test file failed", ok);
        }
        ++i;
      }
    } while (FindNextFile(h, &fd) != 0);
    FindClose(h);
  }
}

void ODINManagerTest::DeleteAllFiles(LPCWSTR startDir)
{
  // we recursively traverse the directories and simply delete every 5th file
  int i=0;
  WIN32_FIND_DATA fd;
  HANDLE h;
  wstring rootDir(startDir);
  rootDir += L"\\*";
  h = FindFirstFile(rootDir.c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) 
  {
    do {
      wstring absFilePath(startDir);
      absFilePath += L"\\";
      absFilePath += fd.cFileName;
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && fd.cFileName[0] != L'.') {
        DeleteSomeFiles(absFilePath.c_str());
      } else {
          // wcout << L"Deleting test file " << absFilePath.c_str() << endl;
          BOOL ok = DeleteFile(absFilePath.c_str());
          CPPUNIT_ASSERT_MESSAGE("Deleting test file failed", ok);
      }
    } while (FindNextFile(h, &fd) != 0);
    FindClose(h);
  }
}

void ODINManagerTest::CreateDriveImage(int driveListIndex, LPCWSTR imageFileName, bool splitFiles)
{
  CSplitManagerCallback cb;

  try {
    fMgr.SavePartition(driveListIndex, imageFileName, splitFiles ? &cb : NULL);
    WaitUntilDone();
  } catch (Exception& e) {
    wstring msg = L"CreateDriveImage failed with exception: ";
    msg += e.GetMessage();
    fMgr.Terminate();
    wcout << msg.c_str() << endl;
    CPPUNIT_FAIL("CreateDriveImage failed with exception");
  }
}

void ODINManagerTest::RestoreDriveImage(int driveListIndex, LPCWSTR imageFileName, unsigned fileCount, __int64 fileSize)
{
  CSplitManagerCallback cb;

  try {
    fMgr.RestorePartition(imageFileName, driveListIndex, fileCount, fileSize, fileCount>0 ? &cb : NULL);
    WaitUntilDone();
  } catch (Exception& e) {
    wstring msg = L"RestoreDriveImage failed with exception: ";
    msg += e.GetMessage();
    fMgr.Terminate();
    wcout << msg.c_str() << endl;
    CPPUNIT_FAIL("RestoreDriveImage failed with exception");
  }
}

void ODINManagerTest::CreateFullPath(const std::wstring& fileName, std::wstring& fullPathName)
{
  fullPathName = fImageSaveDir();
  if (fullPathName[fullPathName.length()-1] != L'\\')
    fullPathName += L"\\";
  fullPathName += fileName;
}

void ODINManagerTest::WaitUntilDone() 
{
  // wait until threads are completed without blocking the user interface
  wcout << L"Operation started worker threads running..." << endl;
  unsigned threadCount = fMgr.GetThreadCount();
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  bool ok = fMgr.GetThreadHandles(threadHandleArray, threadCount);
  if (!ok)
    return;

  while (TRUE) {
    DWORD result = MsgWaitForMultipleObjects(threadCount, threadHandleArray, FALSE, INFINITE, QS_ALLEVENTS);
    if (result >= WAIT_OBJECT_0 && result < (DWORD)threadCount) {
      if (--threadCount == 0)  {
        wcout << L"...Operation completed all worker threads terminated" << endl;
        fMgr.Terminate(); // work is finished
        break;
      }
      // setup new array with the remaining threads:
      for (unsigned i=result; i<threadCount; i++)
        threadHandleArray[i] = threadHandleArray[i+1];
    }
    else if (result  == WAIT_OBJECT_0 + threadCount)
    {
      MSG msg ;
      while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        ::DispatchMessage(&msg) ;
    }
    else if ( WAIT_FAILED) {
          CPPUNIT_FAIL("MsgWaitForMultipleObjects failed");
      break;
    }
    else
      CPPUNIT_FAIL("unusual return code from MsgWaitForMultipleObjects");
  }
  delete [] threadHandleArray;
}

bool ODINManagerTest::CopyFile(LPCWSTR srcPath, LPCWSTR destPath)
{
  HANDLE hInput, hOutput;
  DWORD dwBytesToRead = 1024 * 1024;
  DWORD dwBytesRead, dwBytesWritten;
  BYTE *buffer;
  BOOL ok, bReadSuccess, bWriteSuccess;

  hInput = CreateFile(srcPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  hOutput= CreateFile(destPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 

  if (INVALID_HANDLE_VALUE == hInput || INVALID_HANDLE_VALUE == hOutput)
    return false;

  buffer = new BYTE[dwBytesToRead];
  do
  {
     bReadSuccess = ReadFile(hInput, buffer, dwBytesToRead, &dwBytesRead, NULL);	
     bWriteSuccess= WriteFile(hOutput, buffer, dwBytesRead, &dwBytesWritten, NULL); 

  } while(bReadSuccess && bWriteSuccess && dwBytesRead == dwBytesToRead && dwBytesRead == dwBytesWritten);

  delete buffer;

  ok = CloseHandle(hOutput);
  ok = CloseHandle(hInput);
  return bReadSuccess && bWriteSuccess;
 }

//======================================================================
//
// class CDriveFormatter code taken from Mark Russinovichs FormatX
// utilitly from sysinternals (fmifs.zip)
//
//======================================================================

BOOL CDriveFormatter::sError = FALSE;

// media flags
#define FMIFS_HARDDISK 0xC
#define FMIFS_FLOPPY   0x8

//----------------------------------------------------------------------
//
// FormatExCallback
//
// The file system library will call us back with commands that we
// can interpret. If we wanted to halt the chkdsk we could return FALSE.
//
//----------------------------------------------------------------------
BOOLEAN __stdcall CDriveFormatter::FormatExCallback( CALLBACKCOMMAND Command, DWORD Modifier, PVOID Argument )
{
	PDWORD percent;
  PTEXTOUTPUT output;
	PBOOLEAN status;
	static int createStructures = FALSE;
  
	// 
	// We get other types of commands, but we don't have to pay attention to them
	//
	switch( Command ) {

	case PROGRESS:
		percent = (PDWORD) Argument;
		_tprintf(L"%d percent completed.\r", *percent);
		break;

	case OUTPUT:
		output = (CDriveFormatter::PTEXTOUTPUT) Argument;
		fprintf(stdout, "%s", output->Output);
		break;

	case DONE:
		status = (PBOOLEAN) Argument;
		if( *status == FALSE ) {

			_tprintf(L"FormatEx was unable to complete successfully.\n\n");
			sError = TRUE;
		}
		break;
	}
	return TRUE;
}

CDriveFormatter::CDriveFormatter() {
  sError = FALSE;
  fLib = LoadLibrary( L"fmifs.dll" );
  if (fLib) {
	  if( !(fFormatEx = (PFORMATEX) GetProcAddress( GetModuleHandle( L"fmifs.dll"), "FormatEx" )) ) {
		  FreeLibrary(fLib);
      fLib = NULL;
	  }
  }
}

CDriveFormatter::~CDriveFormatter() {
  if (fLib != NULL)
    FreeLibrary(fLib);
}


bool CDriveFormatter::FormatDrive(LPCWSTR drive, TFileSystem fileSystem, LPCWSTR label, BOOL quickFormat) {
	DWORD mediaFlag = FMIFS_HARDDISK; // floppy not yet supported
  LPCWSTR fsFormat;
  DWORD clusterSize = 0; // default
  sError = 0;

  switch (fileSystem) {
    case FAT:
     fsFormat = L"FAT";
     break;
    case FAT32:
     fsFormat = L"FAT32";
     break;
    case NTFS:
     fsFormat = L"NTFS";
     break;
   // new file system "EXFAT" seems not to be supported :(
  }

  DismountVolume(drive);
  fFormatEx( drive, mediaFlag, fsFormat, label, quickFormat, clusterSize, FormatExCallback );
	if( sError ) 
    return false;
  DismountVolume(drive);
	return true;
}

void CDriveFormatter::DismountVolume(LPCWSTR drive) {
  wstring volName = L"\\\\.\\";
  volName += drive;
  BOOL res;
  DWORD dummy;
  bool hasLocked = true;
  HANDLE hVolume;

  hVolume = CreateFile(volName.c_str(), GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hVolume == INVALID_HANDLE_VALUE) {
    wcout << L"Failed to open drive " << drive << endl;
    return;
  }

  res = DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
  if (res == 0) {
    wcout << L"Warning: Failed to lock volume " << volName.c_str() << L" error is: " << GetLastError() << endl;
    wcout << L"         Forcing now dismount " << endl;
    hasLocked = false;
    // locking failed so we try a hard dismount and lock again
  }

  res = DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
  if (res == 0) {
    wcout << L"Failed to dismount volume "  << L" error is: " << GetLastError() << endl;
  }

  if (hasLocked)
    res = DeviceIoControl(hVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);
    if (res == 0) {
      wcout << L"Failed to unlock volume."  << L" error is: " << GetLastError() << endl;
    }

  res = CloseHandle(hVolume);
  if (res == 0) {
    wcout << L"Failed to close volume handle."  << volName.c_str() << L" error is: " << GetLastError() << endl;
  }
}
