// change.cpp - Class representing a change
// Copyright (C) 2001 Martin Bright <martin@boojum.org.uk>

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_C_INCLUDES
#include <string.h>
#else
#include <cstring>
#endif

#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <stdexcept.h>
#else
#include <vector>
#include <stdexcept>
#endif
#include <string>

#include <ringing/bell.h>
#include <ringing/change.h>

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0) && defined(_MSC_VER)
// Microsoft have deprecated strcpy in favour of a non-standard
// extension, strcpy_s.  4996 is the warning about it being deprecated.
#pragma warning (disable: 4996)
#endif

RINGING_USING_STD

RINGING_START_NAMESPACE

void change::init( const string &pn )
{
  string::const_iterator p = pn.begin();
  swaps.erase(swaps.begin(), swaps.end());
  if(pn.empty()) {
#if RINGING_USE_EXCEPTIONS
    if (n != 0) throw invalid();
#endif
    return;
  }
  bell c;
  if( pn.size() == 1 && (*p == 'X' || *p == 'x' || *p == '-') )
    c = 0;
  else {
    bell b;
    b.from_char(*p); if(b > 0 && b & 1) c = 1; else c = 0;
    while(p != pn.end()) {
      b.from_char(*p);
#if RINGING_USE_EXCEPTIONS
      if(b >= n || b <= c-1) throw invalid(pn);
#endif
      if(b >= c) {
        bell d;
        for(d = c; d < b-1; d += 2) swaps.push_back(d);
#if RINGING_USE_EXCEPTIONS
        if ( d == b-1 ) throw invalid(pn);
#endif
        c = b + 1;
      }
      ++p;
    }
  }
  for(bell d = c; d < n-1; d += 2) swaps.push_back(d);
}

// Construct from place notation to a change
change::change(int num, const char *pn)
  : n( num )
{
  init( pn );
}

// Construct from place notation to a change
change::change(int num, const string& pn)
  : n( num )
{
  init( pn );
}

change::invalid::invalid()
  : invalid_argument("The change supplied was invalid") {}

change::invalid::invalid(const string& s)
  : invalid_argument("The change '" + s + "' was invalid") {}

change::out_of_range::out_of_range()
  : RINGING_PREFIX_STD out_of_range("The place supplied was out of range") {}

// Return the reverse of a change
change change::reverse(void) const
{
  change c(n);
  vector<bell>::const_reverse_iterator s1 = swaps.rbegin();
  while(s1 != swaps.rend())
    c.swaps.push_back(n - 2 - *s1++);
  return c;
}

// Print place notation to a string
string change::print() const
{
  string p;
  p.reserve( bells() );

  if(n != 0) {
    bell i = 0;
    vector<bell>::const_iterator s;
    for(s = swaps.begin(); s != swaps.end(); s++) { // Find the next swap
      while(i < *s) { p += i.to_char(); ++i; } // Write all the places
      i += 2;
    }
    // Write the remaining places
    while(i < n) { p += i.to_char(); ++i; }
    if(p.empty()) p = "X";
  }
  return p;
}

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0) 
char *change::print(char *pn) const
{
  return strcpy( pn, print().c_str() );
}
#endif



// Check whether a particular swap is done
bool change::findswap(bell which) const
{
  if(n == 0) return false;
  for(vector<bell>::const_iterator s = swaps.begin();
      s != swaps.end() && *s <= which; s++)
    if(*s == which) return true;
  return false;
}

// Check whether a particular place is made
bool change::findplace(bell which) const
{
  if(n == 0) return true;
  for(vector<bell>::const_iterator s = swaps.begin();
      s != swaps.end() && *s <= which; s++)
    if(*s == which || *s == which-1) return false;
  return true;
}

// Swap or unswap a pair of bells
// Return 1 if we swapped them, 0 if we unswapped them
bool change::swappair(bell which)
{
  if(which + 2 > n)
#if RINGING_USE_EXCEPTIONS
    throw out_of_range();
#else
    return false;
#endif

  vector<bell>::iterator s;

  for(s = swaps.begin(); s != swaps.end() && *s <= which + 1; s++) {
    if(*s == which) { // The swap is already there, so take it out
      swaps.erase(s);
      return false;
    }
    if(*s == which - 1) { // The swap before it is there. Replace it
                          // with our new swap.
      *s++ = which;
      if(*s == which + 1) // The swap after it is there too. Take it out.
        swaps.erase(s);
      return true;
    }
    if(*s == which + 1) { // The swap after it is there. Replace it with
                          // our new swap.
      *s = which;
      return true;
    }
  }
  // OK, we just need to add it.
  swaps.insert(s, which);
  return true;
}

// Does it contain internal places?
bool change::internal(void) const
{
  if(n < 3) return false;
  if(swaps.empty() || swaps[0] > 1) return true;
  vector<bell>::const_iterator s = swaps.begin();
  bell b = swaps[0];
  for(++s; s != swaps.end(); ++s) {
    if((*s - b) > 2) return true;
    b = *s;
  }
  if((n - swaps.back()) > 3) return true;
  return false;
}

// Count the places made
// Useful for finding out how long the place notation is
int change::count_places(void) const
{
  if(n == 0) return 0;
  vector<bell>::const_iterator s;
  int count = 0;
  bell b = 0;
  for(s = swaps.begin(); s != swaps.end() && *s < n; ++s) {
    count += (*s - b);
    b = *s + 2;
  }
  count += (n - b);
  return count;
}

// Return whether it's odd or even
int change::sign(void) const
{
  if(n == 0) return 1;
  return (swaps.size() & 1) ? -1 : 1;
}

// Apply a change to a position
bell& operator*=(bell& b, const change& c)
{
  vector<bell>::const_iterator s;
  for(s = c.swaps.begin(); s != c.swaps.end() && *s <= b; s++)
    if(*s == b - 1)
      --b;
    else if(*s == b)
      ++b;
  return b;
}

RINGING_END_NAMESPACE
