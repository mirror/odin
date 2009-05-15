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
 
/////////////////////////////////////////////////////////////////////////////
//
// Class that parses the command line and processes the given instructions
//
///////////////////////////////////////////////////////////////////////////// 

#pragma once
#include "compression.h"
#include <string>

class COdinManager;
class CConsoleSplitManagerCallback;
class CUserFeedbackConsole;
template <class T> class CStlCmdLineArgsWin;

class CCommandLineProcessor: public IWaitCallback {

public:
  typedef enum { CmdBackup, CmdRestore, CmdVerify, CmdList, CmdUsage } TOdinCommand; 
  typedef enum { modeOnlyUsedBlocks, modeUsedBlocksAndSnapshot, modeAllBlocks } TBackupMode;

  typedef struct {
	  TOdinCommand cmd;
      std::wstring source;
      std::wstring target;
      std::wstring comment;
      int sourceIndex;
      int targetIndex;
	  int splitSizeMB;
      TBackupMode mode; 
	  TCompressionFormat compression;
	  bool force;
  } TOdinOperation;

  CCommandLineProcessor();
  ~CCommandLineProcessor();
  
  int ParseAndProcess();

  bool InitConsole(bool createConsole);
  void ReportFeedback();

private:
  // Parse the command line fill fOperation member
  // return true to continue with program operation or false to stop
  void Init();
  void Parse();
  void Parse(const wchar_t* commandLine); // for unit tests
  void Parse(CStlCmdLineArgsWin<wchar_t>& cmdLineParser);

  void PrintUsage();
  void ProcessCommandLine();
  void TerminateConsole();
  void ListDrives(); 
  void PreprocessSourceAndTarget(const std::wstring& name, bool sourceOrTarget);
  void CheckValidParameters();
  void Reset();
  virtual void OnThreadTerminated();
  virtual void OnFinished();
  virtual void OnAbort();
  virtual void OnPartitionChange(int i, int n);
  virtual void OnPrepareSnapshotBegin();
  virtual void OnPrepareSnapshotReady();
  CStlCmdLineArgsWin<wchar_t>* fCmdLineParser;
  FILE *fpIn, *fpOut, *fpErr;
  bool fHasConsole;
  bool fConsoleCreated;
  bool fVerifyRun;
  TOdinOperation fOperation; // structure that contains the command and parameters
  COdinManager* fOdinManager;
  HANDLE fTimer;
  int fLastPercent;
  DWORD fCrc32;
  int fExitCode;
  CConsoleSplitManagerCallback* fSplitCB;
  CUserFeedbackConsole* fFeedback;
  friend class CCmdLineTest;
};