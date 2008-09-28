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
 
// ODINTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "cppunit/CompilerOutputter.h"
#include "cppunit/TextOutputter.h"
#include "cppunit/extensions/TestFactoryRegistry.h"
#include "cppunit/ui/text/TestRunner.h"

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	  // Get the top level suite from the registry
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

  // Adds the test to the list of test to run
  CppUnit::TextUi::TestRunner runner;
  
  // to run all tests use:
  runner.addTest( suite );

  // Change the default outputter to a compiler error format outputter
  //runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(),
  //                                                     std::cerr ) );
  runner.setOutputter( new CppUnit::TextOutputter( &runner.result(),
                                                       std::cerr ) );
  
  bool wasSuccessful;
  // Run all tests.
  wasSuccessful = runner.run("", true);
  // Run a single test:
  //wasSuccessful = runner.run("ConfigTest", true);
  //wasSuccessful = runner.run("ODINManagerTest", true);
  //wasSuccessful = runner.run("ExceptionTest", true);
  //wasSuccessful = runner.run("ImageTest", true);

  // Return error code 1 if the one of test failed.
  //cout << "Press <return> to continue...";
  //char ch = cin.get();
  return wasSuccessful ? 0 : 1; 

}

