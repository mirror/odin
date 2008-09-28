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
#include "IniWrapper.h"

// A simple class to store constant strings in a table
class CStringTable {
public: 
  CStringTable(int capacity=1024);
  ~CStringTable();

  // Add a string to a table and return a reference to the location in the store
  // return NULL if no mor space is available
  const wchar_t* AddString(const wchar_t* str);

private:
  size_t fSize;
  size_t fCapacity;
  wchar_t* fStringTable;
};

//////////////////////////////////////////////////////////////////////////////
// static variables
//////////////////////////////////////////////////////////////////////////////

extern bool CfgFileInitialize(const wchar_t* iniFileName, bool forceModuleDir=false);

// put all static members in a separate non templated class so that they exist
// only once for all template types
class CConfigEntryStatics {
public:
  static CIniWrapper sIni;
  static CStringTable sStringTable;
};

extern const wchar_t defaultSectionName[];

// for use inside of class declarations
#define DECLARE_SECTION() static const wchar_t newSectionName[];
#define IMPL_SECTION(cls, sectionName) const wchar_t cls::newSectionName[] = sectionName;
#define DECLARE_NAMED_SECTION(section) static const wchar_t section[];
#define IMPL_NAMED_SECTION(cls, section, name) const wchar_t cls::section[] = name;
// for use outside of classes 
#define DECLARE_SECTION_GLOBAL(sectionName) extern const wchar_t newSectionName[] = sectionName;
#define DECLARE_NAMED_SECTION_GLOBAL(section, sectionName) extern const wchar_t section[] = sectionName;

#define DECLARE_INIT_ENTRY(type, name, key, defaultVal) CConfigEntry<type, newSectionName> name(key, defaultVal);
#define DECLARE_ENTRY(type, name) CConfigEntry<type, newSectionName> name;
#define DECLARE_INIT_ENTRY_WITH_SECTION(type, name, key, section, defaultVal) CConfigEntry<type, section> name(key, defaultVal);
#define DECLARE_ENTRY_WITH_SECTION(type, name, section) CConfigEntry<type, section> name;

//////////////////////////////////////////////////////////////////////////////
// global helper functions
//////////////////////////////////////////////////////////////////////////////

template<class T>
T GetIniValue(const wchar_t* key, const wchar_t* section, T defaultVal);

template<class T>
void SetIniValue(const wchar_t* key, const wchar_t* section, T value);

template<>
inline int GetIniValue<int>(const wchar_t* key, const wchar_t* section, int defaultVal) {
  return CConfigEntryStatics::sIni.GetInt(section, key, defaultVal);
}

template<>
inline void SetIniValue<int>(const wchar_t* key, const wchar_t* section, int value) {
  CConfigEntryStatics::sIni.WriteInt(section, key, value);
}

template<>
inline void SetIniValue<__int64>(const wchar_t* key, const wchar_t* section, __int64 value) {
  CConfigEntryStatics::sIni.WriteInt64(section, key, value);
}

template<>
inline __int64 GetIniValue<__int64>(const wchar_t* key, const wchar_t* section, __int64 defaultVal) {
  return CConfigEntryStatics::sIni.GetInt64(section, key, defaultVal);
}

template<>
inline unsigned GetIniValue<unsigned>(const wchar_t* key, const wchar_t* section, unsigned defaultVal) {
  return CConfigEntryStatics::sIni.GetUInt(section, key, defaultVal);
}

template<>
inline void SetIniValue<unsigned>(const wchar_t* key, const wchar_t* section, unsigned value) {
  CConfigEntryStatics::sIni.WriteUInt(section, key, value);
}

template<>
inline unsigned __int64 GetIniValue<unsigned __int64>(const wchar_t* key, const wchar_t* section, unsigned __int64 defaultVal) {
  return CConfigEntryStatics::sIni.GetUInt64(section, key, defaultVal);
}

template<>
inline void SetIniValue<unsigned __int64>(const wchar_t* key, const wchar_t* section, unsigned __int64 value) {
  CConfigEntryStatics::sIni.WriteUInt64(section, key, value);
}


template<>
inline double GetIniValue<double>(const wchar_t* key, const wchar_t* section, double defaultVal) {
  return CConfigEntryStatics::sIni.GetDouble(section, key, defaultVal);
}

template<>
inline void SetIniValue<double>(const wchar_t* key, const wchar_t* section, double value) {
  CConfigEntryStatics::sIni.WriteDouble(section, key, value);
}

template<>
inline bool GetIniValue<bool>(const wchar_t* key, const wchar_t* section, bool defaultVal) {
  
  return CConfigEntryStatics::sIni.GetBool(section, key, defaultVal?TRUE:FALSE) ? true : false;
}

template<>
inline void SetIniValue<bool>(const wchar_t* key, const wchar_t* section, bool value) {
  CConfigEntryStatics::sIni.WriteBool(section, key, value);
}

template<>
inline wchar_t GetIniValue<wchar_t>(const wchar_t* key, const wchar_t* section, wchar_t defaultVal) {
  return CConfigEntryStatics::sIni.GetChar(section, key, defaultVal);
}

template<>
inline void SetIniValue<wchar_t>(const wchar_t* key, const wchar_t* section, wchar_t value) {
  CConfigEntryStatics::sIni.WriteChar(section, key, value);
}

template<>
inline void SetIniValue<std::wstring>(const wchar_t* key, const wchar_t* section, std::wstring value) {
  CConfigEntryStatics::sIni.WriteString(section, key, value.c_str());
}



//////////////////////////////////////////////////////////////////////////////
// class CConfigEntry
//////////////////////////////////////////////////////////////////////////////

template<class T, const wchar_t* section=defaultSectionName>
class CConfigEntry {
public:
  CConfigEntry(const wchar_t* key, T defaultVal);
  ~CConfigEntry();

  T operator()() const {
    return fValue;
  }

  operator T() const{
    return fValue;
  }

  CConfigEntry& operator=(T newVal) {
    fValue=newVal;
    return *this;
  }

private:
  T fValue;
  const wchar_t* fKey;
};

template<class T, const wchar_t* section>
CConfigEntry<T, section>::CConfigEntry(const wchar_t* key, T defaultVal)
{
  fKey = CConfigEntryStatics::sStringTable.AddString(key);
  fValue = GetIniValue(key, section, defaultVal);
}

template<class T, const wchar_t* section>
CConfigEntry<T, section>::~CConfigEntry()
{
  SetIniValue(fKey, section, fValue);
}

// a specialization for string types:
template<const wchar_t* section>
class CConfigEntry<std::wstring, section> {
public:
  CConfigEntry(const wchar_t* key, const wchar_t* defaultVal) {
    fKey = CConfigEntryStatics::sStringTable.AddString(key);
    CConfigEntryStatics::sIni.GetString(section, fKey, defaultVal, fValue);
  }
  CConfigEntry(const wchar_t* key, const std::wstring& defaultVal) {
    fKey = CConfigEntryStatics::sStringTable.AddString(key);
    CConfigEntryStatics::sIni.GetString(section, fKey, defaultVal.c_str(), fValue);
  }
  ~CConfigEntry() {
     CConfigEntryStatics::sIni.WriteString(section, fKey, fValue.c_str());
  }

  const std::wstring& operator()() {
    return fValue;
  }

  operator const std::wstring&() {
    return fValue;
  }
  
  CConfigEntry& operator=(const std::wstring& newVal) {
    fValue=newVal;
    return *this;
  }
  CConfigEntry& operator=(const wchar_t* newVal) {
    fValue=newVal;
    return *this;
  }

private:
  std::wstring fValue;
  const wchar_t* fKey;
};