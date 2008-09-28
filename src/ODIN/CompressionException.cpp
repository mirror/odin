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
#include "CompressionException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG


// error message strings (should later be moved to a separate non compiled file)
LPCWSTR EZLibCompressionException::sMessages[] = {
  L"no error (0)",                 
  L"general compression/decompression error (-1)", // Z_ERRNO
  L"error in data stream (-2)",     // Z_STREAM_ERROR 
  L"error in data (-3)",            // Z_DATA_ERROR   
  L"memory error (-4)",             // Z_MEM_ERROR    
  L"buffer error (-5)",             // Z_BUF_ERROR    
  L"wrong version of library (-6)", // Z_VERSION_ERROR
};

void EZLibCompressionException::BuildMessageString()
{
  if (fErrorCode >= fFirstCode && fErrorCode <= fLastCode)
    fMessage += sMessages[-fErrorCode];
}

//---------------------------------------------------------------------------

// error message strings (should later be moved to a separate non compiled file)
LPCWSTR EBZip2CompressionException::sMessages[] = {
  L"no error (0)",                 
  L"error in bzip2 sequence (-1)", // BZ_SEQUENCE_ERROR  
  L"bzip2 parameter error (-2)", // BZ_PARAM_ERROR     
  L"bzip2 memory error (-3)", // BZ_MEM_ERROR       
  L"bzip2 data error (-4)", // BZ_DATA_ERROR      
  L"bzip2 magic data error (-5)", // BZ_DATA_ERROR_MAGIC
  L"bzip2 IO error (-6)", // BZ_IO_ERROR        
  L"bzip2 unexpected end of file (-7)", // BZ_UNEXPECTED_EOF  
  L"bzip2 output buffer is full (-8)", // BZ_OUTBUFF_FULL    
  L"bzip2 configuration error (-9)", // BZ_CONFIG_ERROR    
};

void EBZip2CompressionException::BuildMessageString()
{
  if (fErrorCode >= fFirstCode && fErrorCode <= fLastCode)
    fMessage += sMessages[-fErrorCode];
}

//---------------------------------------------------------------------------

void exceptionTest()
{
}
