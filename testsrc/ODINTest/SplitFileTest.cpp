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
#include "SplitFileTest.h"
#include "ImageStreamSimulator.h"
#include "..\..\src\ODIN\ODINThread.h"
#include "..\..\src\ODIN\ReadThread.h"
#include "..\..\src\ODIN\WriteThread.h"
#include "..\..\src\ODIN\BufferQueue.h"
#include "..\..\src\ODIN\ImageStream.h"
#include "..\..\src\ODIN\SplitManager.h"
#include <iostream>
using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( SplitFileTest );

const unsigned SplitFileTest::sChunkSize = 1024*1024; // 1MB
const unsigned SplitFileTest::sStreamSize = 4*1024*1024; // 4MB

class CSplitManagerCallback : public ISplitManagerCallback 
{
public:
  virtual void GetFileName(unsigned fileNo, std::wstring& fileName) {
    wchar_t buf[10];
    wsprintf(buf, L"%04u", fileNo);
    fileName += buf;
  }

  virtual size_t AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, std::wstring& newName) {
    return IDCANCEL;
  }
};

class CAskUserSplitManagerCallback : public ISplitManagerCallback 
{
public:
  CAskUserSplitManagerCallback(LPCWSTR orgName)
    : fOrgName(orgName)
  {}

  virtual void GetFileName(unsigned fileNo, std::wstring& fileName) {
    wchar_t buf[10];
    wsprintf(buf, L"%04u", 0);
    fileName += buf;
  }

  virtual size_t AskUserForMissingFile(LPCWSTR missingFileName, unsigned fileNo, std::wstring& newName) {
    newName = fOrgName;
    GetFileName(fileNo, newName);
    return IDOK;
  }

private:
  wstring fOrgName;
};

void SplitFileTest::setUp()
{
  int cReadChunkSize = 64 * 1024;
  int nBufferCount = 8;
  fClusterSize = 4096;
  fEmptyReaderQueue = new CImageBuffer(cReadChunkSize, nBufferCount, L"fEmptyReaderQueue");
  fFilledReaderQueue = new CImageBuffer(L"fFilledReaderQueue");

  // get temp file
  wchar_t pathBuffer[MAX_PATH];
  GetTempPath(MAX_PATH, pathBuffer);  
  fFileNamePrefix = pathBuffer;
  fFileNamePrefix += L"SplitTestFile-";
  fRenamedFileNamePrefix = pathBuffer;
  fRenamedFileNamePrefix += L"RenamedSplitTestFile-";
}

void SplitFileTest::tearDown()
{
  delete fEmptyReaderQueue;
  delete fFilledReaderQueue;
}

void SplitFileTest::saveSplitFileTest()
{
  CImageStreamSimulator streamSimSource(sStreamSize, true); // 4MB
  CFileImageStream targetStream;
  CSplitManagerCallback cb;

  targetStream.Open(NULL, IImageStream::forWriting);
  CSplitManager splitCallback(fFileNamePrefix.c_str(), sChunkSize, &targetStream, &cb);
  targetStream.RegisterCallback(&splitCallback);
  streamSimSource.SetClusterSize(fClusterSize);
  unsigned __int64 size = streamSimSource.GetSize() ;
  cout << "saveSplitFileTest()" << endl;
  CReadThread* readThread = new CReadThread(&streamSimSource, fEmptyReaderQueue, fFilledReaderQueue, false);
  COdinThread* writeThread = new CWriteThread(&targetStream, fFilledReaderQueue, fEmptyReaderQueue, false);

  // Run threads
  readThread->Resume();
  writeThread->Resume();

  // wait until done:
  int threadCount = 2;
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  threadHandleArray[0] = readThread->GetHandle();
  threadHandleArray[1] = writeThread->GetHandle();

  WaitUntilDone(threadHandleArray, 2);
  cout << "   ...done." << endl;
  targetStream.UnegisterCallback();
  DWORD crcRead = streamSimSource.GetCRC32();
  DWORD crcWrite = writeThread->GetCrc32();
  CPPUNIT_ASSERT(crcRead == crcWrite);
  CPPUNIT_ASSERT(splitCallback.GetFileCount()+1 == sStreamSize / sChunkSize + (sStreamSize%sChunkSize==0 ? 0 : 1));
  targetStream.Close();
  
  // all files except the last one should have chunk size
  for (unsigned i=0; i<splitCallback.GetFileCount(); i++) {
    wstring fileName;
    splitCallback.GetFileName(i, fileName);
    CheckFileSize(fileName.c_str(), sChunkSize);
  }
  delete readThread;
  delete writeThread;
  delete [] threadHandleArray;
}

