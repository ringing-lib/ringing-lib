// -*- C++ -*- test-base.cpp - Framework for testing the ringing class library 
// Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>
#include <ringing/streamutils.h>
#include "test-base.h"
#include <string>
#if RINGING_OLD_INCLUDES
#include <typeinfo.h>
#include <iostream.h>
#include <vector.h>
#include <utility.h>
#else
#include <typeinfo>
#include <iostream>
#include <vector>
#include <utility>
#endif


RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_NAMESPACE_TEST


ostream &operator<<( ostream &os, const location &loc )
{
  os << loc.file << ":" << loc.line << ":";
  return os;
}
  
test_failure::test_failure( const location &loc, const string &str )
  : domain_error( make_string() << loc << " " << str )
{}

condition_failure::condition_failure( const location &loc, 
				      const char *condition )
  : test_failure( loc, make_string() << "Test failed: " << condition )
{}

missing_exception::missing_exception( const location &loc, 
				      const char *statement )
  : test_failure( loc, make_string() << "Statement failed to throw: "
		  << statement )
{}

unexpected_exception::unexpected_exception( const location &loc,
					    const exception &e )
  : test_failure( loc, make_string() << "Caught exception of type '" 
		  << typeid( e ).name() << "' (" << e.what() << ")" )
{}

unexpected_exception::unexpected_exception( const location &loc )
  : test_failure( loc, make_string() << "Caught unknown exception" )
{}


RINGING_START_ANON_NAMESPACE

class test_engine
{
private:
  test_engine() {}
public:
  // MSVC <= 6 and EGCS compilers need a public destructor in 
  // a singleton class.
 ~test_engine() {}

  static test_engine &instance() 
  {
    static test_engine tmp;
    return tmp;
  }

  typedef vector< pair< string, void (*)(void) > > test_vector;
  test_vector tests;
};

RINGING_END_ANON_NAMESPACE

void register_test( const char *str, void (*test)(void) ) 
{
  test_engine::instance().tests.push_back( pair<string, void (*)()>( str, test ) ); 
}

bool run_tests( bool verbose )
{
  unsigned int failures(0u);

  for ( test_engine::test_vector::const_iterator 
	  i( test_engine::instance().tests.begin() ),
	  e( test_engine::instance().tests.end() );  i != e; ++i )
    {
      try 
	{
	  if ( verbose ) cout << i->first << endl;
	  (*i->second)();
	}
      catch ( const test_failure &e )
	{
	  ++failures;
	  cout << e.what() << endl;
	}
      catch ( const exception &e )
	{
	  ++failures;
	  cout << "Whilst running " << i->first 
	       << " an unexpected exception occured: '" 
	       << typeid(e).name() << "' (" << e.what() << ")" << endl;
	}
      catch ( ... )
	{
	  ++failures;
	  cout << "Whilst running " << i->first 
	       << " an unexpected exception of unknown type occured." << endl;
	}
    }

  if ( failures == 0 )
    {
      cout << "All tests succeeded" << endl;
      return true;
    }
  else
    {
      cout << failures << " tests failed" << endl;
      return false;
    }
}

// Move a bit of the exception handling out of line
void handle_exception( const location& loc )
{
  // This is always called during execution of a catch handler,
  // therefore we can perfectly safely rethrow and catch the exception
  // internally to recover type information:
  try 
    {
      throw;
    }
  catch ( const test_failure& )
    {
      throw;
    }
  catch ( const exception& e )
    {
      throw unexpected_exception(loc, e);
    }
  catch ( ... )
    {
      throw unexpected_exception(loc);
    }
}

RINGING_END_NAMESPACE_TEST

RINGING_END_NAMESPACE

