// -*- C++ -*- row_wildcard.h - Represents a set of rows, e.g. '1???5678'
// Copyright (C) 2001, 2008, 2009, 2010 Mark Banner <mark@standard8.co.uk>
// and Richard Smith <richard@ex-parrot.com>.

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

#ifndef RINGING_ROW_WILDCARD_H
#define RINGING_ROW_WILDCARD_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif
#include <string>
#include <ringing/row.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class RINGING_API row_wildcard 
{
public:
  // The default number of bells (0) means that a number must be 
  // supplied to the count function.  This is for compatibility
  // with the music_details interface that doesn't initially have
  // a number of bells available. 
  row_wildcard( string const& pattern = "", unsigned bells = 0 );

  // If exceptions are not supported, returns false if invalid.
  bool set_bells( unsigned bells );
  bool set( string const& pattern, unsigned bells = 0 );

  unsigned bells() const { return b; }
  string const& get() const { return pat; }

  // Counts the number of rows that match the pattern.
  // The bells argument here overrides the value passed to the constructor.
  unsigned count( unsigned bells = 0 ) const;

#if RINGING_USE_EXCEPTIONS
  struct invalid_pattern : public invalid_argument {
    invalid_pattern( string const& pat, string const& msg );
  };
#endif

private:
  bool check_expression() const;

  unsigned int possible_matches
    ( unsigned int bells, unsigned int pos, const string &s, int &q ) const;

  string pat;
  unsigned b;
};


RINGING_END_NAMESPACE

#endif // RINGING_ROW_WILDCARD_H


