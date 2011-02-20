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

#include "stdafx.h"
#include "OdinManager.h"
#include "CommandLineProcessor.h"
#include "CmdLineParser.h"
#include "OdinManager.h"
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <crtdbg.h>
#include "CmdLineException.h"
#include "DriveList.h"
#include "UserFeedbackConsole.h"
#include "util.h"
#include "SplitManager.h"
#include "MultiPartitionHandler.h"
#include "ParamChecker.h"

using namespace std;

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

DWORD WINAPI ODINTimerThread ( LPVOID pData) 
{ 
  Sleep(2000);
  for (;;) {
    CCommandLineProcessor *pCmd = (CCommandLineProcessor *)pData;
    pCmd->ReportFeedback();
    Sleep(1000);
  }
  return 0;
}


// Here the description of what the command line parser can do:
// from http://www.codeproject.com/KB/cpp/cma.aspx

/**************************************************************************
Description
Like in WScript.arguments the option names and its values have to be separated with 
':' (but unlike WSH, it does not have to be : but any character of your choice):

/option1:its_value

or

/option2=its_value

etc., but not with spaces or tabs. This is not supported:

argument1 argument2 /option1 its_value /option2 its_value

Just like in WSH, the options may be what we could call simple, i.e. without value:

/option1 /option2

The options (named arguments) can be given in any order and can be mixed with (unnamed) 
arguments. Long options (i.e. the ones beginning with 2 or more characters, like with --)
are not supported. Like WScript.arguments, this parser in the time of its instantiation 
does not demand any information about desired command line arguments. That's why I added 
keeping track of named arguments usage. Any time you can retrieve the number or named 
arguments not yet accessed, and the list of them. It is actually quite a natural approach. 
Why would we have to tell the parser what options we expect when it can conclude that 
itself by watching us using that option in our code??? It serves as some kind of 
additional debugging information too. We are no more warned only when the code is trying 
to use some options that are not given on the command line, but the other way round too.
I hope people will find this approach handy and interesting.
It's other most important characteristics:

    * It's a template class that can be used with char or wchar_t.
    * When used with whar_t, it works flawlessly with Unicode characters, I test it all 
      the time. (Printing them properly to the Windows console is another matter. One way 
      to do it is to print directly to console using functions from conio.h like _tcprintf.
      For the other way, see my article on this matter Unicode output to the Windows console.) 
    * If you compile it on Linux, you should of course use char. (Thanks to the consistent 
      usage of UTF-8, Linux has no problems with Unicode characters so, if you want to print 
      them out to the console, simply using printf or cout will do the job.)
    * It is so similar to WScript.arguments that, if you have any experience with
      WScript.arguments, you can start using it right away. There are some additional features 
      (keeping track of named arguments usage), but you can study them if and when you will 
      need them.   
    * It can be used on Windows non-console applications, i.e. those that have _tWinMain 
      (i.e. wWinMain or WinMain) function instead of _tmain (i.e. wmain or main) function.
      Can be instantiated anywhere in the program, not only in _tmain function.
    * You can choose whether the options should begin with '/' or '-', or any other character.
    * You can choose whether the options names should be separated from their values with 
      ':' or '=', or any other character.

Usage

Note: I explained everything as you would use TCHAR. (_TCHAR and TCHAR are the same thing. 
I sometimes write TCHAR and sometimes _TCHAR. I apologize for confusing you by this.) 
Therefore strings are written as _T("foo"). But you can of course use char or wchar_t 
directly. In that case, string literals would be written as "foo" and L"foo" respectively.
You will probably first try to use the parser inside of the example project. To use it in 
your project, you will of course have to put my header files into your project folder or 
somewhere in the include path of your compiler. 
Instantiating

There are 3 classes:

    * CStlCmdLineArgs
    * CArgList
    * CStlCmdLineArgsWin

CStlCmdLineArgs is the very parser. It needs argc and argv to be passed as arguments in the 
constructor. CArgList creates that argument using the Windows function GetCommandLine and 
CStlCmdLineArgsWin connects them by inheritance and containment respectively, so that you 
can instantiate the parser in the most easy way. Having included CStlCmdLineArgsWin_4.h: 

CStlCmdLineArgsWin<TCHAR> cma;  

Since you are not dependent on argv and argc, you can instantiate it this way easily anywhere 
in the code. It is the most recommended way for Windows applications.

On Linux, CArgList makes no sense so you must instantiate CStlCmdLineArgs directly using argc 
and argv. This approach can also be used in Windows command line applications. On Linux, the template 
argument should be char instead of TCHAR. Having included CStlCmdLineArgs4.h:  

CStlCmdLineArgs<TCHAR> cma(argc, argv); 

The Very Basic Usage

You use them just like WScript.arguments. The only differences are that in WScript.arguments, you 
would use round brackets and here you use square brackets:

You can access plain arguments (i.e., unnamed arguments) with:  

cma.unnamed[0]  
cma.unnamed[1]  
etc.

... and that, unlike WScript.arguments.unnamed, cma.unnamed starts with the 0th command line argument 
so that cma.unnamed[0] gives you what you would get retrieving argv[0] in the _tmain function.  

If the index is out of bounds, i.e. if you try to access more arguments than present on the command 
line, it will simply return NULL.
You can access options (i.e., named arguments) with: 

cma.named[_T("name_of_your_option")]

If the particular option is not present on the command line, it will return NULL.

If the option is given without a value, it will return empty string. This is how you check if a 
"simple" option, i.e. an option that does not require a value is present.
You Can AlsoSee how many unnamed arguments were on the command line with (something you will 
need in every program): 

cma.unnamed.size()

See how many named arguments were on the command line with (something you will probably never use ;): 

cma.named.size() 

Advanced Use - If You Want to Give the User More Precise Error Messages

This is something WScript.arguments hasn't got. The class keeps track of named arguments you access.
The information about arguments that were present on the command line and are not accessed can be
retrieved with: 

cma.unusednamed.size() 

That will give you their number, and...

cma.unusednamed.toString()  

... that will give you all of them in a single string, quoted and separated by spaces.
There is More

Till now, we have treated command line options like in Windows Script: they all begin with '/' and ':' 
separates values from names. This is the default. But class constructors accept additional arguments, 
so that we can for example do this: 

CStlCmdLineArgsWin<TCHAR> cma('-', '='); 

Now the command line options are expected to begin with '-' and '=' will separate its names from 
its values. Let's say you have chosen the default and you need to have an unnamed argument 
beginning with '/', say /arg1, you can escape it like this: /:/arg1. If you have chosen '-' and 
have an argument beginning with '-', you would escape it like this: -:-arg1.

**************************************************************************************************/
/////////////////////////////////////////////////////////////////////////////
//
// class CODINSplitManagerCallback
//
class CConsoleSplitManagerCallback : public ISplitManagerCallback 
{
public:
  CConsoleSplitManagerCallback()
  {
  }
  virtual void GetFileName(unsigned fileNo, std::wstring& fileName);
  virtual size_t AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, std::wstring& newName);

  void SetBaseName(LPCWSTR baseName)
  {
    fBaseName = baseName;
  }

