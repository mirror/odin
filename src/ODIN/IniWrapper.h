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
 
///////////////////////////////////////////////////////////////////
// This class is taken from www.codproject.com as an interface to 
// read and write ini files. For details see also
// http://www.codeproject.com/cpp/CIni.asp
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
//                 IniWrapper.h
//
// "CIniWrapper" is a simple API wrap class used for ini file access.
// The purpose of this class is to make ini file access more
// convenient than direct API calls.
//	
// This file is distributed "as is" and without any expressed or implied
// warranties. The author holds no responsibilities for any possible damages
// or loss of data that are caused by use of this file. The user must assume
// the entire risk of using this file.
//
// 7/08/2002    Bin Liu
//
// Update history:
//
//  7/08/2002 -- Initial release.
//  7/14/2002 -- Added "IncreaseInt" and "AppendString"
//  9/02/2002 -- Added "removeProfileSection" and "RemoveProfileEntry"
//  2/09/2003 -- The class has been made unicode-compliant
// 11/04/2003 -- Integrated MFC support, added in new member functions
//               for accessing arrays.
// 11/08/2003 -- Fixed "GetString" and "GetPathName" method, changed parameter
//               from "LPSTR" to "LPTSTR"
// 11/10/2003 -- Renamed method "GetKeys" to "GetKeyLines",
//               Added method "GetKeyNames"
//               Added parameter "bTrimString" to method "GetArray"
// 11/14/2003 -- Use "__AFXWIN_H__" instead of "_AFXDLL" to determine MFC presence
//               Removed length limit on "m_pszPathName"
//               Removed "GetStruct" and "WriteStruct"
//               Added "GetDataBlock", "WriteDataBlock", "AppendDataBlock"
//               Added "GetChar" and "WriteChar"
//
///////////////////////////////////////////////////////////////////

#ifndef __INI_H__
#define __INI_H__

#include <windows.h>
#include <tchar.h>
#include <string>

// If MFC is linked, we will use CStringArray for great convenience
#ifdef __AFXWIN_H__
	#include <afxtempl.h>
#endif

// Number bases
#define BASE_BINARY			2
#define BASE_OCTAL			8
#define BASE_DECIMAL		10
#define BASE_HEXADECIMAL	16

//---------------------------------------------------------------
//	    Callback Function Type Definition
//---------------------------------------------------------------
// The callback function used for parsing a "double-null terminated string".
// When called, the 1st parameter passed in will store the newly extracted sub
// string, the 2nd parameter is a 32-bit user defined data, this parameter can
// be NULL. The parsing will terminate if this function returns zero. To use
// the callback, function pointer needs to be passed to "CIniWrapper::ParseDNTString".
typedef BOOL (CALLBACK *SUBSTRPROC)(LPCTSTR, LPVOID);

class CIniWrapper
{
public:		

	//-----------------------------------------------------------
	//    Constructors & Destructor
	//-----------------------------------------------------------
	CIniWrapper(); // Default constructor
	CIniWrapper(LPCTSTR lpPathName); // Construct with a given file name
	virtual ~CIniWrapper();

	//-----------------------------------------------------------
	//    Ini File Path Name Access
	//-----------------------------------------------------------
	void SetPathName(LPCTSTR lpPathName); // Specify a new file name
	DWORD GetPathName(LPTSTR lpBuffer, DWORD dwBufSize) const; // Retrieve current file name
  LPCTSTR GetPathName() const {
    return m_pszPathName;
  }
#ifdef __AFXWIN_H__
	CString GetPathName() const;
#endif
	
	//------------------------------------------------------------
	//    String Access
	//------------------------------------------------------------	
	DWORD GetString(LPCTSTR lpSection, LPCTSTR lpKey, LPTSTR lpBuffer, DWORD dwBufSize, LPCTSTR lpDefault = NULL) const;
#ifdef __AFXWIN_H__
	CString GetString(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpDefault = NULL) const;
#endif
  void GetString(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpDefault, std::wstring& result) const;

	BOOL WriteString(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpValue) const;

	// Read a string from the ini file, append it with another string then write it
	// back to the ini file.
	BOOL AppendString(LPCTSTR Section, LPCTSTR lpKey, LPCTSTR lpString) const;
	
