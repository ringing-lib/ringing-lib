// -*- C++ -*- libraries.cpp - singleton containing the method libraries
// Copyright (C) 2002, 2009 Richard Smith <richard@ex-parrot.com>

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

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "libraries.h"
#include <string>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <set.h>
#include <list.h>
#else
#include <vector>
#include <set>
#include <list>
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#else
#include <iostream>
#endif
#if RINGING_OLD_C_INCLUDES
#include <cstdlib.h>
#else
#include <cstdlib>
#endif
#include <ringing/method.h>
#include <ringing/library.h>
#include <ringing/cclib.h>
#include <ringing/mslib.h>
#include <ringing/xmllib.h>


RINGING_USING_NAMESPACE
RINGING_USING_STD

struct method_libraries::impl
{
  // Helper functions
  void read_library( const string &name );

  // Data members
  bool init;
  vector< string > library_names;
  set< method > named_methods;
};

method_libraries::impl &method_libraries::instance()
{
  static method_libraries tmp;
  return *tmp.pimpl;
}

method_libraries::method_libraries()
  // Passing the second argument here is a work-around for an MSVC 5
  // bug.   See the comment on the constructor for more details.
  : pimpl( new impl, delete_helper<impl>::fn )
{
  pimpl->init = false;
}

method_libraries::~method_libraries()
{
}

bool method_libraries::has_libraries()
{
  return !instance().library_names.empty();
}

void method_libraries::add_new_library( const string &name )
{
  instance().library_names.push_back( name );
  instance().init = false;
}

void method_libraries::impl::read_library( const string &filename )
{
  try
    {
      library l( filename );

      if (!l.good())
	{
	  cerr << "Unable to read library " << filename << "\n";
	  exit(1);
	}

      // TODO -- this can now use library iterators

      list< method > ms;
      int count = l.mdir( ms );
      if ( count == 0 )
	{
	  cerr << "The library " << filename << " appears to be empty\n";
	  exit(1);
	}

      copy( ms.begin(), ms.end(), 
	    inserter( named_methods, named_methods.begin() ) );
    }
  catch ( const exception &e )
    {
      cerr << "Unable to read library: " << filename << ": " 
	   << e.what() << '\n';
      exit(1);
    }

}

void method_libraries::init()
{
  if ( !instance().init && has_libraries() )
    {
      cclib::registerlib();
      mslib::registerlib();
      xmllib::registerlib();

      if ( char const* const methlibpath = getenv("METHLIBPATH") )
        library::setpath( methlibpath );
      
      instance().named_methods.clear();

      for ( vector< string >::const_iterator 
	      i( instance().library_names.begin() ), 
	      e( instance().library_names.end() );
	    i != e; ++i )
	{
	  instance().read_library( *i );
	}

      instance().init = true;
    }
}

const method &method_libraries::lookup_method( const method &m )
{
  set<method>::const_iterator i( instance().named_methods.find( m ) );
  if ( i != instance().named_methods.end() )
    return *i;
  else 
    return m;
}