private:
  wstring fBaseName; // file name patttern where number string is appended to
};


void CConsoleSplitManagerCallback::GetFileName(unsigned fileNo, std::wstring& fileName)
{
  wchar_t buf[10];
  wsprintf(buf, L"%04u", fileNo);

  size_t lastDotPos = fileName.rfind(L'.');
  if (lastDotPos < 0)
    fileName += buf;
  else {
    wstring ext = fileName.substr(lastDotPos); // get extension
    fileName = fileName.substr(0, lastDotPos);
    fileName += buf;
    fileName += ext;
  }
}



size_t CConsoleSplitManagerCallback::AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, wstring& newName)
{
  wcout  << L"Please provide path to file number " << fileNo;
  wcin >> newName;
  return newName.length();
}



CCommandLineProcessor::CCommandLineProcessor() {
  // _CrtSetBreakAlloc(186);
  fHasConsole = false;
  fConsoleCreated = false;
  fFeedback = NULL;
  Reset();
  fOdinManager = NULL;
  fSplitCB = new CConsoleSplitManagerCallback();
  fVerifyRun = false;
  fCrc32 = 0;
  fExitCode = 0;
  fpIn = fpOut = fpErr = NULL;
}

CCommandLineProcessor::~CCommandLineProcessor() {
  delete fOdinManager;
  delete fSplitCB;
  delete fFeedback;
  TerminateConsole();
}

