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

#include "cppunit/extensions/HelperMacros.h"
#include "..\..\src\ODIN\Config.h"

typedef enum { FAT, FAT32, NTFS} TFileSystem;

/*
  ODINManager test is bit more effort and is not a typical unit test.
  To really test the end user functionality we need to prepare a test
  environment with appropriate disk volumes that can be backuped and
  restored. Those are typically contained in a virtual machine. To 
  maintain this test setup we need a configuration file (ini file),
  a set of test files and folders and routines to prepare the test
  data
*/
class InitClass
{
public:
  InitClass(LPCWSTR fileName);
};

class ODINManagerTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( ODINManagerTest );
  /**/
  CPPUNIT_TEST( ODINManagerTest0 );
  CPPUNIT_TEST( ODINManagerTest1 );  
  CPPUNIT_TEST( ODINManagerTest2 );
  CPPUNIT_TEST( ODINManagerTest3 );
  CPPUNIT_TEST( ODINManagerTest4 );
  CPPUNIT_TEST( ODINManagerTest5 );
  CPPUNIT_TEST( ODINManagerSplitTest );
  /**/
  CPPUNIT_TEST_SUITE_END();

public:
  ODINManagerTest();
  void setUp();
  void tearDown();

  void ODINManagerTest0();
  void ODINManagerTest1();
  void ODINManagerTest2();
  void ODINManagerTest3();
  void ODINManagerTest4();
  void ODINManagerTest5();
  void ODINManagerSplitTest();

private:
  void ODINManagerTestTemplate(TFileSystem fileSystem, LPCWSTR volumeLabel, LPCWSTR imageFile, bool splitFiles);
  void SetBackupOptions(TCompressionFormat mode, bool useOnlyUsedBlocks, unsigned __int64 splitSize); 
  void CopyTestFiles();
  void FormatTestDrive(TFileSystem fileSystem, LPCWSTR volumeLabel);
  void DeleteSomeFiles(LPCWSTR startDir);
  // void CalcCrc32Old(LPCWSTR rootDir, std::vector<DWORD>& crc);
  void CreateDriveImage(int driveListIndex, LPCWSTR imageFileName, bool splitFiles);
  void RestoreDriveImage(int driveListIndex, LPCWSTR imageFileName, unsigned fileCount, __int64 fileSize);
  void WaitUntilDone();
  void CreateFullPath(const std::wstring& fileName, std::wstring& fullPathName);
  bool CopyFile(LPCWSTR srcPath, LPCWSTR destPath);
  void DeleteAllFiles(LPCWSTR startDir);
  bool CalcCrc32(LPCWSTR startDir, std::vector<DWORD>& crc);

private:
  InitClass dummyInit;
  COdinManager fMgr;
  std::vector<DWORD> fSrcCrc32;
  std::vector<DWORD> fDestCrc32;
  TCompressionFormat fOldCompressionMode;
  bool fOldUseOnlyUsedBlocks;
  unsigned __int64 fOldSplitSize;

  DECLARE_SECTION()
  DECLARE_ENTRY(bool, fEnableTest)
  DECLARE_ENTRY(std::wstring, fTestDataDir) // path to directory containing sample files to backup
  DECLARE_ENTRY(std::wstring, fImageDrive)
  DECLARE_ENTRY(std::wstring, fImageSaveDir)
  DECLARE_ENTRY(int, fSleepTime)
};

//====================================================================
//
// class CDriveFormatter: a utility class for formatting a drive
//
//
class CDriveFormatter {
public:
  
  CDriveFormatter();
  ~CDriveFormatter();

  // true if object is ready to use, false if object is not able to format
  bool IsInited() const {
    return fLib != INVALID_HANDLE_VALUE;
  }
  
  // true if error occured, false if last operation was successfuk
  bool WasError() const {
    return sError != FALSE;
  }

  bool FormatDrive(LPCWSTR drive, TFileSystem fileSystem, LPCWSTR label, BOOL quickFormat=FALSE);

private:
  void CDriveFormatter::DismountVolume(LPCWSTR drive);
  typedef enum {
    PROGRESS,
    DONEWITHSTRUCTURE,
    UNKNOWN2,
    UNKNOWN3,
    UNKNOWN4,
    UNKNOWN5,
    INSUFFICIENTRIGHTS,
    UNKNOWN7,
    UNKNOWN8,
    UNKNOWN9,
    UNKNOWNA,
    DONE,
    UNKNOWNC,
    UNKNOWND,
    OUTPUT,
    STRUCTUREPROGRESS
  } CALLBACKCOMMAND;

  typedef struct {
	  DWORD Lines;
	  PCHAR Output;
  } TEXTOUTPUT, *PTEXTOUTPUT;
  static BOOL	sError;



  typedef BOOLEAN (__stdcall *PFMIFSCALLBACK)( CALLBACKCOMMAND Command, DWORD SubAction, PVOID ActionInfo ); 
  typedef VOID (__stdcall *PFORMATEX)( LPCWSTR DriveRoot,
						  DWORD MediaFlag,
						  LPCWSTR Format,
						  LPCWSTR Label,
						  BOOL QuickFormat,
						  DWORD ClusterSize,
						  PFMIFSCALLBACK Callback );
  HMODULE     fLib;
  PFORMATEX   fFormatEx;
  static BOOLEAN __stdcall FormatExCallback( CALLBACKCOMMAND Command, DWORD Modifier, PVOID Argument );

};