	//------------------------------------------------------------
	//    Ini File String Array Access
	//------------------------------------------------------------	
	// Parse the string retrieved from the ini file and split it into a set of sub strings.
	DWORD GetArray(LPCTSTR lpSection, LPCTSTR lpKey, LPTSTR lpBuffer, DWORD dwBufSize, LPCTSTR lpDelimiter = NULL, BOOL bTrimString = TRUE) const;
#ifdef __AFXWIN_H__
	void GetArray(LPCTSTR lpSection, LPCTSTR lpKey, CStringArray* pArray, LPCTSTR lpDelimiter = NULL, BOOL bTrimString = TRUE) const;
	BOOL WriteArray(LPCTSTR lpSection, LPCTSTR lpKey, const CStringArray* pArray, int nWriteCount = -1, LPCTSTR lpDelimiter = NULL) const;
#endif	
	
	//------------------------------------------------------------
	//    Primitive Data Type Access
	//------------------------------------------------------------
	int GetInt(LPCTSTR lpSection, LPCTSTR lpKey, int nDefault, int nBase = BASE_DECIMAL) const;
	BOOL WriteInt(LPCTSTR lpSection, LPCTSTR lpKey, int nValue, int nBase = BASE_DECIMAL) const;
	BOOL IncreaseInt(LPCTSTR lpSection, LPCTSTR lpKey, int nIncrease = 1, int nBase = BASE_DECIMAL) const;
	
	UINT GetUInt(LPCTSTR lpSection, LPCTSTR lpKey, UINT nDefault, int nBase = BASE_DECIMAL) const;
	BOOL WriteUInt(LPCTSTR lpSection, LPCTSTR lpKey, UINT nValue, int nBase = BASE_DECIMAL) const;
	BOOL IncreaseUInt(LPCTSTR lpSection, LPCTSTR lpKey, UINT nIncrease = 1, int nBase = BASE_DECIMAL) const;
  
  __int64 GetInt64(LPCTSTR lpSection, LPCTSTR lpKey, __int64 nDefault, int nBase = BASE_DECIMAL) const;
	BOOL WriteInt64(LPCTSTR lpSection, LPCTSTR lpKey, __int64 nValue, int nBase = BASE_DECIMAL) const;
	BOOL IncreaseInt64(LPCTSTR lpSection, LPCTSTR lpKey, int nIncrease = 1, int nBase = BASE_DECIMAL) const;

  unsigned __int64 GetUInt64(LPCTSTR lpSection, LPCTSTR lpKey, unsigned __int64 nDefault, int nBase = BASE_DECIMAL) const;
  BOOL WriteUInt64(LPCTSTR lpSection, LPCTSTR lpKey, unsigned __int64 nValue, int nBase = BASE_DECIMAL) const;
	BOOL IncreaseUInt64(LPCTSTR lpSection, LPCTSTR lpKey, int nIncrease = 1, int nBase = BASE_DECIMAL) const;	

	BOOL GetBool(LPCTSTR lpSection, LPCTSTR lpKey, BOOL bDefault) const;
	BOOL WriteBool(LPCTSTR lpSection, LPCTSTR lpKey, BOOL bValue) const;
	BOOL InvertBool(LPCTSTR lpSection, LPCTSTR lpKey) const;
	
	double GetDouble(LPCTSTR lpSection, LPCTSTR lpKey, double fDefault) const;
	BOOL WriteDouble(LPCTSTR lpSection, LPCTSTR lpKey, double fValue, int nPrecision = -1) const;
	BOOL IncreaseDouble(LPCTSTR lpSection, LPCTSTR lpKey, double fIncrease, int nPrecision = -1) const;

	TCHAR GetChar(LPCTSTR lpSection, LPCTSTR lpKey, TCHAR cDefault) const;
	BOOL WriteChar(LPCTSTR lpSection, LPCTSTR lpKey, TCHAR c) const;

	//------------------------------------------------------------
	//    User-Defined Data Type & Data Block Access
	//------------------------------------------------------------
	POINT GetPoint(LPCTSTR lpSection, LPCTSTR lpKey, POINT ptDefault) const;
	BOOL WritePoint(LPCTSTR lpSection, LPCTSTR lpKey, POINT pt) const;
	
	RECT GetRect(LPCTSTR lpSection, LPCTSTR lpKey, RECT rcDefault) const;
	BOOL WriteRect(LPCTSTR lpSection, LPCTSTR lpKey, RECT rc) const;

	DWORD GetDataBlock(LPCTSTR lpSection, LPCTSTR lpKey, LPVOID lpBuffer, DWORD dwBufSize, DWORD dwOffset = 0) const;
	BOOL WriteDataBlock(LPCTSTR lpSection, LPCTSTR lpKey, LPCVOID lpData, DWORD dwDataSize) const;
	BOOL AppendDataBlock(LPCTSTR lpSection, LPCTSTR lpKey, LPCVOID lpData, DWORD dwDataSize) const;
	
