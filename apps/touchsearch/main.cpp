// main.cpp - Entry point for touchsearch
// Copyright (C) 2002, 2003, 2007 Richard Smith <richard@ex-parrot.com>

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
#pragma implementation "gsiril/prog_args.h"
#endif

#include <ringing/change.h>
#include <ringing/method.h>
#include <ringing/streamutils.h>
#include <ringing/table_search.h>
#include <ringing/touch.h>

#include "args.h"
#include "init_val.h"
#include "prog_args.h"
#include "iteratorutils.h"

#include <string>
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#include <vector.h>
#else
#include <stdexcept>
#include <vector>
#endif
#if RINGING_OLD_IOSTREAMS
#include <iostream.h>
#include <fstream.h>
#else
#include <iostream>
#include <fstream>
#endif


RINGING_USING_NAMESPACE

arguments::arguments( int argc, char** argv )
  : length( 0u, static_cast<size_t>(-1) )
{
  arg_parser ap( argv[0], "touchsearch -- search for touches.",
                 "OPTIONS" );

  bind(ap);
  if ( !ap.parse(argc, argv) ) {
    ap.usage();
    exit(1);
  }

  if ( !validate(ap) )
    exit(1);
}

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.set_default( new string_opt( '\0', "", "", "", meth_str ) );
  
  p.add( new integer_opt
         ( 'b', "bells",
           "The default number of bells.",  "BELLS",
           bells ) );

  p.add( new strings_opt
         ( 'C', "call",
           "Specify a call",  "CHANGE",
           call_strs ) );

  p.add( new boolean_opt
         ( 'r', "ignore-rotations",
           "Filter touches only differing by a rotation",
           ignore_rotations ) );
 
  p.add( new range_opt
         ( 'l', "length",
           "Length or range of lengths required",  "MIN-MAX",
           length ) );
}

bool arguments::validate( arg_parser& ap )
{
  if ( !bells ) {
    ap.error( "Must specify the number of bells" );
    return false;
  }

  if ( bells < 4 || bells >= int(bell::MAX_BELLS) ) {
    ap.error( make_string() << "The number of bells must be between 4 and "
              << bell::MAX_BELLS-1 << " (inclusive)" );
    return false;
  }

  try {  
    meth = method( meth_str, bells );
  } catch ( const exception& e ) {
    ap.error( make_string() << "Invalid method place notation: "
              << e.what() );
    return false;
  }

  if ( meth.empty() ) {
    ap.error( "Must specify a method" );
    return false;
  }

  if ( !generate_pends( ap ) )
    return false;

  if ( !generate_calls( ap ) )
    return false;

  const size_t leadlen = meth.size();
  if ( length.first % leadlen )
    length.first = (length.first / leadlen + 1);
  else
    length.first /= leadlen;
  if ( length.second < static_cast<size_t>(-1) )
    length.second /= leadlen;

  if ( length.first > length.second ) {
    ap.error( "The length range does not encompass a whole number of leads" );
    return false;
  }
}

bool arguments::generate_calls( arg_parser& ap )
{
  // This is only a warning -- so don't return false
  if ( call_strs.empty() ) {
    call_strs.push_back( "14" );
    ap.error( "No calls specified -- assuming fourths place bobs");
  }

  for ( vector<string>::iterator
          i( call_strs.begin() ), e( call_strs.end() ); i != e; ++i )
    try {
      size_t eq = i->find('=');
      if ( eq != string::npos ) {
        change const ch( bells, i->substr(eq+1, string::npos) );
        calls.push_back( ch );
        i->erase(eq);
      } 
      else {
        change const ch( bells, *i );
        calls.push_back(ch);
      }
    }
    catch ( exception const& ex ) {
      ap.error( make_string() << "Unable to parse change '"
                << *i << "': " << ex.what() );
      return false;
    }

  return true;
}

bool arguments::generate_pends( arg_parser& ap )
{
  vector<row> gens;
  gens.reserve( pend_strs.size() );

  for ( vector<string>::const_iterator
          i( pend_strs.begin() ), e( pend_strs.end() ); i != e; ++i )
    {
      row const g( row(bells) * row(*i) );
      gens.push_back(g);
    }

  if ( gens.size() )
    pends = group( gens );
  else
    pends = group( row(bells) );

  // TODO: Warning if partends generate the extent? 
  // Or anything equally silly?

  return true;
}


// NB.  Once ostream &operator<<( ostream &, const touch & ) has
// been defined, there will be no need for this.

// A function object to attempt to sanely print the touch
class print_touch
{
public:
  typedef touch argument_type;
  typedef void result_type;
  
  print_touch( arguments const& args ) 
    : args(args), leadlen(args.meth.size()) 
  {}
    
  void operator()( const touch &t ) const
  {
    int n=0;
    for ( touch::const_iterator i( t.begin() ); i != t.end(); ++i )
      if ( ++n % leadlen == 0 ) {
        size_t cn = find( args.calls.begin(), args.calls.end(), *i ) 
          - args.calls.begin();
        if ( cn < args.calls.size() ) cout << args.call_strs[cn];
        else cout << '.';
      }

    cout << "  (" << n << " changes)" << endl;
  } 

private:
  arguments const& args;
  size_t leadlen;
};



int main( int argc, char *argv[] )
{
  try
    {
      arguments args( argc, argv );

      table_search srch( args.meth, args.calls, args.length, 
                         args.ignore_rotations );
      touch_search( srch, iter_from_fun( print_touch(args) ) );
      cout << endl;
    }
  catch ( const exception &ex )
    {
      cerr << "Unexpected error: " << ex.what() << endl;
      exit(1);
    }
  catch ( ... )
    {
      cerr << "An unknown error occured" << endl;
      exit(1);
    }

  return 0;
}

