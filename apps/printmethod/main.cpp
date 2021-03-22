// main.cpp - Entry point for printmethod
// Copyright (C) 2008, 2009, 2010, 2011, 2021
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

#include <ringing/common.h>

#include <iostream>
#include <map>
#include <string>
#include <cassert>

#if RINGING_USE_TERMCAP
# include <curses.h>
# include <term.h>
# ifdef bell
#   undef bell
# endif
# define RINGING_TERMINFO_VAR( name ) \
    ( cur_term && (name) != (char const*)-1 ? (name) : NULL )
#else
# define RINGING_TERMINFO_VAR( name ) NULL
static inline char* tparm( char const*, ... ) { return NULL; }
#endif

#include "args.h"
#include "init_val.h"
#include "bell_fmt.h"

#include <ringing/bell.h>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/place_notation.h>
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct bellfmt { 
  int colour;
  bool bold;
};

struct arguments
{
  init_val<int,0>      bells;

  init_val<bool,false> whole_course;

  init_val<int,0>      init_rounds;
  init_val<int,0>      final_rounds;
  init_val<bool,false> omit_final;
  init_val<bool,false> omit_start;

  string methstr;
  method meth;
  
  row startrow;

  string               rstr, gstr, bstr;
  bell_fmt             bellfmt;
 
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
         ( 'c', "course",
           "Print a whole course",
           whole_course ) );

  p.add( new boolean_opt
         ( 'F', "omit-final",
           "Omit the final row to avoid duplication of the lead- or "
           "course-head",
           omit_final ) );

  p.add( new boolean_opt
         ( 'S', "omit-start",
           "Omit the starting row to avoid duplication of the lead- or "
           "course-head",
           omit_start ) );

  p.add( new integer_opt
         ( 'r', "init-rounds",
           "Start with NUM whole blows of rounds", "NUM",
            init_rounds ) );

  p.add( new integer_opt
         ( 'f', "final-rounds",
           "Finish with NUM whole blows of rounds", "NUM",
            final_rounds ) );

  p.add( new row_opt
         ( 's', "start", 
           "Starting from ROW", "ROW",
           startrow ) );

#if RINGING_USE_TERMCAP
  p.add( new string_opt
         ( 'R', "red", "Colour BELLS in red", "BELLS", rstr ) );
  p.add( new string_opt
         ( 'G', "green", "Colour BELLS in green", "BELLS", gstr ) );
  p.add( new string_opt
         ( 'B', "blue", "Colour BELLS in blue", "BELLS", bstr ) );
#endif

  p.set_default( new string_opt( '\0', "", "", "", methstr ) );
}

bool arguments::validate( arg_parser &ap )
{
  if ( bells == 0 ) 
    {
      ap.error( "Must specify the number of bells" );
      return false;
    }

  try {
    meth = method( methstr, bells );
  } 
  catch ( bell::invalid const& ) {
    ap.error( make_string()
              << "Error: '" << methstr << "' contains an invalid bell" );
    return false;
  }
  catch ( change::invalid const& ) {
    ap.error( make_string()
              << "Error: '" << methstr << "' contains an invalid change" );
    return false;
  }
  catch ( place_notation::invalid const& ) {
    ap.error( make_string()
              << "Error: '" << methstr << "' is not a place notation" );
    return false;
  }

  if ( startrow.bells() == 0 ) {
    startrow = row(bells);
  }
  else if ( startrow.bells() != bells ) {
    ap.error( "Starting row is on wrong number of bells" );
    return false;
  }

#if RINGING_USE_TERMCAP
  try {
    bellfmt.set_colours(rstr, gstr, bstr);
  }
  catch (bell::invalid const&) {
    ap.error("Invalid bell in colour specification");
    return false;
  }
#endif

  return true;
}

int main(int argc, char* argv[]) 
{
  bell::set_symbols_from_env();

  arguments args;

  {
    arg_parser ap(argv[0], "printmethod -- print a method.", "OPTIONS" );
    
    args.bind( ap );
    
    if ( !ap.parse(argc, argv) ) 
      {
	ap.usage();
	return 1;
      }

    if ( !args.validate( ap ) ) 
      return 1;
  }

# if RINGING_USE_TERMCAP
  if ( !args.bellfmt.empty() )
    setupterm(NULL, 1, NULL);
# endif

  row r( args.startrow );  bool first = true;

  for ( int i=0; i<args.init_rounds*2; ++i)
    cout << args.bellfmt << r << "\n";

  do for ( method::const_iterator i=args.meth.begin(), e=args.meth.end(); 
           i!=e; ++i )  
  {
    if ( !( first && (args.omit_start || args.init_rounds) ) ) 
      cout << args.bellfmt << r << "\n";

    r *= *i;  first = false;
  } while ( r != args.startrow && args.whole_course );

  if (!args.omit_final && args.meth.size())
    cout << args.bellfmt << r << "\n";

  for ( int i=0; i<args.final_rounds*2; ++i)
    cout << args.bellfmt << r << "\n";
}


