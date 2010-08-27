// util.cpp - Various string utility functions
// Copyright (C) 2002, 2008, 2010 Richard Smith <richard@ex-parrot.com>

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

#include "stringutils.h"
#include <string>
#include <ringing/streamutils.h>
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <cstdlib.h>
#else
#include <cctype>
#include <cstdlib>
#endif
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif

RINGING_USING_NAMESPACE

string lower_case( const string& str )
{
  string s(str);
  for ( string::iterator i(s.begin()), e(s.end()); i != e; ++i )
    *i = tolower(*i);
  return s;
}

void trim_leading_whitespace( string &line )
{
  string::iterator i( line.begin() ), e( line.end() );
  while ( i != e && isspace(*i) ) ++i;
  line = string( i, e );
}

void trim_trailing_whitespace( string &line )
{
  string::iterator b( line.begin() ), j( line.end() );
  while ( j-1 != b && isspace(*(j-1)) ) --j;
  line = string( b, j );
}

void trim_whitespace( string &line )
{
  trim_leading_whitespace( line );
  trim_trailing_whitespace( line );
}

int string_to_int( const string &num )
{
  char *nptr;
  int n = strtol( num.c_str(), &nptr, 10 );

  for ( string::const_iterator i = num.begin() + (nptr - num.c_str()); 
	i != num.end();  ++i )
    if ( !isspace(*i) )
      throw runtime_error( make_string() << "'" << num << "'"
			   " is not an integer" );

  return n;
}
