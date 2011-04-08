// prog_args.cpp - Program argument parsing
// Copyright (C) 2002, 2003, 2007, 2009, 2010 
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
#pragma implementation "gsiril/prog_args.h"
#endif

#include "args.h"
#include "init_val.h"
#include "prog_args.h"

#include <ringing/change.h>
#include <ringing/method.h>
#include <ringing/group.h>
#include <ringing/streamutils.h>

#include <string>
#if RINGING_OLD_INCLUDES 
#include <vector.h>
#else
#include <vector>
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
           "Filter out touches only differing by a rotation",
           ignore_rotations ) );
 
  p.add( new range_opt
         ( 'l', "length",
           "Length or range of lengths required",  "MIN-MAX",
           length ) );

  p.add( new strings_opt
         ( 'P', "part-end",
           "Specify a part end",  "ROW",
           pend_strs ) );

  p.add( new boolean_opt
         ( 'm', "mutually-true-parts",
           "Only require mutually true parts "
           "without requiring them to be joined",
           mutually_true_parts ) );

  p.add( new boolean_opt
         ( 'R', "allow-non-round-blocks",
           "Allow true touches that do not come round ",
           round_blocks, false ) );

  p.add( new boolean_opt
         ( 'a', "use-plan",
           "Read a touch plan from standard input",
           use_plan ) );

  p.add( new boolean_opt
         ( 'q', "quiet",
           "Don't output the touches",
           quiet ) );

  p.add( new boolean_opt
         ( 'g', "comma-separate",
           "Comma separate the output (suitable for passing to gsiril)",
           comma_separate ) );

  p.add( new string_opt
         ( 'p', "plain-name",
           "Use STR instead of '.' or 'p' for plain leads", "STR",
           plain_name ) );

  p.add( new boolean_opt
         ( 'c', "count",
           "Count the number of touches found",
           count ) );

  p.add( new boolean_opt
         ( '\0', "raw-count", 
           "Count the number of touches found, "
           "and print it without surrounding text",
           raw_count ) );

  p.add( new integer_opt
         ( '\0', "limit",
           "Limit the search to the first NUM touches", "NUM",
           search_limit ) );

  p.add( new boolean_opt
         ( '\0', "filter",
           "Run as a filter on a method library",
           filter_mode ) );
}

bool arguments::validate( arg_parser& ap )
{
  if ( !bells ) {
    ap.error( "Must specify the number of bells" );
    return false;
  }

  if ( bells < 4 || bells > int(bell::MAX_BELLS) ) {
    ap.error( make_string() << "The number of bells must be between 4 and "
              << bell::MAX_BELLS << " (inclusive)" );
    return false;
  }

  try {  
    meth = method( meth_str, bells );
  } catch ( const exception& e ) {
    ap.error( make_string() << "Invalid method place notation: "
              << e.what() );
    return false;
  }

  if ( !filter_mode && !use_plan && meth.empty() ) {
    ap.error( "Must specify a method" );
    return false;
  }

  if ( !generate_pends( ap ) )
    return false;

  if ( !generate_calls( ap ) )
    return false;

  if ( meth.size() ) {
    // TODO:  Warn if no whole number of leads is possible

    if ( length.first > length.second ) {
      ap.error( "The minimum length is greater than the maximum length" );
      return false;
    }
    if ( use_plan ) {
      ap.error( "Must not specify a method when using a plan" );
      return false;
    }
  }

  if ( plain_name.empty() ) 
    plain_name = comma_separate ? 'p' : '.';
 
  return true;
}

bool arguments::generate_calls( arg_parser& ap )
{
  // This is only a warning -- so don't return false
  if ( call_strs.empty() ) {
    call_strs.push_back( "b=14" );
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


