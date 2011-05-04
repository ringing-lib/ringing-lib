// rowcalc.cpp - Entry point for row calculator
// Copyright (C) 2009, 2010, 2011
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
#include <ringing/streamutils.h>
#include <ringing/row.h>

#include "init_val.h"
#include "row_calc.h"
#include "args.h"
#include "console_stream.h"
#include "stringutils.h"

#include <iostream>
#include <exception>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  arguments( int argc, char *argv[] );

  init_val<bool, false>  interactive;
  init_val<bool, false>  count;
  init_val<int, 0>       bells;
  init_val<bool, false>  accept_changes;

  vector<string>         expr;

private:
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt
         ( 'b', "bells",
           "The number of bells.  This option is required", "BELLS",
           bells ) );

  p.add( new boolean_opt
         ( 'c', "count",
           "Count the number of rows instead of printing them",
           count ) );

  p.add( new boolean_opt
         ( 'i', "interactive",
           "Run in interactive mode",
           interactive ) );

  p.add( new boolean_opt
         ( 'C', "accept-changes",
           "Allow changes in row expressions",
           accept_changes ) );

  p.set_default( new strings_opt( '\0', "", "", "", expr ) );
}

bool arguments::validate( arg_parser& ap )
{
  if ( bells < 0 || bells > int(bell::MAX_BELLS) )
    {
      ap.error( make_string() << "The number of bell must be greater than "
                "0 and less than " << bell::MAX_BELLS+1 );
      return false;
    }

  if ( accept_changes && !bells )
    {
      ap.error( make_string() << "The number of bells must be provided if -C "
                "is used" );
      return false;
    }
  return true;
}

arguments::arguments( int argc, char *argv[] )
{
  arg_parser ap( argv[0], "rowcalc -- a calculator for rows", "OPTIONS" );
  bind( ap );

  if ( !ap.parse(argc, argv) )
    {
      ap.usage();
      exit(1);
    }

  if ( !validate(ap) )
    exit(1);
}

bool evaluate_expr( arguments const& args, string const& expr )
{
  row_calc::flags flags; 
  if (args.accept_changes) 
    flags = row_calc::allow_raw_changes;
  else
    flags = static_cast<row_calc::flags>( row_calc::allow_implicit_treble 
                                        | row_calc::allow_row_promotion );

  scoped_pointer<row_calc> rc;
  try {
    // If bells == 0 this is equivalent to omitting that argument.
    rc.reset( new row_calc( args.bells, expr, flags ) );
  } 
  catch ( exception const& e ) { 
    cerr << "Error parsing expression: " << e.what() << "\n";
    return false;
  }

  try {  
    if ( args.count )
      cout << distance( rc->begin(), rc->end() ) << "\n";
    else
      copy( rc->begin(), rc->end(), ostream_iterator<row>(cout, "\n") );
  }
  catch ( row::invalid const& ex ) {
    cerr << "Invalid row produced: " << ex.what() << "\n";
    return false;
  }

  return true;
}

int main( int argc, char *argv[] )
{
  bell::set_symbols_from_env();

  arguments args( argc, argv );

  if ( args.expr.empty() ) {
    console_istream in( args.interactive );
    in.set_prompt( "> " );
    string expr;
    while ( getline( in, expr ) ) {
      trim_whitespace(expr);
      if ( expr.size() )
        evaluate_expr( args, expr );
    }
    if (args.interactive)
      cout << "\n";
  }
  else for ( vector<string>::const_iterator 
               i=args.expr.begin(), e=args.expr.end(); i != e; ++i )
    if ( !evaluate_expr( args, *i ) )
      return 1;
}
