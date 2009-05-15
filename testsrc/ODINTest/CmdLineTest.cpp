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
#include <iostream>
#include "..\..\src\ODIN\OdinManager.h"
#include "CmdLineTest.h"
#include "..\..\src\ODIN\CommandLineProcessor.h"
#include "..\..\src\ODIN\CmdLineException.h"

using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( CCmdLineTest );

class DummyWaitCallback: public IWaitCallback {
public:
  virtual void OnThreadTerminated()
  {}
  virtual void OnFinished()
  {}
  virtual void OnAbort()
  {}
  virtual void OnPartitionChange(int i, int n)
  {}
  virtual void OnPrepareSnapshotBegin()
  {}
  virtual void OnPrepareSnapshotReady()
  {}
};

void CCmdLineTest::setUp()
{
}

void CCmdLineTest::tearDown()
{
}

void CCmdLineTest::TestParseOptions()
{
  CCommandLineProcessor cp;
  fCommandLine = L"ODIN.exe -usage";
  cp.Parse(fCommandLine.c_str());

  // check that all backup options are filled
  fCommandLine = L"ODIN.exe -backup -source=0 -target=myfile.img -compression=bzip -makeSnapshot -split=640";
  try {
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_ASSERT(cp.fOperation.cmd == CCommandLineProcessor::CmdBackup);
    CPPUNIT_ASSERT(cp.fOperation.compression == compressionBZIP2);
    CPPUNIT_ASSERT(cp.fOperation.mode == CCommandLineProcessor::modeUsedBlocksAndSnapshot);
    CPPUNIT_ASSERT(cp.fOperation.splitSizeMB == 640);
    CPPUNIT_ASSERT(cp.fOperation.force == false);

    cp.Reset();
    fCommandLine = L"ODIN.exe -backup -source=0 -target=myfile.img -compression=gzip -usedBlocks -force";
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_ASSERT(cp.fOperation.compression == compressionGZip);
    CPPUNIT_ASSERT(cp.fOperation.mode == CCommandLineProcessor::modeOnlyUsedBlocks);
    CPPUNIT_ASSERT(cp.fOperation.force == true);

    fCommandLine = L"ODIN.exe -backup -source=0 -target=myfile.img -compression=none -allBlocks -comment=\"some comment\"";
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_ASSERT(cp.fOperation.compression == noCompression);
    CPPUNIT_ASSERT(cp.fOperation.mode == CCommandLineProcessor::modeAllBlocks);
    CPPUNIT_ASSERT(cp.fOperation.force == false);
    CPPUNIT_ASSERT(cp.fOperation.comment.compare(L"some comment") == 0);

    fCommandLine = L"ODIN.exe -restore -source=myfile.img -target=0";
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_ASSERT(cp.fOperation.cmd == CCommandLineProcessor::CmdRestore);

    fCommandLine = L"ODIN.exe -verify -source=myfile.img";
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_ASSERT(cp.fOperation.cmd == CCommandLineProcessor::CmdVerify);

    fCommandLine = L"ODIN.exe -list";
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_ASSERT(cp.fOperation.cmd == CCommandLineProcessor::CmdList);

  } catch (ECmdLineException &) {
    CPPUNIT_FAIL("backup options should not raise a CmdLineException");
  }
}

void CCmdLineTest::TestParseSourceTarget()
{
  CCommandLineProcessor cp;

  try {
    fCommandLine = L"ODIN.exe -backup -source=0 -target=myfile.img";
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    CPPUNIT_ASSERT(cp.fOperation.sourceIndex == 0);
    CPPUNIT_ASSERT(cp.fOperation.source.compare(L"0")==0);
    CPPUNIT_ASSERT(cp.fOperation.target.compare(L"myfile.img")==0);
    CPPUNIT_ASSERT(cp.fOperation.targetIndex == -1);
    
    cp.Reset();
    fCommandLine = L"ODIN.exe -backup -source=\\Device\\Harddisk0\\Partition0 -target=myfile.img";
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    CPPUNIT_ASSERT(cp.fOperation.sourceIndex >= 0);
    CPPUNIT_ASSERT(cp.fOperation.source.compare(L"\\Device\\Harddisk0\\Partition0")==0);
    CPPUNIT_ASSERT(cp.fOperation.target.compare(L"myfile.img")==0);
    CPPUNIT_ASSERT(cp.fOperation.targetIndex == -1);

    cp.Reset();
    fCommandLine = L"ODIN.exe -restore -target=0 -source=myfile.img";
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    CPPUNIT_ASSERT(cp.fOperation.targetIndex == 0);
    CPPUNIT_ASSERT(cp.fOperation.target.compare(L"0")==0);
    CPPUNIT_ASSERT(cp.fOperation.source.compare(L"myfile.img")==0);
    CPPUNIT_ASSERT(cp.fOperation.sourceIndex == -1);

    cp.Reset();
    fCommandLine = L"ODIN.exe -restore -target=\\Device\\Harddisk0\\Partition0 -source=myfile.img";
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    CPPUNIT_ASSERT(cp.fOperation.target.compare(L"\\Device\\Harddisk0\\Partition0")==0);
    CPPUNIT_ASSERT(cp.fOperation.targetIndex >= 0);
    CPPUNIT_ASSERT(cp.fOperation.source.compare(L"myfile.img")==0);
    CPPUNIT_ASSERT(cp.fOperation.sourceIndex == -1);

  } catch (ECmdLineException &) {
    CPPUNIT_FAIL("backup options should not raise a CmdLineException");
  }
}

