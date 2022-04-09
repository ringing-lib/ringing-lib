// -*- C++ -*- libraries.cpp - singleton containing the method libraries
// Copyright (C) 2002, 2009, 2010, 2021, 2022 
// Richard Smith <richard@ex-parrot.com>

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
#include "format.h"
#include <string>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
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
#include <ringing/litelib.h>
#include <ringing/cclib.h>
#include <ringing/mslib.h>
#include <ringing/xmllib.h>
#include <ringing/methodset.h>


RINGING_USING_NAMESPACE
RINGING_USING_STD

method_libraries &method_libraries::instance()
{
  static method_libraries tmp;
  return tmp;
}

method_libraries::method_libraries()
  : done_init(false)
{
}

bool method_libraries::has_libraries()
{
  return !instance().library_names.empty() || getenv("METHOD_LIBRARY");
}

void method_libraries::add_new_library( const string &filename )
{
  instance().library_names.push_back( filename );
  instance().done_init = false;
}

void method_libraries::read_libraries()
{
  clear();

  if ( formats_have_cc_ids() )
    store_facet< cc_collection_id >();

  try {
    if ( instance().library_names.empty() )
      import_libraries_from_env();

    for ( vector< string >::const_iterator 
            i( instance().library_names.begin() ), 
            e( instance().library_names.end() );
          i != e; ++i )
      import_library( *i );
  }
  catch ( const exception &e ) {
    cerr << "Error reading method libraries: " << e.what() << '\n';
    exit(1);
  }
}

void method_libraries::init()
{
  if ( !instance().done_init && has_libraries() )
    {
      // Register mslib last, as various things can accidentally match it
      cclib::registerlib();
      xmllib::registerlib();
      mslib::registerlib();

      library::setpath_from_env();
      
      instance().read_libraries();

      instance().done_init = true;
    }
}

method method_libraries::lookup_method( const method &m ) {
  library_entry i( instance().find( m ) );
  if ( ! i.null() )
    return i.meth();
  else 
    return m;
}

bool method_libraries::has_method( const method &m ) {
  return !instance().find( m ).null();
}

library const& overwork_map( int bells, const string& filename ) {
  static methodset out;
  if ( bells ) {
    litelib in( bells, filename );
    out = methodset();
    out.store_facet<litelib::payload>();
    out.append( in.begin(), in.end() );
  }
  return out;
}

library const& underwork_map( int bells, const string& filename ) {
  static methodset out;
  if ( bells ) {
    litelib in( bells, filename );
    out = methodset();
    out.store_facet<litelib::payload>();
    out.append( in.begin(), in.end() );
  }
  return out;
}


