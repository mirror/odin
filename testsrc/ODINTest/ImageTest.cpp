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
#include "ImageTest.h"
#include "ImageStreamSimulator.h"
#include "..\..\src\ODIN\ODINThread.h"
#include "..\..\src\ODIN\ReadThread.h"
#include "..\..\src\ODIN\WriteThread.h"
#include "..\..\src\ODIN\CompressionThread.h"
#include "..\..\src\ODIN\DecompressionThread.h"
#include "..\..\src\ODIN\BufferQueue.h"
#include <iostream>
using namespace std;


// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ImageTest );

static vector<unsigned __int64> sSavePositions, sRestorePositions;
unsigned ImageTest::sSavedCrc32;

void ImageTest::setUp()
{
  int cReadChunkSize = 64 * 1024;
  int nBufferCount = 8;
  fClusterSize = 4096;
  fEmptyReaderQueue = new CImageBuffer(cReadChunkSize, nBufferCount, L"fEmptyReaderQueue");
  fFilledReaderQueue = new CImageBuffer(L"fFilledReaderQueue");
  fEmptyCompDecompQueue = new CImageBuffer(cReadChunkSize, nBufferCount, L"fEmptyCompDecompQueue");
  fFilledCompDecompQueue = new CImageBuffer(L"fFilledCompDecompQueue");
}

void ImageTest::tearDown()
{
  delete fEmptyReaderQueue;
  delete fFilledReaderQueue;
  delete fEmptyCompDecompQueue;
  delete fFilledCompDecompQueue;
}

void ImageTest::runSimpleSaveRestore(bool verifyOnly)
{
  CImageStreamSimulator streamSimSource(4*1024*1024, true); // 4MB
  CImageStreamSimulator streamSimTarget(false);
  streamSimSource.SetClusterSize(fClusterSize);
  streamSimTarget.SetClusterSize(fClusterSize);
  unsigned __int64 size = streamSimSource.GetSize() ;
  CReadThread* readThread = new CReadThread(&streamSimSource, fEmptyReaderQueue, fFilledReaderQueue, verifyOnly);
  COdinThread* writeThread = new CWriteThread(&streamSimTarget, fFilledReaderQueue, fEmptyReaderQueue, verifyOnly);

  // Run threads
  readThread->Resume();
  writeThread->Resume();

  // wait until done:
  int threadCount = 2;
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  threadHandleArray[0] = readThread->GetHandle();
  threadHandleArray[1] = writeThread->GetHandle();

  WaitUntilDone(threadHandleArray, 2);

  delete readThread;
  delete writeThread;
  delete [] threadHandleArray;

  if (verifyOnly) {
    DWORD crcRead = streamSimSource.GetCRC32();
    CPPUNIT_ASSERT(sSavedCrc32 == crcRead);
  } else {
    DWORD crcRead = streamSimSource.GetCRC32();
    DWORD crcWrite = streamSimTarget.GetCRC32();
    CPPUNIT_ASSERT(crcRead == crcWrite);
    sSavedCrc32 = crcWrite;
  }
}

void ImageTest::saveRestoreImageTestSimple()
{
  cout << "saveRestoreImageTestSimple()" << endl;
  runSimpleSaveRestore(false);
  cout << "   ...done." << endl;
}

void ImageTest::verifyTestSimple()
{
  cout << "verifyTestSimple()" << endl;
  runSimpleSaveRestore(true);
  cout << "   ...done." << endl;
}

/*
void ImageTest::saveImageTestSimple()
{
  CImageStreamSimulator streamSimSource(4*1024*1024, true); // 4MB
  CImageStreamSimulator streamSimTarget(false);
  streamSimSource.SetClusterSize(fClusterSize);
  streamSimTarget.SetClusterSize(fClusterSize);
  unsigned __int64 size = streamSimSource.GetSize() ;
  cout << "saveImageTestSimple()" << endl;
  CReadThread* readThread = new CReadThread(&streamSimSource, fEmptyReaderQueue, fFilledReaderQueue, false);
  COdinThread* writeThread = new CWriteThread(&streamSimTarget, fFilledReaderQueue, fEmptyReaderQueue, false);

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

  delete readThread;
  delete writeThread;
  delete [] threadHandleArray;

  DWORD crcRead = streamSimSource.GetCRC32();
  DWORD crcWrite = streamSimTarget.GetCRC32();
  CPPUNIT_ASSERT(crcRead == crcWrite);
}

void ImageTest::restoreImageTestSimple()
{
  CImageStreamSimulator streamSimTarget(true);
  CImageStreamSimulator streamSimSource(4*1024*1024, false);
  streamSimSource.SetClusterSize(fClusterSize);
  streamSimTarget.SetClusterSize(fClusterSize);
  cout << "restoreImageTestSimple()" << endl;

  // create threads and save an image
  CReadThread* readThread = new CReadThread(&streamSimSource, fEmptyReaderQueue, fFilledReaderQueue, false);
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

  delete readThread;
  delete writeThread;
  delete [] threadHandleArray;

  DWORD crcRead = streamSimSource.GetCRC32();
  DWORD crcWrite = streamSimTarget.GetCRC32();
  CPPUNIT_ASSERT(crcRead == crcWrite);
}
*/

