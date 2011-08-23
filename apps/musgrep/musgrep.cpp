// -*- C++ -*- musgrep.cpp - utility to grep for music in an extent
// Copyright (C) 2009, 2010, 2011 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/row.h>
#include <ringing/music.h>
#include <ringing/streamutils.h>
#include "args.h"
#if RINGING_OLD_IOSTREAMS
#include <iostream.h>
#include <istream.h>
#else
#include <iostream>
#include <istream>
#endif
#include <string>
#include <vector>

// Find isatty()
#ifdef _MSC_VER
#include <io.h>
#elif defined(_POSIX_C_SOURCE)
#include <unistd.h>
#else
#define isatty(n) false
#endif

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

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  init_val<int,0> bells;

  init_val<bool,false> in_course;

  init_val<bool,false> count;
  init_val<bool,false> score;

  init_val<bool,false> positive;
  init_val<bool,false> negative;

  init_val<bool,false> hilight;

  vector<string> musstrs;
  music mus;

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
           "Print the number of matching rows", count ) );

  p.add( new boolean_opt
         ( 's', "score",
           "Print the score for matching rows", score ) );

  p.add( new boolean_opt
         ( 'p', "positive",
           "Print the number of rows with a positive score", positive ) );

  p.add( new boolean_opt
         ( 'n', "negative",
           "Print the number of rows with a negative score", negative ) );

  p.add( new boolean_opt
         ( 'H', "highlight",
           "Highlight matching rows", hilight ) );

  p.add( new boolean_opt
         ( 'i', "in-course",
           "Match only in-course rows", in_course ) );

  p.set_default( new strings_opt( '\0', "", "", "", musstrs ) ); 
}

bool arguments::validate( arg_parser& ap )
{               
  if ( bells <= 0 )
    {
      ap.error( "Then number of bells must be positive" );
      return false;
    }

  if ( bells >= int(bell::MAX_BELLS) )
    {
      ap.error( make_string() << "The number of bells must be less than "
                << bell::MAX_BELLS );
      return false;
    }

  mus = music(bells);
  for ( vector<string>::const_iterator i = musstrs.begin(), e = musstrs.end();
          i != e; ++i ) 
  {
    try 
    {
      add_scored_music_string( mus, *i );
    }
    catch ( exception const& e ) {
      ap.error( make_string() << "Error parsing music pattern: " << e.what() );
      return false;
    }
  }

  return true;
}

void output_counter( ostream& os, bool& need_sep, int counter )
{
  if (need_sep) os << "\t";
  os << counter;
  need_sep = true;
}

int main( int argc, char *argv[] )
{
  bell::set_symbols_from_env();

  arguments args;

  {
    arg_parser ap( argv[0], "musgrep -- grep rows for music", "OPTIONS" );
    args.bind( ap );

    if ( !ap.parse(argc, argv) )
      {
        ap.usage();
        return 1;
      }
    
    if ( !args.validate(ap) )
      return 1;
  }

# if RINGING_USE_TERMCAP
  if ( args.hilight )
    setupterm(NULL, 1, NULL);
# endif

  char const* seq1 = RINGING_TERMINFO_VAR( enter_standout_mode );
  char const* seq2 = RINGING_TERMINFO_VAR( exit_standout_mode );

  // Let's use an asterisk to allow 'highlighting' on platforms that 
  // don't support termcap on when stdout is not a tty.
  if (!seq2 || !isatty(1)) seq1 = NULL, seq2 = " *";

  const bool output_rows 
    = !args.count && !args.score && !args.negative && !args.positive;

  // NB: Don't use args.mus.get_count() -- that will double count 5-runs
  // when 4-runs are selected, for example.
  int count = 0, countp = 0, countn = 0;
  bool back = true;
  while ( cin ) {
    row r; 
    cin >> r;
    if ( r.bells() != args.bells ) continue;
    back = !back;

    if ( r.sign() < 0 && args.in_course ) continue;

    int old_score = args.mus.get_score();
    if ( args.mus.process_row(r, back) ) 
    {
      ++count;

      int delta = args.mus.get_score() - old_score;
      if (delta > 0) ++countp; else if (delta < 0) ++countn;

      if (output_rows)
      {
        if (args.hilight && seq1) cout << seq1;
        cout << r;
        if (args.hilight && seq2) cout << seq2;
        cout << "\n";
      }
    }
    else if (output_rows && args.hilight)
      cout << r << "\n";
  }

  // Print counters
  bool need_sep = false;
  if (args.positive) output_counter( cout, need_sep, countp ); 
  if (args.negative) output_counter( cout, need_sep, countn );
  if (args.count)    output_counter( cout, need_sep, count  );
  if (args.score)    output_counter( cout, need_sep, args.mus.get_score() );
  if (need_sep)      cout << endl;
}

