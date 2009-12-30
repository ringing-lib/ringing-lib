// main.cpp - Entry point for printmethod
// Copyright (C) 2008, 2009 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/bell.h>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/place_notation.h>
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  init_val<int,0>      bells;

  init_val<bool,false> whole_course;

  init_val<int,0>      init_rounds;
  init_val<bool,false> omit_final;
  init_val<bool,false> omit_start;

  string methstr;
  method meth;
  
  row startrow;

  string           rstr, gstr, bstr;
  map< bell, int > colours;
 
  void bind( arg_parser& p );
  bool validate( arg_parser& p );

private:
  bool handle_colour( arg_parser& ap, string const& str, int val );
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
  } catch ( ... ) {
    try { throw; }
    catch ( bell::invalid const& ) {}
    catch ( change::invalid const& ) {}
    catch ( place_notation::invalid const& ) {}
    catch ( ... ) { throw; }
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
  // The COLOR_ macros are defined in ncurses.h
  if ( !handle_colour( ap, rstr, COLOR_RED   ) ) return false;
  if ( !handle_colour( ap, gstr, COLOR_GREEN ) ) return false;
  if ( !handle_colour( ap, bstr, COLOR_BLUE  ) ) return false;
#endif

  return true;
}

bool arguments::handle_colour( arg_parser& ap, string const& str, int val )
{
  for (string::const_iterator i=str.begin(), e=str.end(); i!=e; ++i)
    try {
      colours[ bell::read_char(*i) ] = val;
    } catch (bell::invalid const&) {
      ap.error( make_string()  
                << "Invalid bell: '" << *i << "' in colour specification" );
      return false;
    }

  return true;
}

// Print a row in colour
void print_row( arguments const& args, row const& r )
{
  if (args.colours.empty()) 
    cout << r;

  else {
    bool coloured = false;
    for (row::const_iterator i=r.begin(), e=r.end(); i!=e; ++i) {
      map<bell, int>::const_iterator c = args.colours.find(*i);
      if ( c == args.colours.end() ) {
        if ( coloured )
          if ( char const* seq = RINGING_TERMINFO_VAR( orig_pair ) ) {
            cout << seq; coloured = false;
          }
      }
      else {
        assert( c->first == *i );
        if ( char const* seq 
               = tparm( RINGING_TERMINFO_VAR( set_a_foreground ), 
                        c->second ) ) {
          cout << seq; coloured = true;
        }
      }
      cout << *i;
    }
    if ( coloured )
      if ( char const* seq = RINGING_TERMINFO_VAR( orig_pair ) )
        cout << seq;
  }

  cout << "\n";
}

int main(int argc, char* argv[]) 
{
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
  if ( !args.colours.empty() )
    setupterm(NULL, 1, NULL);
# endif

  row r( args.startrow );  bool first = true;

  for ( int i=0; i<args.init_rounds*2; ++i)
    print_row(args, r);

  do for ( method::const_iterator i=args.meth.begin(), e=args.meth.end(); 
           i!=e; ++i )  
  {
    if ( !( first && (args.omit_start || args.init_rounds) ) ) 
      print_row(args, r);

    r *= *i;  first = false;
  } while ( r != args.startrow && args.whole_course );

  if (!args.omit_final && args.meth.size())
    print_row(args, r);
}


