// -*- C++ -*- bell_fmt.cpp - class to syntax highlight a bell
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

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
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
#endif


#include <ringing/streamutils.h>

#include "bell_fmt.h"

void bell_fmt::set_colour(string const& str, int colour)
{
  bool bold = 0;
  for (string::const_iterator i=str.begin(), e=str.end(); i!=e; ++i) {
    if (*i == '*') bold = !bold;
    else {
      struct fmt fmt = { colour, bold };
      fmts[ bell::read_char(*i) ] = fmt;
    }
  }
}

void bell_fmt::set_colours(string const& r, string const& g, string const& b)
{
  set_colour(r, COLOR_RED);
  set_colour(g, COLOR_GREEN);
  set_colour(b, COLOR_BLUE);
}


ostream& bell_fmt::do_fmt::operator<<(bell b) const
{
  map<bell, fmt>::const_iterator c( bf->fmts.find(b) );

  bool coloured = false, bold = false;

  if ( c != bf->fmts.end() ) {
    if ( char const* seq 
           = tparm( RINGING_TERMINFO_VAR( set_a_foreground ), 
                    c->second.colour ) ) {
      *os << seq; coloured = true;
    }

    if ( c->second.bold )
      if ( char const* seq = RINGING_TERMINFO_VAR( enter_bold_mode ) ) {
        *os << seq; bold = true;
      }
  }

  *os << b;

  if ( c != bf->fmts.end() ) {
    if ( bold ) 
      if ( char const* seq = RINGING_TERMINFO_VAR( exit_attribute_mode ) ) 
        *os << seq;
    if ( coloured )
      if ( char const* seq = RINGING_TERMINFO_VAR( orig_pair ) )
        *os << seq;
  }

  return *os;
}

ostream& bell_fmt::do_fmt::operator<<(row const& r) const
{
  for (row::const_iterator i=r.begin(), e=r.end(); i != e; ++i)
    *this << *i;
  return *os;
}