void ImageTest::saveImageTestRunLengthStandard()
{
  int runLengths[] = {563, 318, 745, 157, 486, 41, 290, 64, 51, 100, 51, 159, 125, 762};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  cout << "saveImageTestRunLengthStandard()" << endl;
  saveImageTestRunLength(runLengths, len);
}

void ImageTest::restoreImageTestRunLengthStandard()
{
  int runLengths[] = {563, 318, 745, 157, 486, 41, 290, 64, 51, 100, 51, 159, 125, 762};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  cout << "restoreImageTestRunLengthStandard()" << endl;
  restoreImageTestRunLength(runLengths, len);
  CPPUNIT_ASSERT(sSavePositions == sRestorePositions);
}

void ImageTest::saveImageTestRunLengthLong()
{
  int runLengths[] = {5000, 5000, 100, 100};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  cout << "saveImageTestRunLengthLong()" << endl;
  saveImageTestRunLength(runLengths, len);
}

void ImageTest::restoreImageTestRunLengthLong()
{
  int runLengths[] = {5000, 5000, 100, 100};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  cout << "restoreImageTestRunLengthLong()" << endl;
  restoreImageTestRunLength(runLengths, len);
  CPPUNIT_ASSERT(sSavePositions == sRestorePositions);
}

void ImageTest::saveImageTestRunLengthShort()
{
  int runLengths[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 1, 1};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  cout << "saveImageTestRunLengthShort()" << endl;
  saveImageTestRunLength(runLengths, len);
}

void ImageTest::restoreImageTestRunLengthShort()
{
  int runLengths[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 1, 1};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  cout << "restoreImageTestRunLengthShort()" << endl;
  restoreImageTestRunLength(runLengths, len);
  // LogSeekPositions();
  CPPUNIT_ASSERT(sSavePositions == sRestorePositions);
}

void ImageTest::saveImageTestRunLengthSpecial()
{
  // test the special case that the run length is the same as block size of read thread
  int runLengths[] = {16, 16};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  cout << "saveImageTestRunLengthSpecial()" << endl;
  saveImageTestRunLength(runLengths, len);
}

void ImageTest::restoreImageTestRunLengthSpecial()
{
  int runLengths[] = {16, 16};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  cout << "restoreImageTestRunLengthSpecial()" << endl;
  restoreImageTestRunLength(runLengths, len);
  CPPUNIT_ASSERT(sSavePositions == sRestorePositions);
}

void ImageTest::WaitUntilDone(HANDLE* threadHandleArray, int threadCount)
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

void ImageTest::saveCompressedImageGzipTest()
{
  cout << "saveCompressedImageGzipTest()..." << endl;
  saveCompressed(compressionGZip);
  cout << "  ... done" << endl;
}

void ImageTest::saveCompressedImageBzip2Test()
{
  cout << "saveCompressedImageBzip2Test()..." << endl;
  saveCompressed(compressionBZIP2);
  cout << "  ... done" << endl;
}


void ImageTest::saveCompressed(TCompressionFormat compressionType)
{
  int runLengths[] = {563, 318, 745, 157, 486, 41, 290, 64, 51, 100, 51, 159, 125, 762};
  int len = sizeof(runLengths) / sizeof(runLengths[0]);
  CImageStreamSimulator streamSimSource(runLengths, len, IImageStream::forReading, true);
  CImageStreamSimulator streamSimTarget(false);
  streamSimSource.SetClusterSize(fClusterSize);
  streamSimTarget.SetClusterSize(fClusterSize);
  unsigned __int64 size = streamSimSource.GetSize() ;

  // create threads and save an image
  CReadThread* readThread = new CReadThread(&streamSimSource, fEmptyReaderQueue, fFilledReaderQueue, false);
  COdinThread* compressionThread = new CCompressionThread(compressionType, fFilledReaderQueue, fEmptyReaderQueue, fEmptyCompDecompQueue, fFilledCompDecompQueue);
  COdinThread* writeThread = new CWriteThread(&streamSimTarget, fFilledCompDecompQueue, fEmptyCompDecompQueue, false);

  readThread->SetAllocationMapReaderInfo(streamSimSource.GetRunLengthStreamReader(), fClusterSize);
  
  // Run threads
  readThread->Resume();
  compressionThread->Resume();
  writeThread->Resume();

  // wait until done:
  int threadCount = 3;
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  threadHandleArray[0] = readThread->GetHandle();
  threadHandleArray[1] = writeThread->GetHandle();
  threadHandleArray[2] = compressionThread->GetHandle();

  WaitUntilDone(threadHandleArray, 3);

  delete readThread;
  delete writeThread;
  delete compressionThread;
  delete [] threadHandleArray;

  DWORD crcRead = streamSimSource.GetCRC32();
  DWORD crcWrite = streamSimTarget.GetCRC32();
  }