int CCommandLineProcessor::ParseAndProcess() {
  try {
    Parse();
    ProcessCommandLine();
    return fExitCode;
  } catch (Exception& e) {
    OnAbort();
    wcout << e.GetMessage() << endl; 
    return -1;
  } catch (...) {
    OnAbort();
    wcout << L"Internal runtime error occured." << endl; 
    return -1;
  }
}

void CCommandLineProcessor::Init() {
  if (fOdinManager == NULL) {
    fOdinManager = new COdinManager();
    fOdinManager->RefreshDriveList();
  }
}

// Parse the command line fill fOperation member
// return true to continue with program operation or false to stop
void CCommandLineProcessor::Parse() {
  // create command line parser, we take - as option prefix and = as separator in -option=value
  CStlCmdLineArgsWin<wchar_t> cmdLineParser('-', '='); 
  Parse(cmdLineParser);
}

void CCommandLineProcessor::Parse(const wchar_t* commandLine) {
  CStlCmdLineArgsWin<wchar_t> cmdLineParser(commandLine, '-', '='); 
  Parse(cmdLineParser);
}

void CCommandLineProcessor::Parse(CStlCmdLineArgsWin<wchar_t>& cmdLineParser) {
  // if usage was requested print usage
  if (cmdLineParser[L"usage"] != NULL) {
     fOperation.cmd = CCommandLineProcessor::CmdUsage;
     return;
  }

  // parse main operation to perform
  if (cmdLineParser[L"backup"] != NULL)
    fOperation.cmd = CCommandLineProcessor::CmdBackup;
  else if (cmdLineParser[L"restore"] != NULL)
    fOperation.cmd = CCommandLineProcessor::CmdRestore;
  else if (cmdLineParser[L"verify"] != NULL)
    fOperation.cmd = CCommandLineProcessor::CmdVerify;
  else if (cmdLineParser[L"list"] != NULL)
    fOperation.cmd = CCommandLineProcessor::CmdList;
  else {
    THROW_CMD_EXC(ECmdLineException::noOperation);
  }

  // check for backup mode options -makeSnapshot -allBlocks -usedBlocks
  // set default first
  fOperation.mode=CCommandLineProcessor::modeOnlyUsedBlocks;
  if (cmdLineParser[L"usedBlocks"] != NULL)
    fOperation.mode = CCommandLineProcessor::modeOnlyUsedBlocks;
  if (cmdLineParser[L"allBlocks"] != NULL)
    fOperation.mode = CCommandLineProcessor::modeAllBlocks;
  if (cmdLineParser[L"makeSnapshot"] != NULL)
    fOperation.mode = CCommandLineProcessor::modeUsedBlocksAndSnapshot;

  // check for compression mode
  fOperation.compression = compressionGZip;
  wstring comp;
  if (cmdLineParser[L"compression"])
    comp = cmdLineParser[L"compression"];

  if (comp.compare(L"gzip") == 0)
    fOperation.compression = compressionGZip;
  else if (comp.compare(L"bzip") == 0)
    fOperation.compression = compressionBZIP2;
  else if (comp.compare(L"none") == 0)
    fOperation.compression = noCompression;
  else if (!comp.empty())
    THROW_CMD_EXC(ECmdLineException::wrongCompression);

  // split and force options
  fOperation.force = false;
  fOperation.splitSizeMB = 0;
  if (cmdLineParser[L"split"] != NULL)
    fOperation.splitSizeMB = _wtoi(cmdLineParser[L"split"]);
  if (cmdLineParser[L"force"] != NULL)
    fOperation.force = true;

  // source and target options
  if (cmdLineParser[L"source"])
    fOperation.source = cmdLineParser[L"source"];
  if (cmdLineParser[L"target"])
    fOperation.target = cmdLineParser[L"target"];

  if (fOperation.source.empty() && fOperation.cmd != CCommandLineProcessor::CmdList) {
    THROW_CMD_EXC(ECmdLineException::noSource);
  }
  if (fOperation.target.empty() && fOperation.cmd != CCommandLineProcessor::CmdList
    && fOperation.cmd != CCommandLineProcessor::CmdVerify) {
    THROW_CMD_EXC(ECmdLineException::noTarget);
  }
  if (cmdLineParser[L"comment"])
    fOperation.comment = cmdLineParser[L"comment"];

  // check for unknown options
  if (cmdLineParser.unusednamed.size() > 0) {
    THROW_CMD_EXC(ECmdLineException::unknownOption);
  }
}

