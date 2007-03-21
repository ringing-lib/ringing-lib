// -*- C++ -*- extent.h - Classes for iterating through an extent
// Copyright (C) 2001, 2002, 2005 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <string>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <utility.h>
#else
#include <vector>
#include <utility>
#endif
#include <ringing/row.h>

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
#include <ringing/mathutils.h>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

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
  explicit extent_iterator( unsigned int nw, unsigned int nh = 0u );

  // Trivial Iterator implementation
  const row *operator->() const { return &r; }
  const row &operator*() const { return r; }
  
  // Equality Comparable implementation
  bool operator==( const extent_iterator &i ) const
    { return end == i.end && (end || r == i.r); }
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

class RINGING_API incourse_extent_iterator
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
  incourse_extent_iterator() {}
 
  // The beginning iterator
  //   nw == The number of working bells 
  //   nh == The number of fixed (hunt) bells  [ default = 0 ]
  //   nt == The total number of bells         [ default = nh + nw ]
  // E.g. The set of tenors together lead heads, "1xxxxx78", has 
  // nw = 5, nh = 1, nt = 8.
  incourse_extent_iterator( unsigned int nw, unsigned int nh, unsigned int nt )
    : ei( nw, nh, nt ) {}
  explicit incourse_extent_iterator( unsigned int nw, unsigned int nh = 0u )
    : ei( nw, nh ) {}

  // Trivial Iterator implementation
  const row *operator->() const { return &*ei; }
  const row &operator*() const { return *ei; }
  
  // Equality Comparable implementation
  bool operator==( const incourse_extent_iterator &i ) const
    { return ei == i.ei; }
  bool operator!=( const incourse_extent_iterator &i ) const
    { return ei != i.ei; }

  // Forward Iterator implementation
  incourse_extent_iterator &operator++();
  incourse_extent_iterator operator++(int)
    { incourse_extent_iterator tmp(*this); ++*this; return tmp; }

private:
  extent_iterator ei, ee;
};

class RINGING_API changes_iterator
  : public RINGING_STD_CONST_ITERATOR( forward_iterator_tag, change )
{
public:
  // Standard iterator typedefs
  typedef forward_iterator_tag iterator_category;
  typedef change value_type;
  typedef ptrdiff_t difference_type;
  typedef const change *pointer;
  typedef const change &reference;

  // The end iterator
  changes_iterator() : end(true) {}

  // The beginning iterator
  //   nw == The number of working bells 
  //   nh == The number of initial fixed (hunt) bell  [ default = 0 ]
  //   nt == The total number of bells                [ default = nh + nw ]
  // E.g. The set of valid changes on eight bells leaving the 1, 7 and 8 still
  // has nw = 5, nh = 1, nt = 8.
  changes_iterator( unsigned int nw, unsigned int nh, unsigned int nt );
  explicit changes_iterator( unsigned int nw, unsigned int nh = 0u );

  // Trivial Iterator implementation
  const change *operator->() const { return &c; }
  const change &operator*() const { return c; }

  // Equality Comparable implementation
  bool operator==( const changes_iterator &i ) const 
    { return end == i.end && (end || c == i.c); }
  bool operator!=( const changes_iterator &i ) const 
    { return !operator==(i); }

  // Forward Iterator implementation
  changes_iterator &operator++() { next(); return *this; }
  changes_iterator operator++(int)
    { changes_iterator tmp(*this); ++*this; return tmp; }


private:
  void next();

  unsigned nw, nh;
  bool end;
  change c;
  vector<unsigned int> stk;
};

RINGING_API row random_row( unsigned nw, unsigned nh, unsigned nt );

RINGING_API inline row random_row( unsigned nw, unsigned nh = 0 ) { 
  return random_row( nw, nh, nw+nh ); 
}


RINGING_END_NAMESPACE

#endif