void ImageTest::saveImageTestRunLength(int* runLengthArray, int len)
{
  CImageStreamSimulator streamSimSource(runLengthArray, len, IImageStream::forReading, true);
  CImageStreamSimulator streamSimTarget(false);
  streamSimSource.SetClusterSize(fClusterSize);
  streamSimTarget.SetClusterSize(fClusterSize);
  unsigned __int64 size = streamSimSource.GetSize() ;
  // cout << "size of test stream is: " << size << endl;

  // create threads and save an image
  CReadThread* readThread = new CReadThread(&streamSimSource, fEmptyReaderQueue, fFilledReaderQueue, false);
  COdinThread* writeThread = new CWriteThread(&streamSimTarget, fFilledReaderQueue, fEmptyReaderQueue, false);

  // streamSimTarget.WriteImageFileHeaderAndAllocationMap(streamSimSource);
  readThread->SetAllocationMapReaderInfo(streamSimSource.GetRunLengthStreamReader(), fClusterSize);
  
  // Run threads
  readThread->Resume();
  writeThread->Resume();

  // wait until done:
  int threadCount = 2;
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  threadHandleArray[0] = readThread->GetHandle();
  threadHandleArray[1] = writeThread->GetHandle();

  WaitUntilDone(threadHandleArray, 2);

  delete readThread;
  delete writeThread;
  delete [] threadHandleArray;

  DWORD crcRead = streamSimSource.GetCRC32();
  DWORD crcWrite = streamSimTarget.GetCRC32();
  sSavePositions = streamSimSource.GetSeekPositions();
  CPPUNIT_ASSERT(crcRead == crcWrite);
}

void ImageTest::restoreImageTestRunLength(int* runLengthArray, int len)
{
  unsigned __int64 simFileSize = 0;
  for (int i=0; i<len; i+=2)
    simFileSize += runLengthArray[i] * fClusterSize;
  CImageStreamSimulator streamSimTarget(runLengthArray, len, IImageStream::forWriting, true);
  CImageStreamSimulator streamSimSource(simFileSize, false);
  streamSimSource.SetClusterSize(fClusterSize);
  streamSimTarget.SetClusterSize(fClusterSize);
  unsigned __int64 size = streamSimSource.GetSize() ;
  // cout << "size of test stream is: " << size << endl;

  // create threads and save an image
  CReadThread* readThread = new CReadThread(&streamSimSource, fEmptyReaderQueue, fFilledReaderQueue, false);
  CWriteThread* writeThread = new CWriteThread(&streamSimTarget, fFilledReaderQueue, fEmptyReaderQueue, false);

  writeThread->SetAllocationMapReaderInfo(streamSimTarget.GetRunLengthStreamReader(), fClusterSize);
  readThread->SetVolumeDataOffset(0);
  
  // Run threads
  readThread->Resume();
  writeThread->Resume();

  // wait until done:
  int threadCount = 2;
  HANDLE* threadHandleArray = new HANDLE[threadCount];
  threadHandleArray[0] = readThread->GetHandle();
  threadHandleArray[1] = writeThread->GetHandle();

  WaitUntilDone(threadHandleArray, 2);

  delete readThread;
  delete writeThread;
  delete [] threadHandleArray;

  DWORD crcRead = streamSimSource.GetCRC32();
  DWORD crcWrite = streamSimTarget.GetCRC32();
  sRestorePositions = streamSimTarget.GetSeekPositions();
  CPPUNIT_ASSERT(crcRead == crcWrite);
}

void ImageTest::LogSeekPositions()
{
  cout << endl;
  cout << "Seek positions comparisons:" << endl;
  cout << "size of save is: " << sSavePositions.size() << " / size of restore is. " <<  sRestorePositions.size() << endl;
  vector<unsigned __int64>::iterator it;
  cout << "Save positions: " << endl;
  for (it = sSavePositions.begin(); it!=sSavePositions.end(); ++it)
    cout << *it << ", ";
  cout << endl;
  
  cout << "Restore positions: " << endl;
  for (it = sRestorePositions.begin(); it!=sRestorePositions.end(); ++it)
    cout << *it << ", ";
  cout << endl;
}