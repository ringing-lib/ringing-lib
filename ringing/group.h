// -*- C++ -*- group.h - Class representing a group
// Copyright (C) 2003 Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_GROUP_H
#define RINGING_GROUP_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/row.h>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

// --------------------------------------------------------------
//
// Geneates a group from a set of generators 
//
class group
{
public:
  group() : b(0), v(1u) {} // The group containing just the identity
  explicit group( const row& generator );
  explicit group( const row& generator1, const row& generator2 );
  explicit group( const vector<row> &generators );

  size_t bells() const { return b; }

  // Container interface
  typedef vector<row>::const_iterator const_iterator;
  const_iterator begin() const { return v.begin(); }
  const_iterator end()   const { return v.end();   }
  size_t         size()  const { return v.size();  }

  void swap( group& g ) { v.swap(g.v); }

  // Named constructors
  static group symmetric_group(int nw, int nh = 0, int nt = 0);
  static group alternating_group(int nw, int nh = 0, int nt = 0);

  // Congugate by r to get { r^-1 g r : g \in *this }
  group conjugate( const row& r ) const; 

  friend bool operator==( const group& a, const group& b );
  friend bool operator< ( const group& a, const group& b );
  friend bool operator> ( const group& a, const group& b );

  friend bool operator!=( const group& a, const group& b ) { return !(a==b); }
  friend bool operator<=( const group& a, const group& b ) { return !(a>b); }
  friend bool operator>=( const group& a, const group& b ) { return !(a<b); }

private:
  size_t b;
  vector<row> v;
};

RINGING_END_NAMESPACE

RINGING_DELEGATE_STD_SWAP( group )

#endif // RINGING_GROUP_H