void CCommandLineProcessor::ProcessCommandLine() {

  Init();
  if (fOperation.cmd == CmdUsage) {
    PrintUsage();
    return;
  } else if (fOperation.cmd == CmdList) {
    ListDrives();
    return;
  }

  // first preprocess source and or target information given
  PreprocessSourceAndTarget(fOperation.source, true);
  if (!fOperation.target.empty())
    PreprocessSourceAndTarget(fOperation.target, false);

  // check for parameter errors
  CheckValidParameters();
  
  fLastPercent = 0;
  fCrc32 = 0;
  fFeedback = new CUserFeedbackConsole(fOperation.force);
  CParamChecker checker(*fFeedback, *fOdinManager);

  // perform operation
  if (fOperation.cmd == CmdBackup) {
      LPCWSTR deviceName = fOdinManager->GetDriveList()->GetItem(fOperation.sourceIndex)->GetDeviceName().c_str();
      wcout << L"Backing up " << deviceName << L" to file: " << fOperation.target.c_str() << endl;
      IUserFeedback::TFeedbackResult res = checker.CheckConditionsForSavePartition(fOperation.target.c_str(), fOperation.sourceIndex);
      if (res == IUserFeedback::TOk || res == IUserFeedback::TYes) {
        fTimer = CreateThread(NULL, 0, ODINTimerThread, this, 0, NULL);
        if (fOperation.comment.length() > 0)
           fOdinManager->SetComment(fOperation.comment.c_str());
        CMultiPartitionHandler::BackupPartitionOrDisk(fOperation.sourceIndex, fOperation.target.c_str(), *fOdinManager, fSplitCB, this, *fFeedback);
      } 
  }
  else if (fOperation.cmd == CmdRestore) {
    unsigned noFiles=0; unsigned __int64 totalSize = 0;
    LPCWSTR deviceName = fOdinManager->GetDriveList()->GetItem(fOperation.targetIndex)->GetDeviceName().c_str();
    wcout << L"Restoring " << deviceName << L" from file: " << fOperation.source.c_str() << endl;
    IUserFeedback::TFeedbackResult res = checker.CheckConditionsForRestorePartition(fOperation.source.c_str(), *fSplitCB, fOperation.targetIndex, noFiles, totalSize);
    if (res == IUserFeedback::TOk || res == IUserFeedback::TYes) {
      // do restore
      fTimer = CreateThread(NULL, 0, ODINTimerThread, this, 0, NULL);
      CMultiPartitionHandler::RestorePartitionOrDisk(fOperation.targetIndex, fOperation.source.c_str(), *fOdinManager, fSplitCB, this);
    }
  }
  else if (fOperation.cmd == CmdVerify) {
    wcout << L"Verifying image file " << fOperation.source.c_str() << endl;
    // do verify
    fVerifyRun = true;
    fCrc32 = 0; //TODO: get from header
    fTimer = CreateThread(NULL, 0, ODINTimerThread, this, 0, NULL);
    CMultiPartitionHandler::VerifyPartitionOrDisk(fOperation.source.c_str(), *fOdinManager, fCrc32, fSplitCB, this, *fFeedback);
  }
  else
    wcerr << L"Internal error illegal program state:  " << __WFILE__ << L" " << __LINE__; // should not happen
}