void SplitFileTest::restoreSplitFileTest()
{
  CImageStreamSimulator streamSimTarget(true);
  CFileImageStream sourceStream;
  CSplitManagerCallback cb;

  sourceStream.Open(NULL, IImageStream::forReading);
  CSplitManager splitCallback(fFileNamePrefix.c_str(), &sourceStream, sStreamSize, &cb);
  sourceStream.RegisterCallback(&splitCallback);

  streamSimTarget.SetClusterSize(fClusterSize);
  cout << "restoreSplitFileTest()" << endl;

  // create threads and save an image
  CReadThread* readThread = new CReadThread(&sourceStream, fEmptyReaderQueue, fFilledReaderQueue, false);
  CWriteThread* writeThread = new CWriteThread(&streamSimTarget, fFilledReaderQueue, fEmptyReaderQueue, false);

  // Run threads
  readThread->Resume();
  writeThread->Resume();

  // wait until done:
  int threadCount = 2;
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  threadHandleArray[0] = readThread->GetHandle();
  threadHandleArray[1] = writeThread->GetHandle();

  WaitUntilDone(threadHandleArray, 2);
  sourceStream.Close();

  DWORD crcRead = readThread->GetCrc32();
  DWORD crcWrite = streamSimTarget.GetCRC32();
  CPPUNIT_ASSERT(crcRead == crcWrite);

  delete readThread;
  delete writeThread;
  delete [] threadHandleArray;
}

void SplitFileTest::askForUserFileTest()
{
  CImageStreamSimulator streamSimTarget(true);
  CFileImageStream sourceStream;
  CAskUserSplitManagerCallback cb(fFileNamePrefix.c_str());

  sourceStream.Open(NULL, IImageStream::forReading);
  CSplitManager splitCallback(fRenamedFileNamePrefix.c_str(), &sourceStream, sStreamSize, &cb);
  sourceStream.RegisterCallback(&splitCallback);

  streamSimTarget.SetClusterSize(fClusterSize);
  cout << "askForUserFileTest()" << endl;

  // create threads and save an image
  CReadThread* readThread = new CReadThread(&sourceStream, fEmptyReaderQueue, fFilledReaderQueue, false);
  CWriteThread* writeThread = new CWriteThread(&streamSimTarget, fFilledReaderQueue, fEmptyReaderQueue, false);

  // Run threads
  readThread->Resume();
  writeThread->Resume();

  // wait until done:
  int threadCount = 2;
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  threadHandleArray[0] = readThread->GetHandle();
  threadHandleArray[1] = writeThread->GetHandle();

  WaitUntilDone(threadHandleArray, 2);
  sourceStream.Close();

  DWORD crcRead = readThread->GetCrc32();
  DWORD crcWrite = streamSimTarget.GetCRC32();
  CPPUNIT_ASSERT(crcRead == crcWrite);

  delete readThread;
  delete writeThread;
  delete [] threadHandleArray;
}

