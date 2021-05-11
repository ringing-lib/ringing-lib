// -*- C++ -*- libraries.h - singleton containing the method libraries
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


#ifndef METHSEARCH_LIBRARIES_INCLUDED
#define METHSEARCH_LIBRARIES_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/methodset.h>
#include <string>

// Forward declare ringing::method
RINGING_START_NAMESPACE
class method;
RINGING_END_NAMESPACE

RINGING_USING_NAMESPACE
RINGING_USING_STD

class method_libraries : public methodset
{
public:
  static void add_new_library( const string &name );
  static void init();
  static bool has_libraries();
  static method lookup_method( const method &m );

  static method_libraries &instance();

private:
  void read_libraries();
  method_libraries();

  bool done_init;
  vector<string> library_names;
};

// Call without arguments to fetch and with arguments to load
library const& 
overwork_map( int bells = 0, const string& filename = string() );

library const&
underwork_map( int bells = 0, const string& filename = string() );

#endif // METHSEARCH_LIBRARIES_INCLUDED
