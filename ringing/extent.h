// -*- C++ -*- extent.h - Classes for iterating through an extent
// Copyright (C) 2001-2 Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_EXTENT_H
#define RINGING_EXTENT_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include <string>
#include <ringing/row.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

inline unsigned factorial(unsigned n)
{
  unsigned f = n;
  while (--n) f *= n;
  return f;
}

class RINGING_API extent_iterator
  : public RINGING_STD_CONST_ITERATOR( forward_iterator_tag, row )
{
public:
  // Standard iterator typedefs
  typedef forward_iterator_tag iterator_category;
  typedef row value_type;
  typedef ptrdiff_t difference_type;
  typedef const row *pointer;
  typedef const row &reference;

  // The end iterator
  extent_iterator() : end(true) {}
  
  // The beginning iterator
  //   nw == The number of working bells 
  //   nh == The number of fixed (hunt) bells  [ default = 0 ]
  //   nt == The total number of bells         [ default = nh + nw ]
  // E.g. The set of tenors together lead heads, "1xxxxx78", has 
  // nw = 5, nh = 1, nt = 8.
  extent_iterator( unsigned int nw, unsigned int nh, unsigned int nt );
  extent_iterator( unsigned int nw, unsigned int nh = 0 );

  // Trivial Iterator implementation
  const row *operator->() const { return &r; }
  const row &operator*() const { return r; }
  
  // Equality Comparable implementation
  bool operator==( const extent_iterator &i ) const
    { return end == i.end && end || r == i.r; }
  bool operator!=( const extent_iterator &i ) const
    { return !( *this == i ); }

  // Forward Iterator implementation
  extent_iterator &operator++();
  extent_iterator operator++(int)
    { extent_iterator tmp(*this); ++*this; return tmp; }

private:
  struct bellsym_cmp;

  unsigned int nw, nh, nt;
  bool end;
  row r;
  string s;
};

RINGING_END_NAMESPACE

#endif
