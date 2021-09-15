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
#include <ringing/methodset.h>
#include <ringing/method_stream.h>
#include <ringing/streamutils.h>

#include <map>
#include <iostream>

#include "init_val.h"
#include "args.h"


RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  arguments( int argc, char const *argv[] );

  init_val<int,0>       bells;
  vector<string>        titles;
  init_val<bool,false>  read_titles;
  init_val<bool,false>  inc_bells;
  vector<string>        libs;

private:
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt
           ( 'b', "bells",  "The number of bells.", "BELLS", bells ) );

  p.add( new strings_opt
           ( 't', "title",  "Find method with this full title.", "TITLE", 
             titles ) );

  p.add( new boolean_opt
           ( 'T', "read-titles",  "Read titles to find from standard input.",
             read_titles ) );

  p.add( new boolean_opt
           ( 'B', "print-bells",  "Include the number of bells in the output",
             inc_bells ) );

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

  if ( read_titles && titles.size() ) {
    ap.error( "The -t and -T options cannot be used together" );
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

void read_library( const string &filename, libout& out ) {
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

    copy( l.begin(), l.end(), back_inserter(out) );
  }
  catch ( const exception &e ) {
    cerr << "Unable to read library: " << filename << ": " 
         << e.what() << '\n';
    exit(1);
  }
}

void read_titles( arguments& args ) {
  string line;
  while ( getline( cin, line ) ) {
    size_t last = line.find_last_not_of(" \t\f\v\n\r");
    if ( last != string::npos )
      args.titles.push_back( line.substr(0, last+1) );
  }
}

int main(int argc, char const** argv) {
  arguments args( argc, argv );

  if (args.read_titles) read_titles(args);

  // Register mslib last, as various things can accidentally match it
  cclib::registerlib();
  xmllib::registerlib();
  mslib::registerlib();

  library::setpath_from_env();

  methodset meths;
  for ( vector< string >::const_iterator 
          i( args.libs.begin() ), e( args.libs.end() ); i != e; ++i )
    read_library( *i, meths );
  
  method_stream out(args.inc_bells);
  
  bool okay = true;
  for ( vector< string >::const_iterator 
          i( args.titles.begin() ), e( args.titles.end() ); i != e; ++i ) {
    library_entry le = meths.find(*i);
    if (le.null()) {
      cerr << "Method not found: " << *i << endl;
      okay = false;
    }

    out.append(le);
  }
 
  return okay ? 0 : 1;
}