void CCommandLineProcessor::ListDrives() {

  int noDrives = fOdinManager->GetDriveCount();
  const CDriveList* dl = fOdinManager->GetDriveList();
  wstring s;
  const int bufSize =80;
  wchar_t buf[bufSize];

  for (int i=0; i<noDrives; i++) {
    CDriveInfo* di = dl->GetItem(i);
    wcout << L"Index: " << i << endl;
    wcout << L"Device Name: " << di->GetDeviceName() << endl;
    s = di->GetDisplayName();
    size_t pos1 = s.find(L'(')+1;
    size_t pos2 = s.find(L')');
    s = s.substr(pos1, pos2-pos1);
    wcout << L"Drive: " << s << endl;
    GetDriveTypeString(di->GetDriveType(), s);
    wcout << L"Label: " << di->GetVolumeName() << endl;
    wcout << L"Type: " << s.c_str() << endl;
    MakeByteLabel(di->GetBytes(), buf, bufSize);
    wcout << L"Size: " << buf << endl;
    wcout << endl;
  }
}

void CCommandLineProcessor::PreprocessSourceAndTarget(const wstring& name, bool sourceOrTarget) {
  // check if source name is a file or a device name or an index into the drive list.

  bool hasOnlyDigits=true;
  int index;

  Init();

  for (size_t i=0; i<name.length(); i++)
    if (!iswdigit(name[i])) {
      hasOnlyDigits = false;
      break;
    }

  if (name.substr(0, 7).compare(L"\\Device") == 0) {
    // it is  a device check validity
    const CDriveList* dl = fOdinManager->GetDriveList();
    index = dl->GetIndexOfDeviceName(name);
    if (index<0)
      THROW_CMD_EXC(sourceOrTarget ? ECmdLineException::wrongSource : ECmdLineException::wrongTarget );
    else
      sourceOrTarget ? fOperation.sourceIndex = index : fOperation.targetIndex = index;
  }
  else if (hasOnlyDigits) {
    wchar_t* stopChar = (wchar_t*) name.c_str()+name.length();
    index = wcstol(name.c_str(), &stopChar, 10);
    if (index < 0 || index>(int)fOdinManager->GetDriveCount())
      THROW_CMD_EXC(ECmdLineException::wrongIndex);
    else
      sourceOrTarget ? fOperation.sourceIndex = index : fOperation.targetIndex = index;
  }
  else // assume it is a file name
    sourceOrTarget ? fOperation.sourceIndex = -1 : fOperation.targetIndex = -1;

}

void CCommandLineProcessor::CheckValidParameters() {
  if (fOperation.cmd == CmdBackup) {
    // Source must be a device, target must be a file name
    if (fOperation.sourceIndex < 0 || fOperation.targetIndex >= 0) {
      THROW_CMD_EXC(ECmdLineException::backupParamError);
    }
  } else if (fOperation.cmd == CmdRestore) {
    // Source must be a file name, target must be a device
    if (fOperation.sourceIndex >= 0 || fOperation.targetIndex < 0) {
      THROW_CMD_EXC(ECmdLineException::restoreParamError);
    }
  } else if (fOperation.cmd == CmdVerify) {
    // Source must be a file name, target must be empty
    if (fOperation.sourceIndex >= 0 || fOperation.targetIndex >=0
      || fOperation.target.length() > 0) {
      THROW_CMD_EXC(ECmdLineException::verifyParamError);
    }
  }
}

