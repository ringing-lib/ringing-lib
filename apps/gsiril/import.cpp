// import.cpp - Code to load other gsiril files
// Copyright (C) 2002, 2003, 2004, 2007, 2008, 2010
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

#include <ringing/pointers.h>
#include <string>
#if RINGING_OLD_INCLUDES
#include <list.h>
#else
#include <list>
#endif
#if RINGING_OLD_IOSTREAMS
#include <istream.h>
#include <fstream.h>
#else
#include <istream>
#include <fstream>
#endif

RINGING_USING_NAMESPACE

RINGING_START_ANON_NAMESPACE

// XXX: This code is also in ringing/library.cpp 
list<string> split_path( string const& p )
{
  list<string> pp;
  string::size_type i=0;
  while (true) {
    string::size_type j = p.find(':', i);
    if ( j == string::npos ) {
      if ( p.size() ) pp.push_back( p.substr(i) );
      break;
    }
    
    pp.push_back( p.substr(i, j-i) );
    i = j+1;
  }
  return pp;
}



shared_pointer<istream> try_load_file( string const& name ) 

{
  shared_pointer<istream> in( new ifstream(name.c_str()) );
  if ( !*in ) {
    string filename( name ); filename += ".gsir";
    in.reset( new ifstream(filename.c_str()) );
  }
  if ( !*in ) {
    string filename( name ); filename += ".sir";
    in.reset( new ifstream(filename.c_str()) );
  }
  if ( !*in )
    in.reset();
  return in;
}

RINGING_END_ANON_NAMESPACE

shared_pointer<istream> load_file( string const& filename )
{
  // XXX: Some code here is common with that in ringing/library.cpp
  char const sep = RINGING_WINDOWS ? '\\' : '/';

  bool isabs = false;
  if ( filename[0] == sep ) isabs = true;
#if RINGING_WINDOWS
  if ( filename.size() >= 3 && filename[1] == ':' && filename[2] == sep )
    isabs = true;
#endif

  shared_pointer<istream> in( try_load_file(filename) );
  if ( !in && !isabs )
  {
    string libpath;
    if ( char const* const gslibpath = getenv("GSIRIL_LIBRARY_PATH") )
      libpath = gslibpath;
    list<string> locs( split_path(libpath) );
    for (list<string>::const_iterator i=locs.begin(), e=locs.end(); i!=e; ++i) 
      if ( in = try_load_file( *i + sep + filename ) )
        break;
  }
  return in;
}



