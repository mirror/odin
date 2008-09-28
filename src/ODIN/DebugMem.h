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
#ifndef __DEBUGMEM_H___
#define __DEBUGMEM_H___

// A class that uses the Microsoft Debug Heap to assist in finding memory leaks
// and other memory related issus

#include <crtdbg.h>


#ifdef DEBUG
  #define DEBUG_NEW   new( _NORMAL_BLOCK, __FILE__, __LINE__)
  #define DEBUG_MALLOC(size) _malloc_dbg( size, _NORMAL_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_NEW
#endif // _DEBUG

// In each source file insert the following code:
//#ifdef DEBUG
//  #define new DEBUG_NEW
  // Note: Do not use #define new _NEW_CRT as defined in STL. This causes blocks to be allocated
  // as part of the C runtime library that are not tracked (or tracked differently)
//  #define malloc DEBUG_MALLOC
//#endif // _DEBUG

#endif