void CCommandLineProcessor::PrintUsage() {
  wcout << L"Usage:" << endl;
  wcout << L"ODIN [operation] [options] -source=[name] -target=[name]" << endl;
  wcout << L"  [operation] is one of -backup, -restore, -verify or -list" << endl;
  wcout << L"  [options] are:" << endl;
  wcout << L"  -compression=[bzip|gzip|none]   use bzip, gzip or no compression" << endl;
  wcout << L"  -makeSnapshot    make snapshot (VSS) before backup (implies -usedBlocks)" << endl;
  wcout << L"  -usedBlocks      copy only used blocks of volume" << endl;
  wcout << L"  -allBlocks       copy all blocks of volume" << endl;
  wcout << L"  -split=[nnn]     split image file every [nnn] MB" << endl;
  wcout << L"  -comment=[string] add comment to image file for backup" << endl;
  wcout << L"  [name]    name can be a device name like \\Device\\Harddisk0\\Partition0 or" << endl;
  wcout << L"            a file name like c:\\DiskCImage.dat or a number that refers to " << endl;
  wcout << L"            an index from the -list command" << endl;
  wcout << L"  -backup   creates an image from a disk or volume to a file" << endl;
  wcout << L"  -restore  restores a disk image from a file to a volume or disk" << endl;
  wcout << L"  -verify   checks an image for damage" << endl;
  wcout << L"  -list     prints a list of available volumes on this machine" << endl;
  wcout << L"  -force    suppress all warning messages and continue immediately (very" << endl;
  wcout << L"            dangerous!)" << endl;
  wcout << endl;
  wcout << L"Examples:" << endl;
  wcout << L"ODIN -backup -usedBlocks -compression=gzip -source=1 -target=myimage.dat" << endl;
  wcout << L"  backups volume number 1 to image file myimage.dat with gzip compression " << endl;
  wcout << L"  and only used blocks (1 refers to index 1 of devices from output of -list) " << endl;
  wcout << L"ODIN -restore -source=myimage.dat -target=\\Device\\Harddisk0\\Partition0" << endl;
  wcout << L"  restores image from file myimage.dat to first partition of first disk " << endl;
  wcout << L"ODIN -list" << endl;
  wcout << L"  prints all availaible volumes and disks with their name and number" << endl;
  wcout << endl;
  wcout << L"WARNING:" << endl;
  wcout << L"USING THIS PROGRAM CAN DESTROY THE COMPLETE CONTENTS OF YOUR HARD DISK!!!" << endl;
  wcout << L"Be sure that you know and have understood what you are doing or stop now!" << endl;
  wcout << endl;
}

void CCommandLineProcessor::Reset() {
  memset(&fOperation, 0, sizeof(fOperation));
  fOperation.sourceIndex = fOperation.targetIndex = -1;
  fTimer = NULL;
  fLastPercent = 0;
  delete fFeedback;
  fFeedback = NULL;
}

void CCommandLineProcessor::OnThreadTerminated()
{
}

void CCommandLineProcessor::OnFinished()
{
  DWORD crc32;
  if (fTimer) {
    TerminateThread(fTimer, 0);
    CloseHandle(fTimer);
    fTimer = NULL;
    wcout << endl;
    delete fFeedback;
    fFeedback = NULL;
    if (fVerifyRun) {
      crc32 = fOdinManager->GetVerifiedChecksum();
      if (crc32 == fCrc32)
        wcout << L"Verify result is ok." << endl;
      else
        wcout << L"Verify result " << crc32 << L" differs from original value " << fCrc32 << endl;
      fExitCode = crc32 != fCrc32;
    }
    else
      fExitCode = 0;
    fLastPercent = 0;
    fCrc32 = 0;
    fVerifyRun = false;
  }
}

