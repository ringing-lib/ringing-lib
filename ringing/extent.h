// -*- C++ -*- extent.h - Classes for iterating through an extent
// Copyright (C) 2001, 2002, 2005, 2007 Richard Smith <richard@ex-parrot.com>

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
#include <ringing/change.h>

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
#include <ringing/mathutils.h>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

// Generate the extent in lexicographical order
class RINGING_API extent_iterator
  : public RINGING_STD_CONST_ITERATOR( forward_iterator_tag, row )
{
public:
  // Standard iterator typedefs
  typedef bidirectional_iterator_tag iterator_category;
  typedef row value_type;
  typedef ptrdiff_t difference_type;
  typedef const row *pointer;
  typedef const row &reference;

  // A generic end iterator (cannot be decremented)
  extent_iterator() : end_(true) {}
  
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
    { return end_ == i.end_ && (end_ || r == i.r); }
  bool operator!=( const extent_iterator &i ) const
    { return !( *this == i ); }

  // Less-than Comparable implementation
  bool operator< ( const extent_iterator &i ) const;
  bool operator> ( const extent_iterator &i ) const
    { return i < *this; }
  bool operator<=( const extent_iterator &i ) const 
    { return !( i < *this ); }
  bool operator>=( const extent_iterator &i ) const
    { return !( *this < i ); }

  // Forward Iterator implementation
  extent_iterator &operator++();
  extent_iterator operator++(int)
    { extent_iterator tmp(*this); ++*this; return tmp; }

  // Bidirectional Iterator implementation
  extent_iterator &operator--();
  extent_iterator operator--(int)
    { extent_iterator tmp(*this); --*this; return tmp; }


  bool is_end() const { return end_; }
  static extent_iterator end( unsigned nw, unsigned nh, unsigned nt );
  static extent_iterator end( unsigned nw, unsigned nh = 0)
    { return end(nw, nh, nw+nh);  }

private:
  struct bellsym_cmp;

  unsigned int nw, nh, nt;
  bool end_;
  row r;
};


// Find the position of the row with in an extent ordered lexicographically.
// Returns in range [0, nw!)
RINGING_API size_t 
position_in_extent( row const& r, unsigned nw, unsigned nh, unsigned nt );

inline RINGING_API size_t
position_in_extent( row const& r, unsigned nw, unsigned nh = 0 ) {
  return position_in_extent( r, nw, nh, nw+nh );
}


// Finds the nth row of an extent ordered lexicographically.
RINGING_API row
nth_row_of_extent( size_t n, unsigned nw, unsigned nh, unsigned nt );

inline RINGING_API row
nth_row_of_extent( size_t n, unsigned nw, unsigned nh = 0 ) {
  return nth_row_of_extent( n, nw, nh, nw+nh );
}


// A pseudo-container class representing the extent
class RINGING_API extent {
public:
  typedef row       value_type;
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;

  explicit extent( unsigned nw, unsigned nh = 0 ) 
    : nw(nw), nh(nh), nt(nw+nh) {}
  extent( unsigned nw, unsigned nh, unsigned nt )
    : nw(nw), nh(nh), nt(nt) {}

  typedef extent_iterator const_iterator;
  extent_iterator begin() const;
  extent_iterator end() const;
  size_t size() const;

  // A less-than comparator for lexicographical ordering
  typedef less<row> compare;

  // Get nth row and corresponding reverse map
  row operator[]( size_t n ) const;
  size_t operator[]( row r ) const;
 
private:
  unsigned nw, nh, nt;
};


// Generate the in-course half-extent in lexicographical order
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
  extent_iterator ei;
};

// Find the position of the row in an in-course half-extent 
// ordered lexicographically.  Returns in range [0, nw!/2)
inline RINGING_API size_t
position_in_incourse_extent( row const& r,
                             unsigned nw, unsigned nh, unsigned nt ) {
  // Division works because positions 2k,2k+1 in the whole extent
  // must have opposite parities, even though it's not obvious which is which.
  return position_in_extent(r, nw, nh, nt) / 2;
}

inline RINGING_API size_t
position_in_incourse_extent( row const& r, unsigned nw, unsigned nh = 0 ) {
  return position_in_extent( r, nw, nh, nw+nh ) / 2;
}

// Finds the nth row of an in-course half-extent ordered lexicographically.
RINGING_API row
nth_row_of_incourse_extent( size_t n, unsigned nw, unsigned nh, unsigned nt );

inline RINGING_API row
nth_row_of_incourse_extent( size_t n, unsigned nw, unsigned nh = 0 ) {
  return nth_row_of_incourse_extent( n, nw, nh, nw+nh );
}


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

  // A generic end iterator
  changes_iterator() : end_(true) {}

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
    { return end_ == i.end_ && (end_ || c == i.c); }
  bool operator!=( const changes_iterator &i ) const 
    { return !operator==(i); }

  // Forward Iterator implementation
  changes_iterator &operator++() { next(); return *this; }
  changes_iterator operator++(int)
    { changes_iterator tmp(*this); ++*this; return tmp; }


private:
  void next();

  unsigned nw, nh;
  bool end_;
  change c;
  vector<unsigned int> stk;
};



RINGING_API row random_row( unsigned nw, unsigned nh, unsigned nt );

RINGING_API inline row random_row( unsigned nw, unsigned nh = 0 ) { 
  return random_row( nw, nh, nw+nh ); 
}


RINGING_END_NAMESPACE

#endif