void CCmdLineTest::TestParameterValidation()
{
  CCommandLineProcessor cp;  

  fCommandLine = L"ODIN.exe -list -unknownOption";
  try {
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_FAIL("unknown option should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::unknownOption);
  }

  cp.Reset();
  fCommandLine = L"ODIN.exe -source=0 -target=myfile.img -compression=bzip -makeSnapshot -split=640";
  try {
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_FAIL("missing operation option should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::noOperation);
  }

  cp.Reset();
  fCommandLine = L"ODIN.exe -backup -source=0 -target=myfile.img -compression=xyzzip";
  try {
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_FAIL("missing operation option should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::wrongCompression);
  }

  cp.Reset();
  fCommandLine = L"ODIN.exe -source=0 -target=myfile.img -compression=bzip -makeSnapshot -split=640";
  try {
    cp.Parse(fCommandLine.c_str());
    CPPUNIT_FAIL("missing operation option should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::noOperation);
  }
}

void CCmdLineTest::TestSourceTargetValidation()
{
  CCommandLineProcessor cp;  

  // test illegal device name for backup
  fCommandLine = L"ODIN.exe -backup -source=\\Device\\GarbageXYZ -target=myfile.img";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    CPPUNIT_FAIL("illegal source device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::wrongSource);
  }
  
  // test illegal device name for restore
  cp.Reset();
  fCommandLine = L"ODIN.exe -restore -target=\\Device\\GarbageXYZ -source=myfile.img";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::wrongTarget);
  }

  // test wrong index for restore
  cp.Reset();
  fCommandLine = L"ODIN.exe -restore -target=32768 -source=myfile.img";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::wrongIndex);
  }

  // test wrong index for backup
  cp.Reset();
  fCommandLine = L"ODIN.exe -backup -source=32768 -target=myfile.img";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::wrongIndex);
  }

  // test missing target for backup
  cp.Reset();
  fCommandLine = L"ODIN.exe -backup -source=0";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::noTarget);
  }

  // test missing source for backup
  cp.Reset();
  fCommandLine = L"ODIN.exe -backup -target=myfile.img";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::noSource);
  }

  // test missing source for verify
  cp.Reset();
  fCommandLine = L"ODIN.exe -verify";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::noSource);
  }

  // test non allowed target for verify
  cp.Reset();
  fCommandLine = L"ODIN.exe -verify -source=myfile.img -target=1";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::verifyParamError);
  }

  // test not allowed target type for backup
  cp.Reset();
  fCommandLine = L"ODIN.exe -backup -source=myfile.img -target=1";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::backupParamError);
  }

  // test not allowed source type for backup
  cp.Reset();
  fCommandLine = L"ODIN.exe -backup -source=myfile.img -target=myotherfile.img";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::backupParamError);
  }

  // test not allowed source type for restore
  cp.Reset();
  fCommandLine = L"ODIN.exe -backup -source=2 -target=1";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::backupParamError);
  }

  // test not allowed target type for restore
  cp.Reset();
  fCommandLine = L"ODIN.exe -restore -source=myfile.img -target=myotherfile.img";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.PreprocessSourceAndTarget(cp.fOperation.target, false);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::restoreParamError);
  }

  // test not allowed source type for verify
  cp.Reset();
  fCommandLine = L"ODIN.exe -verify -source=1";
  try {
    cp.Parse(fCommandLine.c_str());
    cp.PreprocessSourceAndTarget(cp.fOperation.source, true);
    cp.CheckValidParameters();
    CPPUNIT_FAIL("illegal target device name should raise a CmdLineException");
  } catch (ECmdLineException &e) {
    CPPUNIT_ASSERT(e.GetErrorCode() == ECmdLineException::verifyParamError);
  }

}
