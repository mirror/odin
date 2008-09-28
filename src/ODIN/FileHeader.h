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
//---------------------------------------------------------------------------
// class CImageFileHeader
// class for definition and manipulation of a common image file header
#include <string>
#include "Compression.h"

class CImageFileHeader {
friend class FileHeaderTest;

public:

  typedef struct {
    GUID guid;                            // UUID to identify file format
    WORD versionMajor;                    // major version of file format
    WORD versionMinor;                    // minor version of file format
    DWORD compressionScheme;              // format of compression algorithm
    DWORD verifyScheme;                   // format of verify information
    DWORD volumeBitmapEncodingScheme;     // format of stored volume bitmap
    DWORD volumeType;                     // type of volume (complete disk or partition)
    DWORD fileCount;                      // number of files the image is distributed across
    DWORD clusterSize;                    // size of cluster in bytes of original disk
    DWORD verifyLength;                   // size in bytes of verify information, e.g. CRC32 checksum
    DWORD commentLength;                  // size in bytes of comment string
    unsigned __int64 volumeBitmapOffset;  // file offset where run length encoded volume bitmap is stored
    unsigned __int64 volumeBitmapLength;  // length of encoded volume bitmap is stored in bytes
    unsigned __int64 verifyOffset;        // file offset where verify information (crc32) is stored
    unsigned __int64 commentOffset;       // file offset where comment string is stored
    unsigned __int64 dataOffset;          // file offset where disk clusters begin
    unsigned __int64 dataSize;            // length of used disk clusters in bytes (sizef of compressed data if compression is used)
    unsigned __int64 usedSize;            // length of used disk clusters in bytes (always uncompressed size)
    unsigned __int64 volumeSize;          // size in bytes of original volume
    unsigned __int64 fileSize;            // total length of file (useful if files are split)
  } TDiskImageFileHeader;
  
  // typedef enum { noCompression = 0, compressionGZip = 1,  compressionBZIP = 2} CompressionFormat;
  typedef enum { noVolumeBitmap = 0, simpleCompressedRunLength = 1 } VolumeEncodingFormat;
  typedef enum { verifyNone = 0, verifyCRC32 = 1 } VerifyFormat;
  typedef enum { volumeHardDisk = 0, volumePartition = 1 } VolumeFormat;

private:
  static const GUID sMagicFileHeaderGUID; 
  static const WORD sVerMajor;
  static const WORD sVerMinor ;

  TDiskImageFileHeader fHeader;
public:
  CImageFileHeader();
  ~CImageFileHeader()
  {}
  
  void WriteHeaderToFile(HANDLE hFileOut);
  void ReadHeaderFromFile(HANDLE hFileIn);
  
  DWORD GetHeaderFileLength() {
    return sizeof(fHeader);
  }

  bool IsValidFileHeader() const;
  bool IsSupportedVersion() const;
  bool IsSupportedChecksumMethod() const;
  bool IsSupportedCompressionFormat() const;
  bool IsSupportedVolumeEncodingFormat() const;
  
  unsigned GetMajorVersion() {
    return (unsigned) fHeader.versionMajor;
  }
  
  unsigned GetMinorVersion() {
    return (unsigned) fHeader.versionMinor;
  }

  void SetVolumeBitmapInfo(VolumeEncodingFormat format, unsigned __int64 offset, unsigned __int64 length);
  void SetCompressionFormat(TCompressionFormat compFormat);
  void SetImageFileVerificationInfo(VerifyFormat format, DWORD length);
  void SetVolumeSize(unsigned __int64 volSize);

  unsigned __int64 GetVolumeSize() const {
    return fHeader.volumeSize;
  }

  TCompressionFormat GetCompressionFormat() const {
    return (TCompressionFormat) fHeader.compressionScheme;
  }

  VolumeEncodingFormat GetVolumeEncoding() const {
    return (VolumeEncodingFormat) fHeader.volumeBitmapEncodingScheme;
  }
  
  VerifyFormat GetVerifyFormat() const {
    return (VerifyFormat)fHeader.verifyScheme;
  }

  void SetVerifyFormat(VerifyFormat verifyFormat) {
    fHeader.verifyScheme = verifyFormat;
  }

  void GetVerifyOffsetAndLength(unsigned __int64& verifyOffset, DWORD& verifyLength) const {
    verifyOffset = fHeader.verifyOffset;
    verifyLength = fHeader.verifyLength;
  }

  void SetVerifyOffsetAndLength(unsigned __int64 verifyOffset, DWORD verifyLength) {
    fHeader.verifyOffset = verifyOffset;
    fHeader.verifyLength = verifyLength;
  }

  void GetClusterBitmapOffsetAndLength(unsigned __int64& volumeBitmapOffset, unsigned __int64& volumeBitmapSize) const {
    volumeBitmapOffset = fHeader.volumeBitmapOffset;
    volumeBitmapSize = fHeader.volumeBitmapLength;
  }

  unsigned __int64 GetVolumeDataOffset() const {
     return fHeader.dataOffset;
  }

  void SetVolumeDataOffset(unsigned __int64 dataOffset) {
    fHeader.dataOffset = dataOffset;
  }

  unsigned __int64 GetVolumeUsedSize() const {
     return fHeader.usedSize;
  }

  void SetVolumeUsedSize(unsigned __int64 usedSize) {
    fHeader.usedSize = usedSize;
  }

  void SetClusterSize(DWORD clusterSize) {
    fHeader.clusterSize = clusterSize;
  }

  DWORD GetClusterSize() const {
    return fHeader.clusterSize;
  }

  void GetCommentOffsetAndLength(unsigned __int64& commentOffset, DWORD& commentLength) const {
    commentOffset = fHeader.commentOffset;
    commentLength = fHeader.commentLength;
  }

  void SetComment(unsigned __int64 commentOffset, DWORD commentLength) {
    fHeader.commentOffset = commentOffset;
    fHeader.commentLength = commentLength;
  }
  
  void SetDataSize(unsigned __int64 bytesProcessed) {
    fHeader.dataSize = bytesProcessed;
    fHeader.fileSize = bytesProcessed + fHeader.commentLength + fHeader.volumeBitmapLength
      + fHeader.verifyLength + sizeof(TDiskImageFileHeader);
    //ATLTRACE("Write file size of %u after %u bytes processed\n", (unsigned)fHeader.fileSize, (unsigned) bytesProcessed);
  }

  unsigned __int64  GetDataSize() const {
    return fHeader.dataSize;
  }

  unsigned __int64  GetFileSize() const {
    return fHeader.fileSize;
  }

  void SetFileCount(DWORD fileCount) {
    fHeader.fileCount = fileCount;
  }

  DWORD GetFileCount() const {
    return fHeader.fileCount;
  }

  VolumeFormat GetVolumeType() const {
    return (VolumeFormat) fHeader.volumeType;
  }

  void SetVolumeType(VolumeFormat volType) {
    fHeader.volumeType = volType;
  }
};
//---------------------------------------------------------------------------