	//------------------------------------------------------------
	//    Section Operations
	//------------------------------------------------------------
	BOOL IsSectionExist(LPCTSTR lpSection) const;
	DWORD GetSectionNames(LPTSTR lpBuffer, DWORD dwBufSize) const;
#ifdef __AFXWIN_H__
	void GetSectionNames(CStringArray* pArray) const;
#endif
	BOOL CopySection(LPCTSTR lpSrcSection, LPCTSTR lpDestSection, BOOL bFailIfExist) const;
	BOOL MoveSection(LPCTSTR lpSrcSection, LPCTSTR lpDestSection, BOOL bFailIfExist = TRUE) const;
	BOOL DeleteSection(LPCTSTR lpSection) const;
	
	//------------------------------------------------------------
	//    Key Operations
	//------------------------------------------------------------
	BOOL IsKeyExist(LPCTSTR lpSection, LPCTSTR lpKey) const;	
	DWORD GetKeyLines(LPCTSTR lpSection, LPTSTR lpBuffer, DWORD dwBufSize) const;
#ifdef __AFXWIN_H__
	void GetKeyLines(LPCTSTR lpSection, CStringArray* pArray) const;
#endif
	DWORD GetKeyNames(LPCTSTR lpSection, LPTSTR lpBuffer, DWORD dwBufSize) const;
#ifdef __AFXWIN_H__	
	void GetKeyNames(LPCTSTR lpSection, CStringArray* pArray) const;
#endif
	BOOL CopyKey(LPCTSTR lpSrcSection, LPCTSTR lpSrcKey, LPCTSTR lpDestSection, LPCTSTR lpDestKey, BOOL bFailIfExist) const;
	BOOL MoveKey(LPCTSTR lpSrcSection, LPCTSTR lpSrcKey, LPCTSTR lpDestSection, LPCTSTR lpDestKey, BOOL bFailIfExist = TRUE) const;
	BOOL DeleteKey(LPCTSTR lpSection, LPCTSTR lpKey) const;

	//------------------------------------------------------------
	// Parse a "Double-Null Terminated String"
	//------------------------------------------------------------
	static BOOL ParseDNTString(LPCTSTR lpString, SUBSTRPROC lpFnStrProc, LPVOID lpParam = NULL);

	//------------------------------------------------------------
	// Check for Whether a String Representing TRUE or FALSE
	//------------------------------------------------------------
	static BOOL StringToBool(LPCTSTR lpString, BOOL bDefault = FALSE);
		
protected:	

	//------------------------------------------------------------
	//    Helper Functions
	//------------------------------------------------------------
	static LPTSTR __StrDupEx(LPCTSTR lpStart, LPCTSTR lpEnd);
	static BOOL __TrimString(LPTSTR lpBuffer);
	LPTSTR __GetStringDynamic(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpDefault = NULL) const;
	static DWORD __StringSplit(LPCTSTR lpString, LPTSTR lpBuffer, DWORD dwBufSize, LPCTSTR lpDelimiter = NULL, BOOL bTrimString = TRUE);
	static void __ToBinaryString(UINT nNumber, LPTSTR lpBuffer, DWORD dwBufSize);
  static void __ToBinaryString(unsigned __int64 nNumber, LPTSTR lpBuffer, DWORD dwBufSize);
	static int __ValidateBase(int nBase);
	static void __IntToString(int nNumber, LPTSTR lpBuffer, int nBase);
	static void __UIntToString(UINT nNumber, LPTSTR lpBuffer, int nBase);
  static void __UInt64ToString(unsigned __int64 nNumber, LPTSTR lpBuffer, int nBase);
  static void __Int64ToString(__int64 nNumber, LPTSTR lpBuffer, int nBase);
  static BOOL CALLBACK __SubStrCompare(LPCTSTR lpString1, LPVOID lpParam);
	static BOOL CALLBACK __KeyPairProc(LPCTSTR lpString, LPVOID lpParam);	
#ifdef __AFXWIN_H__
	static BOOL CALLBACK __SubStrAdd(LPCTSTR lpString, LPVOID lpParam);
#endif

	//------------------------------------------------------------
	//    Member Data
	//------------------------------------------------------------
	LPTSTR m_pszPathName; // Stores path of the associated ini file
};

#endif // #ifndef __INI_H__