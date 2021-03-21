// -*- C++ -*- psline.cpp - print out lines for methods
// Copyright (C) 2021 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>
#include <ringing/library.h>
#include <ringing/cclib.h>
#include <ringing/mslib.h>
#include <ringing/xmllib.h>
#include <ringing/method_stream.h>
#include <ringing/streamutils.h>

#include <iostream>

#include "init_val.h"
#include "args.h"


RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  arguments( int argc, char const *argv[] );

  init_val<int,0> bells;

  vector<string>  libs;

private:
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt( 'b', "bells",  "The number of bells.", "BELLS", 
                          bells ) );

  p.set_default( new strings_opt( '\0', "", "", "", libs ) );
}

bool arguments::validate( arg_parser& ap )
{
  if ( libs.empty() ) {
    ap.error( "Please provide at least one library" );
    return false;
  }

  if ( bells == 1 || bells > int(bell::MAX_BELLS) ) {
    ap.error( make_string() << "The number of bells must be between 2 and " 	              << bell::MAX_BELLS << " (inclusive)" );
    return false;
  }

  return true;
}

arguments::arguments( int argc, char const *argv[] )
{
  arg_parser ap( argv[0], "methodlib -- extract methods from a method library",
                 "OPTIONS" );
  bind( ap );

  if ( !ap.parse(argc, argv) ) {
    ap.usage();
    exit(1);
  }

  if ( !validate(ap) )
    exit(1);
}

bool filter_method( const arguments& args, const method& meth ) {
  if ( args.bells && meth.bells() != args.bells )
    return false;
  return true;
}

void read_library( const arguments& args, const string &filename, 
                   libout& out ) {
  try {
   library l( filename );

    if (!l.good()) {
      cerr << "Unable to read library " << filename << "\n";
      exit(1);
    }

    if ( l.empty() ) {
      cerr << "The library " << filename << " appears to be empty\n";
      exit(1);
    }

    for ( library::const_iterator i(l.begin()), e(l.end()); i != e; ++i )
      if ( filter_method(args, i->meth()) )
        out.append(*i);
  }
  catch ( const exception &e ) {
    cerr << "Unable to read library: " << filename << ": " 
         << e.what() << '\n';
    exit(1);
  }
}

int main(int argc, char const** argv) {
  arguments args( argc, argv );

  // Register mslib last, as various things can accidentally match it
  cclib::registerlib();
  xmllib::registerlib();
  mslib::registerlib();

  library::setpath_from_env();

  method_stream out;
  
  for ( vector< string >::const_iterator 
          i( args.libs.begin() ), e( args.libs.end() ); i != e; ++i )
    read_library( args, *i, out );
}
