// -*- C++ -*- testbase.cpp - test the base classes
// Copyright (C) 2001 Mark Banner <mark@standard8.co.uk>

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// $Id$


// Note:  If you have compiled this with Microsoft Visual C++ and are unable to
// get it to read file names and/or if you find that you have to press return
// twice after each line of input, this may be a symptom of a bug in <istream>
// in the version of the Dinkumware STL that is supplied with versions of MSVC
// between versions 4.2 and 6 of Visual Studio.   A fix is avaliable 
// from the Dinkumware website ( http://www.dinkumware.com/vc_fixes.html ) 
// which supposedly solves the problem.  


#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <stdexcept.h>
#else
#include <iostream>
#include <stdexcept>
#endif
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/cclib.h>
#include <ringing/mslib.h>
#include <string>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

void print_row_block(const row_block& b)
{
  cout << "\t" << b[0] << " " 
       << ((b[0].sign() < 0) ? '-' : '+') << " " << endl;
  for(unsigned int i = 1;i < b.size();i++)
    cout << b.get_changes()[i-1] << "\t" << b[i]  << " " 
	 << ((b[i].sign() < 0) ? '-' : '+') << " " << endl;
}

string trim_whitespace( const string &str )
{
  const char *whitespace = " \t\r\n";

  size_t first( str.find_first_not_of( whitespace ) );
  if ( first == string::npos ) 
    return string();
  
  size_t last( str.find_last_not_of( whitespace ) );
  if ( last == string::npos ) 
    return string( str, first, string::npos );
  
  return string( str, first, last - first + 1 );
}

int main()
{
  cout << "\nTesting method libraries...\n";

  mslib::registerlib();
  cclib::registerlib();

#if defined(SEPERATE_FILES)
  {
    // First run the seperation routine
    string dirname;

    cout << "Attempt to seperate cc library files - WARNING: backup first!\n";
    cout << "\nFirst enter the directory of the cc method collection\n"
	 << "or just press enter\n";
    getline(cin, dirname);

    switch (cclib::seperatefiles(dirname))
      {
      case 0:
	cout << "Files were successfully modified\n";
	break;
      case -1:
	cerr << "Unsuccessful in opening directory\n";
	break;
      case 1:
	cout << "No modifications required\n";
	break;
      default:
	cerr << "ERROR: This shouldn't have occured\n";
      }
  }
#endif

  {
    string filename;

    cout << "\nFirst enter a file that does not exist: ";
    getline(cin, filename);

    library l(trim_whitespace( filename ).c_str());
    if (!l.good())
      {
	cout << "Good, you entered a file that does not exist\n";
      }
    else
      {
	cout << "Are you sure this isn't an existing method library file?\n";
      }
  }

  {
    string filename;
    
    cout << "\nNow enter a file that is not a method library: ";
    getline(cin, filename);

    library l(trim_whitespace( filename ).c_str());
    if (!l.good())
      {
	cout << "Good, I have no valid libraries registered for that file\n";
      }
    else
      {
	cout << "If this is not a method library file, please email it to the authors as we have a bug.\n";
      }
  }

  {
    string filename;
    
    cout << "\nNow enter a valid method library file name: ";
    getline(cin, filename);

    library l(trim_whitespace( filename ).c_str());
    if (!l.good())
      {
	cerr << "I cannot read that file.\n";
	return 1;
      }
    else
      {
	cout << "A library can read that file.\n";

	string input;

	cout << "\nDo you wish to list the methods (y/n)? ";
	getline(cin, input);

	if ((input[0] == 'y') || (input[0] == 'Y'))
	  {
	    list<string> mylist;
	    if (l.dir(mylist) == 0)
	      {
		// Either Library doesn't support the function, or there were
		// no methods present - this second one shouldn't happen
		// of course.
		cerr << "Sorry, Could not list files in library\n";
	      }
	    else
	      {
		cout << "\nMethod list:\n\n";
		list<string>::iterator i;
		for(i = mylist.begin(); i != mylist.end(); i++)
		  {
		    cout << *i << endl;
		  }
	      }
	  }

	cout << "\nTesting multiple loads of methods...\n";
	string methname;
	cout << "\nPlease enter a method name: ";
	getline(cin, methname);

#if RINGING_USE_EXCEPTIONS
	try
#endif
	  {
	    method m(l.load(trim_whitespace( methname ).c_str()));
	    
	    cout << m.fullname() << endl;
	    
	    print_row_block((row_block)m);
	    
	    cout << "This method is "
		 << ((m.isregular()) ? "" : "ir")
		 << "regular"
		 << ((m.isdouble()) ? " and double" : "")
		 << ".\n";
	    cout << "Lead head code: " << m.lhcode() << endl;
	  }
#if RINGING_USE_EXCEPTIONS
	catch (exception &e)
	  {
	    cerr << "Error cannot load method: " << e.what() << endl;
	  }
#endif

	cout << "Please enter another method name: ";
	getline(cin, methname);

#if RINGING_USE_EXCEPTIONS
	try
#endif
	  {
	    method m(l.load(trim_whitespace( methname ).c_str()));
	    
	    cout << m.fullname() << endl;
	    
	    print_row_block((row_block)m);
	    
	    cout << "This method is "
		 << ((m.isregular()) ? "" : "ir")
		 << "regular"
		 << ((m.isdouble()) ? " and double" : "")
		 << ".\n";
	    cout << "Lead head code: " << m.lhcode() << endl;
	  }
#if RINGING_USE_EXCEPTIONS
	catch (exception &e)
	  {
	    cerr << "Error cannot load method: " << e.what() << endl;
	  }
#endif
      }
  }
    
  return 0;
}
 
