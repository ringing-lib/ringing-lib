// -*- C++ -*- chage.h - Class representing a change
// Copyright (C) 2001, 2009, 2012 Martin Bright <martin@boojum.org.uk>
// and Richard Smith <richard@ex-parrot.com>

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// $Id$


#ifndef RINGING_CHANGE_H
#define RINGING_CHANGE_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <vector.h>
#include <algo.h>
#include <stdexcept.h>
#else
#include <ostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#endif
#include <string>

#include <ringing/bell.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class row;

// change : This stores one change
class RINGING_API change {
public:
  change() : n(0) {}            //
  explicit change(int num) : n(num) {}   // Construct an empty change
  change(int num, const char *pn);
  change(int num, const string& s);
  // Use default copy constructor and assignment

  change& set(int num, const char *pn) // Assign from place notation
    { change(num, pn).swap(*this); return *this; }
  change& set(int num, const string& pn)
    { change(num, pn).swap(*this); return *this; }
  bool operator==(const change& c) const
    { return (n == c.n) && (n == 0 || swaps == c.swaps); }
  bool operator!=(const change& c) const
    { return !(*this == c); }
  change reverse(void) const;            // Return the reverse
  void swap(change& other) {  // Swap this with another change 
    int t = n; n = other.n; other.n = t;
    swaps.swap(other.swaps);
  }

  friend RINGING_API row& operator*=(row& r, const change& c);
  friend RINGING_API bell& operator*=(bell& i, const change& c);

  string print() const;         // Print place notation to a string
  int bells(void) const { return n; } // Return number of bells
  int sign(void) const;         // Return whether it's odd or even
  bool findswap(bell which) const; // Check whether a particular swap is done
  bool findplace(bell which) const; // Check whether a particular place is made
  bool swappair(bell which);            // Swap or unswap a pair
  bool internal(void) const;    // Does it contain internal places?
  int count_places(void) const; // Count the number of places made

  // So that we can put changes into containers
  bool operator<(const change& c) const {
    return (n < c.n) || (n == c.n && swaps < c.swaps);
  }
  bool operator>(const change& c) const {
    return (n > c.n) || (n == c.n && swaps > c.swaps);
  }
  bool operator>=(const change& c) const { return !( *this < c ); }
  bool operator<=(const change& c) const { return !( *this > c ); }

  // Thrown by swappair if the pair to swap is out of range
  struct out_of_range : public RINGING_PREFIX_STD out_of_range {
    out_of_range();
  };

  // Thrown by the constructor if an invalid place-notation is given 
  struct invalid : public invalid_argument {
    invalid();
    invalid(const string& s);
  };

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0) 
  char *print(char *pn) const;  // This overload is deprecated.
#endif

private:
  void init( char const* p, size_t sz );

  int n;                        // Number of bells
  vector<bell> swaps;           // List of pairs to swap
};

inline RINGING_API ostream& operator<<(ostream& o, const change& c) {
  return o << c.print();
}

// Apply a change to a position
RINGING_API bell& operator*=(bell& i, const change& c);
inline RINGING_API bell operator*(bell i, const change& c)
{
  bell j = i; j *= c; return j;
}

bool have_same_places( const change &a, const change &b );

#if RINGING_AS_DLL
RINGING_EXPLICIT_STL_TEMPLATE vector<change>;
#endif

RINGING_END_NAMESPACE

// specialise std::swap
RINGING_DELEGATE_STD_SWAP( change )

#endif