void SplitFileTest::seekSplitFileTest()
{
  CFileImageStream sourceStream;
  DWORD curFilePos=0; 
  HANDLE h;
  CSplitManagerCallback cb;

  cout << "seekSplitFileTest()" << endl;
  sourceStream.Open(NULL, IImageStream::forReading);
  CSplitManager splitCallback(fFileNamePrefix.c_str(), &sourceStream, sStreamSize, &cb);
  sourceStream.RegisterCallback(&splitCallback);

  // seek to position 0 
  sourceStream.Seek(0, FILE_BEGIN);
  CPPUNIT_ASSERT(sourceStream.GetPosition() == 0);
  CPPUNIT_ASSERT(splitCallback.GetFileNo() == 0);
  h = sourceStream.GetFileHandle(); 
  curFilePos = SetFilePointer(h, 0, NULL, FILE_CURRENT);
  CPPUNIT_ASSERT(curFilePos == 0);

  // seek to end position 
  sourceStream.Seek(-1, FILE_END);
  CPPUNIT_ASSERT(sourceStream.GetPosition() == sStreamSize-1);
  CPPUNIT_ASSERT(splitCallback.GetFileNo()+1 == splitCallback.GetFileCount());
  h = sourceStream.GetFileHandle(); 
  curFilePos = SetFilePointer(h, 0, NULL, FILE_CURRENT);
  CPPUNIT_ASSERT(curFilePos == sChunkSize-1);

  // seek to 2nd file 
  sourceStream.Seek(sChunkSize+100, FILE_BEGIN);
  CPPUNIT_ASSERT(sourceStream.GetPosition() == sChunkSize+100);
  CPPUNIT_ASSERT(splitCallback.GetFileNo() == 1);
  h = sourceStream.GetFileHandle(); 
  curFilePos = SetFilePointer(h, 0, NULL, FILE_CURRENT);
  CPPUNIT_ASSERT(curFilePos == 100);
  sourceStream.Close();

  // seek relative 
  sourceStream.Seek(-(int)sChunkSize, FILE_CURRENT);
  CPPUNIT_ASSERT(sourceStream.GetPosition() == 100);
  CPPUNIT_ASSERT(splitCallback.GetFileNo() == 0);
  h = sourceStream.GetFileHandle(); 
  curFilePos = SetFilePointer(h, 0, NULL, FILE_CURRENT);
  CPPUNIT_ASSERT(curFilePos == 100);
  sourceStream.Close();
}

void SplitFileTest::deleteSplitFileTest()
{
  cout << "deleteSplitFileTest()" << endl;
  unsigned noFiles = 0;
  CFileImageStream sourceStream;
  CSplitManagerCallback cb;

  sourceStream.Open(NULL, IImageStream::forReading);
  CSplitManager splitCallback(fFileNamePrefix.c_str(), &sourceStream, sStreamSize, &cb);
  sourceStream.Close();

  // delete files
  for (unsigned i=0; i<=noFiles; i++) {
    wstring fileName;
    splitCallback.GetFileName(i, fileName);
    BOOL ok = DeleteFile(fileName.c_str());
    CPPUNIT_ASSERT(ok == TRUE);
  }
}

void SplitFileTest::WaitUntilDone(HANDLE* threadHandleArray, int threadCount)
{
  while (TRUE) {
    DWORD result = MsgWaitForMultipleObjects(threadCount, threadHandleArray, FALSE, INFINITE, QS_ALLEVENTS);
    if (result >= WAIT_OBJECT_0 && result < (DWORD)threadCount) {
      // ATLTRACE("event arrived: %d, thread id: %x\n", result, threadHandleArray[result]);
      if (--threadCount == 0)  {
        //cout << " All worker threads are terminated now\n" << endl;
        break;
      }
      // setup new array with the remaining threads:
      for (int i=result; i<threadCount; i++)
        threadHandleArray[i] = threadHandleArray[i+1];
    }
  }
}

void SplitFileTest::CheckFileSize(LPCWSTR fileName, unsigned __int64 expectedSize) 
{
  LARGE_INTEGER realSize;
  HANDLE hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  DWORD err = GetLastError();
  BOOL ok = ::GetFileSizeEx(hFile, &realSize);
  CPPUNIT_ASSERT(ok == TRUE);
  CPPUNIT_ASSERT(expectedSize == realSize.QuadPart);
  CloseHandle(hFile);
}