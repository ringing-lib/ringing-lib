// -*- C++ -*- falseness.h - Class for falseness table
// Copyright (C) 2001, 2002 Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_FALSENESS_H
#define RINGING_FALSENESS_H

#if __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include <ringing/row.h>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class method;

// The set of lead-heads that are false against the lead starting with rounds.
class falseness_table
{
public:
  // Contains just the trivial falseness (a row is false against itself)
  falseness_table();

  // What gets included in the falseness table?
  enum
  {
    no_fixed_treble = 0x01,
    in_course_only  = 0x02
  };

  // falseness table for the method.  
  falseness_table( const method &m, int flags = 0 );

  // Assignment and swapping
  void swap( falseness_table &other ) { t.swap( other.t ); }

  // Iterators
  typedef row value_type;
  typedef vector<row>::const_iterator const_iterator;
  const_iterator begin() const { return t.begin(); }
  const_iterator end() const   { return t.end(); }

  // Number of elements
  size_t size() const { return t.size(); }

private:
  vector<row> t;
};

RINGING_END_NAMESPACE

// specialise std::swap
RINGING_DELEGATE_STD_SWAP( falseness_table )

#endif