void CCommandLineProcessor::OnAbort()
{
  if (fTimer) {
    TerminateThread(fTimer, 0);
    CloseHandle(fTimer);
    fTimer = NULL;
    delete fFeedback;
    fFeedback = NULL;
    wcout << endl;
    fLastPercent = 0;
    fCrc32 = 0;
    fVerifyRun = false;
    fExitCode = 1;
  }
}

void CCommandLineProcessor::OnPartitionChange(int i, int n)
{
  wcout << endl << L"Processing now partition " << i+1 << L" of " << n << endl;
}

void CCommandLineProcessor::OnPrepareSnapshotBegin()
{
  wcout << L"Preparing Snapshot..." << endl;
}

void CCommandLineProcessor::OnPrepareSnapshotReady()
{
  wcout << L"...Preparing Snapshot done." << endl;
}


// Synchronous operations
// http://msdn.microsoft.com/en-us/magazine/cc164023.aspx
// "/SUBSYSTEM:WINDOWS /ENTRY:myMain"
// http://www.tech-archive.net/Archive/Development/microsoft.public.win32.programmer.kernel/2004-04/0534.html

void CCommandLineProcessor::ReportFeedback()
{
  wcout << L'.';
  unsigned __int64 bytesTotal = fOdinManager->GetTotalBytesToProcess();
  unsigned __int64 bytesProcessed = fOdinManager->GetBytesProcessed();
  if (bytesTotal) {
    int percent = (int) ((bytesProcessed * 100 + (bytesTotal/2)) / bytesTotal);
    if (percent > fLastPercent+10) {
      wcout << percent/10*10 << L'%';
      fLastPercent = percent;
    }
  }
}

// Code taken from: http://dslweb.nwnexus.com/~ast/dload/guicon.htm
static const WORD MAX_CONSOLE_LINES = 500;
bool  CCommandLineProcessor::InitConsole(bool createConsole) {
  int hConHandle;
  long lStdHandle;
  CONSOLE_SCREEN_BUFFER_INFO coninfo;


  // Attach a console if one exists from the parent (i.e. started from a shell)
  // This works but does not works synchronously: The parent console returns 
  // immediately and there is no way to get controlled input. Therefore use
  // ODINC.exe for this purpose which just waits and starts ODIN.exe
    bool consoleOutput = AttachConsole(ATTACH_PARENT_PROCESS) != 0;
    if (!consoleOutput) {
    if (createConsole) {
      // allocate a console for this app
      BOOL ok = AllocConsole();
      if (ok)
        fConsoleCreated = true;
      else
        return false;
    } else 
      return false;
    }

  fHasConsole = true;
  // set the screen buffer to be big enough to let us scroll text
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
  coninfo.dwSize.Y = MAX_CONSOLE_LINES;
  SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),
  coninfo.dwSize);
  // redirect unbuffered STDOUT to the console
  lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  if (hConHandle > 0) {
    fpOut = _fdopen( hConHandle, "w" );
    *stdout = *fpOut;
    setvbuf( stdout, NULL, _IONBF, 0 );
  }
  // redirect unbuffered STDIN to the console
  lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  if (hConHandle > 0) {
    fpIn = _fdopen( hConHandle, "r" );
    *stdin = *fpIn;
    setvbuf( stdin, NULL, _IONBF, 0 );
  }
  // redirect unbuffered STDERR to the console
  lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  if (hConHandle > 0) {
    fpErr = _fdopen( hConHandle, "w" );
    *stderr = *fpErr;
    setvbuf( stderr, NULL, _IONBF, 0 );
  }
  // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
  // point to console as well
  ios::sync_with_stdio();
  return true;
}

void CCommandLineProcessor::TerminateConsole() {
  if (fHasConsole) {
    if (fConsoleCreated) {
      wchar_t c;
      wcout << L"Press <return> to close console window: ";
      wcin.getline(&c, 1);
    }
    if (NULL != fpErr)
      fclose(fpErr);
    if (NULL != fpIn)
      fclose(fpIn);
    if (NULL != fpOut)
      fclose(fpOut);
    FreeConsole();
  }
}
