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
// Command Line parser class
// This code is from http://www.codeproject.com/KB/cpp/cma.aspx
// See this web page for documentation
// Code contributed from swuk Croatia licensed under the The Code Project 
// Open License (CPOL) 1.02: http://www.codeproject.com/info/cpol10.aspx
// Croatian comments removed.
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <vector>
//using namespace std;
#include <string.h>

template <typename T> class CArgList
{
	/*
	I found this functions at:
	http://latexdaemon.googlecode.com/svn/latexdaemon/
	I'm using them for some time now and they seem to work correctly.
	*/
	PWCHAR* MyCommandLineToArgvW( PCWCH CmdLine, int* _argc )
	{
		PWCHAR* argv;
		PWCHAR  _argv;
		size_t   len;
		int   argc;
		WCHAR   a;
		size_t   i, j;

		BOOLEAN  in_QM;
		BOOLEAN  in_TEXT;
		BOOLEAN  in_SPACE;

		len = wcslen(CmdLine);
		i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

		argv = (PWCHAR*)GlobalAlloc(GMEM_FIXED,
			i + (len+2)*sizeof(WCHAR));

		_argv = (PWCHAR)(((PUCHAR)argv)+i);

		argc = 0;
		argv[argc] = _argv;
		in_QM = FALSE;
		in_TEXT = FALSE;
		in_SPACE = TRUE;
		i = 0;
		j = 0;

		while( a = CmdLine[i] ) {
			if(in_QM) {
				if(a == '\"') {
					in_QM = FALSE;
				} else {
					_argv[j] = a;
					j++;
				}
			} else {
				switch(a) {
			case '\"':
				in_QM = TRUE;
				in_TEXT = TRUE;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				in_SPACE = FALSE;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if(in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = FALSE;
				in_SPACE = TRUE;
				break;
			default:
				in_TEXT = TRUE;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = FALSE;
				break;
				}
			}
			i++;
		}
		_argv[j] = '\0';
		argv[argc] = NULL;

		(*_argc) = argc;
		return argv;
	}


	PCHAR* MyCommandLineToArgvA( PCHAR CmdLine, int* _argc )
	{
		PCHAR* argv;
		PCHAR  _argv;
		size_t   len;
		int   argc;
		CHAR   a;
		size_t   i, j;

		BOOLEAN  in_QM;
		BOOLEAN  in_TEXT;
		BOOLEAN  in_SPACE;

		len = strlen(CmdLine);
		i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

		argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
			i + (len+2)*sizeof(CHAR));

		_argv = (PCHAR)(((PUCHAR)argv)+i);

		argc = 0;
		argv[argc] = _argv;
		in_QM = FALSE;
		in_TEXT = FALSE;
		in_SPACE = TRUE;
		i = 0;
		j = 0;

		while( a = CmdLine[i] ) {
			if(in_QM) {
				if(a == '\"') {
					in_QM = FALSE;
				} else {
					_argv[j] = a;
					j++;
				}
			} else {
				switch(a) {
			case '\"':
				in_QM = TRUE;
				in_TEXT = TRUE;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				in_SPACE = FALSE;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if(in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = FALSE;
				in_SPACE = TRUE;
				break;
			default:
				in_TEXT = TRUE;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = FALSE;
				break;
				}
			}
			i++;
		}
		_argv[j] = '\0';
		argv[argc] = NULL;

		(*_argc) = argc;
		return argv;
	}

	void CreateArgList(int* nArgs, wchar_t** *argList)
	{
#ifdef USING_CommandLineToArgvW
		*argList = CommandLineToArgvW(GetCommandLineW(), nArgs);
#else
		*argList = MyCommandLineToArgvW(GetCommandLineW(), nArgs);
#endif
	}
	void CreateArgList(int* nArgs, char** *argList)
	{
		*argList = MyCommandLineToArgvA(GetCommandLineA(), nArgs);
	}

	void CreateArgList(const wchar_t* commandLine, int* nArgs, wchar_t** *argList)
	{
#ifdef USING_CommandLineToArgvW
		*argList = CommandLineToArgvW(commandLine, nArgs);
#else
		*argList = MyCommandLineToArgvW(commandLine, nArgs);
#endif
	}

    void CreateArgList(const char* commandLine, int* nArgs, char** *argList)
	{
		*argList = MyCommandLineToArgvA(commandLine, nArgs);
	}



public:
	/*
	Let them be public. When we have _tmain(int argc, _TCHAR* argv[]), those argc and argv are also freely accessible
	and can even be changed.
	*/
	T** argv;
	int argc;
	CArgList()
	{
		CreateArgList(&argc, &argv);
	}
	CArgList(const T* commandLine)
	{
		CreateArgList(commandLine, &argc, &argv);
	}
	~CArgList()
	{
		LocalFree(argv);
	}
};


template <typename T> class CStlCmdLineArgs
{
	typedef std::basic_string<T> STR;

	char* myinstr(char* s, char c)
	{
		return ::strchr(s, c);
		//maybe better, but it works with unsigned...
		//return _mbschr(s, c);
	}
	wchar_t* myinstr(wchar_t* s, wchar_t c)
	{
		return ::wcschr(s, c);
	}

	T terminatingCharacter;
	T delimiter; //Named option should begin with this character. Default is '/'.
	T optionsCharacter; //Separates option name from its value. Default is ':'.

	friend class CUnnamed;
	class CUnnamed
	{
		friend class CStlCmdLineArgs;
		CStlCmdLineArgs *pcontainer;
		std::vector <T*> v;
	public:
		size_t size() const {return v.size();}
		const T* const operator[](const unsigned int i) const
		{
			if (v.size() > i) return *(v.begin() + i);
			else return NULL;
		}
	};
	 /*base class for CNamed and CUnusedNamed*/
	class CBaseClassForAllNamed
	{
		friend class CStlCmdLineArgs;
		void appendQuote(std::string & s) { s.append("\""); }
		void appendQuote(std::wstring & s) { s.append(L"\""); }
		void appendSpace(std::string & s) { s.append(" "); }
		void appendSpace(std::wstring & s) { s.append(L" ");	}
	public: std::map <STR, T*> m;
	protected:
		CStlCmdLineArgs *pcontainer;
	public:

		size_t size() const {return m.size();}
		/*
		Why bother with enumeration?
		This toString method should suffice.
		*/
		STR toString()
		{
			STR s;
			//typedef std::map<STR, T*, std::less<STR>, std::allocator<std::pair <const STR, T*> > > mymap;
			for ( typename std::map<STR, T*>::const_iterator it = this->m.begin(); it != this->m.end(); ++it )
			{
				appendQuote(s);
				//s += pcontainer->optionsCharacter;
				s.append(it->first.data());
				appendQuote(s);
				appendSpace(s);
			}
			return s;
		}
	};
	class CNamed : public CBaseClassForAllNamed
	{
	public:
		const T* const operator[](const STR key) /*const*/
		{
			if ( this->m.find(key) != this->m.end())
			{
				this->pcontainer->unusednamed.m.erase(key);
				return this->m[key];
			}
			else return NULL;
		}
	};
	class CUnusedNamed : public CBaseClassForAllNamed
	{
	public:
		const T* const operator[](const STR key) /*const*/
		{
			if ( this->m.find(key) != this->m.end())
			{
				return this->m[key];
			}
			else return NULL;
		}
	};
public:
	CNamed named;
	CUnusedNamed unusednamed;
	CUnnamed unnamed;
protected:
	/* That's why it is protected.*/
	CStlCmdLineArgs(){};
	/*To be called from constructors. 
	Do we need default values here? No!!! I removed them. They helped me to create
	a bug by allowing me to call Init without these parameters!*/
	void Init(int argc, T* argv[], T optionsCharacter, T delimiter)
	{
		unnamed.pcontainer = this;
		named.pcontainer = this;
		unusednamed.pcontainer = this;

		this->delimiter = delimiter;
		this->optionsCharacter = optionsCharacter;
		this->terminatingCharacter = '\0';

		for (int i=0; i<argc; i++)
		{
			if (argv[i][0] != optionsCharacter) //unnamed arguments
			{
				unnamed.v.push_back(argv[i]);
			}
			else //named arguments
			{
				T* delimiter_position = myinstr(argv[i]+1, delimiter);
				if (delimiter_position != NULL) //myinstr je našla delimiter
				{

					if (delimiter_position == argv[i]+1) //delimiter je odmah iza '/'
					{
						unnamed.v.push_back(argv[i]+2);
					}
					else
					{
						named.m[STR(argv[i]+1, delimiter_position-argv[i]-1)] = delimiter_position+1;
						unusednamed.m[STR(argv[i]+1, delimiter_position-argv[i]-1)] = delimiter_position+1;
					}
				}
				else
				{
					named.m[argv[i]+1] = (T*)&terminatingCharacter; //put empty string in the map
					unusednamed.m[argv[i]+1] = (T*)&terminatingCharacter; //put empty string in the map
				}
			}
		}
	}

public:
	CStlCmdLineArgs(int argc, T* argv[], T optionsCharacter = '/', T delimiter = ':')
	{
		Init(argc, argv, optionsCharacter, delimiter);
	}


	const T* const operator[](const unsigned int i) /*const*/
	{
		if (unnamed.v.size() > i) return unnamed.v[i];
		else return NULL;
	}
	const T* const operator[](const STR key) {
		if ( named.m.find(key) != named.m.end())
		{
			unusednamed.m.erase(key);
			return named.m[key];
		}
		else return NULL;
	}

};

template <typename T> class CStlCmdLineArgsWin : public CStlCmdLineArgs<T>
{
	CArgList<T> argList;
public:
	CStlCmdLineArgsWin(T optionsCharacter = '/', T delimiter = ':')
	{
// original code commented out this line...	  	
      Init(argList.argc, argList.argv, optionsCharacter, delimiter);
// and used this line here. But this causes a memory leak for whatever reason...		
        //this->CStlCmdLineArgs::CStlCmdLineArgs(argList.argc, argList.argv, optionsCharacter, delimiter);
	}
	
    CStlCmdLineArgsWin(const T* commandLine, T optionsCharacter = '/', T delimiter = ':')
      : argList(commandLine)
	{
// original code commented out this line...	  	
      Init(argList.argc, argList.argv, optionsCharacter, delimiter);
// and used this line here. But this causes a memory leak for whatever reason...		
        //this->CStlCmdLineArgs::CStlCmdLineArgs(argList.argc, argList.argv, optionsCharacter, delimiter);
	}
};
