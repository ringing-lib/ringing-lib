// proof.cpp - Proving Stuff
// Copyright (C) 2001-2 Mark Banner <mark@standard8.co.uk>
// and Richard Smith <richard@ex-parrot.com>

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

// The next and prior function in this file are taken from the boost 
// utility library [see http://www.boost.org for details], and are under 
// the following copyright:

// (C) Copyright boost.org 1999. Permission to copy, use, modify, sell
// and distribute this software is granted provided this copyright
// notice appears in all copies. This software is provided "as is" without
// express or implied warranty, and with no claim as to its suitability for
// any purpose.

// $Id$

/********************************************************************
 * Description     :
 *     This is the implementation part of proof.h. As proof is a
 * template class, most of it is defined in the header, however the
 * hash function must be defined here.
 ********************************************************************/

#include <ringing/common.h>

#ifdef RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/proof.h>

RINGING_USING_STD

RINGING_START_NAMESPACE

RINGING_START_ANON_NAMESPACE

//  Helper functions for classes like bidirectional iterators not supporting
//  operator+ and operator-.

template <class T>
inline T next(T x) { return ++x; }

template <class T>
inline T prior(T x) { return --x; }

RINGING_END_ANON_NAMESPACE



// Returns false if the touch is false
bool prover::add_row( const row &r )
{
  // This function is quite complicated to avoid doing more than one
  // O( ln N ) operation on the multimap.  The equal_range function call
  // will be O( ln N ); the insert function ought to be O(1) because a 
  // sensible hint is supplied (although the C++ standard doesn't require
  // the hint to be used).  The number of identical elements is also 
  // calculated from the range, rather than doing a O( ln N ) call to 
  // multimap::count.

  typedef pair< mmap::iterator, mmap::iterator > range;
  range rng = m.equal_range(r);
  size_t n = distance( rng.first, rng.second ) + 1; // effecively m.count(r)
  mmap::iterator i( m.insert( rng.first == m.begin() ? m.begin() 
			                             : prior( rng.first ),
			      mmap::value_type( r, ++lineno ) ) );

  if ( n > 1 )
    ++dups;

  if ( (int) n > max_occurs )
    {
      is_true = false; 
      if ( fi )
	{

	  for ( failinfo::iterator j = fi->begin(), e = fi->end(); j != e; ++j)
	    if ( j->_row == r )
	      {
		j->_lines.push_back( i->second );
		return false;
	      }

	  linedetail l;

	  // The C++ standard doesn't make any guarantee about where i
	  // is in relation to rng.first and rng.second -- it may fall in
	  // the range [rng.first, rng.second), but equally, it might get 
	  // inserted immediately before rng.first.  If we don't detect i 
	  // in this range, we explicitly add it by hand afterwards.
	  l._row = r;
	  bool added_i = false;
	  {
	    for ( mmap::iterator j = rng.first; j != rng.second; ++j )
	      {
		if ( j == i ) added_i = true;
		l._lines.push_back( j->second );
	      }
	  }
	  if (!added_i) 
	    l._lines.push_back( i->second );

	  fi->push_back( l );
	  return false;
	}
    }

  return is_true;
}


RINGING_END_NAMESPACE
