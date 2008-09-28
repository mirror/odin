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
#include <string>
#include "IImageStream.h"
#include "OSException.h"
#include "SplitManager.h"

using namespace std;

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

//---------------------------------------------------------------------------

  // constructor for writing access to split files
CSplitManager::CSplitManager(LPCWSTR fileNamePrefix, unsigned __int64 chunkSize, CFileImageStream *stream, 
                             ISplitManagerCallback* callback)
{
  fMode = modeWrite;
  fCallback = callback;
  Init(fileNamePrefix, stream);
  fChunkSize = chunkSize;
  fFileNo = 0;
  fFileCount = 0;
  fSize = 0;
}

  // constructor for reading access to split files
CSplitManager::CSplitManager(LPCWSTR fileNamePrefix, CFileImageStream *stream, unsigned __int64 totalSize, 
                             ISplitManagerCallback* callback)
{
  fMode = modeRead;
  fCallback = callback;
  fChunkSize = 0;
  Init(fileNamePrefix, stream); // sets fChunkSize
  fFileNo = 0;
  fFileCount = (unsigned) (totalSize / fChunkSize) + (totalSize % fChunkSize ? 1 : 0);
  fSize = totalSize;
}

CSplitManager::~CSplitManager() {
}

void CSplitManager::Init(LPCWSTR fileNamePrefix, CFileImageStream *stream)
{
  fFileNamePrefix = fileNamePrefix;
  // check if we got a full name including the chunk number or a partial name
  size_t pos = fFileNamePrefix.find(L"0000");
  size_t len = fFileNamePrefix.length();
  if (pos != string::npos) {
    wstring ext;
    if (pos+4 < len)
      ext += fFileNamePrefix.substr(pos+4, len-pos-4);
    fFileNamePrefix = fFileNamePrefix.substr(0, pos);
    fFileNamePrefix+=ext;
  }
  fStream = stream;
  fFileNo = 0;
  SwitchFile(0);
}

bool CSplitManager::PreReadEvent(void * buffer, unsigned length, unsigned *bytesRead) {
  unsigned __int64 lastPosInFile = (fFileNo+1) * fChunkSize;
  unsigned bytesReadTmp;
  if (length + fStream->GetPosition() > lastPosInFile) {
    // 1. read remaining part 
    unsigned firstPartLen = (unsigned) (lastPosInFile - fStream->GetPosition());
    fStream->ReadIntern(buffer, firstPartLen, bytesRead);
    if (fFileNo < fFileCount-1) { // we may have reached the end of the image
      // 2: change file
      SwitchFile(++fFileNo);
      if (fFileNo > fStream->GetFileCount())
        fStream->SetFileCount(fFileNo+1);
      // 3: read rest from new file 
      fStream->ReadIntern((BYTE*)buffer+firstPartLen, length - firstPartLen, &bytesReadTmp);
      *bytesRead += bytesReadTmp;
    }
    return false;
  } else {
    return true;
  }
}

bool CSplitManager::PreWriteEvent(void *buffer, unsigned length, unsigned *bytesWritten) {
  unsigned __int64 lastPosInFile = (fFileNo+1) * fChunkSize;
  unsigned bytesWrittenTmp;
  if (length + fStream->GetPosition() > lastPosInFile) {
    // 1. write remaining part 
    unsigned firstPartLen = (unsigned) (lastPosInFile - fStream->GetPosition());
    fStream->WriteIntern(buffer, firstPartLen, bytesWritten);
    fSize += *bytesWritten;
    if (length - firstPartLen > 0) {
      // 2: change file
      SwitchFile(++fFileNo);
      if (fFileNo > fStream->GetFileCount())
        fStream->SetFileCount(fFileNo+1);

      // 3: write rest to new file 
      fStream->WriteIntern((BYTE*)buffer+firstPartLen, length - firstPartLen, &bytesWrittenTmp);
      *bytesWritten += bytesWrittenTmp;
      fSize += bytesWrittenTmp;
    }
    return false;
  } else {
    return true;
  }
}

bool CSplitManager::PreSeekEvent(__int64 offset, DWORD moveMethod) {
  unsigned __int64 absOffset;
  // FILE_BEGIN, FILE_END: calc absolute position, use FILE_POS
  if (moveMethod == FILE_BEGIN)
    absOffset = offset;
  else if (moveMethod == FILE_END)
    absOffset = fSize + offset;
  else
    absOffset = fStream->GetPosition() + offset;

  unsigned newFileNo = (unsigned)(absOffset / fChunkSize);
    // calculate file number to open: nFileNo = totalFilePos / splitSize
    
  if (newFileNo != fFileNo) {
    SwitchFile(newFileNo);
    fFileNo = newFileNo;

    unsigned __int64 newFilePos = absOffset - newFileNo * fChunkSize;
    fStream->SeekIntern(newFilePos, FILE_BEGIN);
    fStream->fPosition = absOffset; // needs friend privilege
    return false;
  } else {
    return true;
  }
}

void CSplitManager::SwitchFile (unsigned newFileNo) {
  wstring newName;
  IImageStream::TOpenMode mode = fStream->GetOpenMode();
  GetFileName(newFileNo, newName);
  fStream->Close();
  int res;

again:
  try {
    fStream->Open(newName.c_str(), mode);
    if (mode == IImageStream::forWriting && newFileNo > fFileCount)
      fFileCount = newFileNo; // increment count of created files
  } catch (EWinException& e) {
    // if this fails allow user to switch CD or similar to get requested file
    if (e.GetErrorCode() == EWinException::fileOpenError) {
      res = fCallback->AskUserForMissingFile(newName.c_str(), newFileNo, newName);
      if (res==IDOK)
        goto again;
      else
        throw e;
    } else
      throw e;
  }
  if (fMode == modeRead && fChunkSize == 0) {
    fChunkSize = GetFileSize(newName.c_str());
  }
}

void CSplitManager::GetFileName(unsigned fileNo, wstring& fileName) 
{
  fileName = fFileNamePrefix;
  fCallback->GetFileName(fileNo, fileName);
}

unsigned __int64 CSplitManager::GetFileSize(LPCWSTR fileName) 
{
  LARGE_INTEGER fileSize;
  HANDLE hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  BOOL ok = ::GetFileSizeEx(hFile, &fileSize);
  CloseHandle(hFile);
  return ok == TRUE ? fileSize.QuadPart : 0;
}

