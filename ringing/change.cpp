// change.cpp - Class representing a change
// Copyright (C) 2001, 2009, 2012, 2020, 2022
// Martin Bright <martin@boojum.org.uk>
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

RINGING_START_ANON_NAMESPACE

static inline 
void append_swaps_between(vector<bell>& swaps, bell& from, bell to) {
  for (bell s = from; s < to-1; s += 2) swaps.push_back(s);
  from = to + 1;
}

RINGING_END_ANON_NAMESPACE

// Construct from place notation to a change
change::change(int n, const string& pn)
  : n(n)
{
  if (pn.empty()) {
#if RINGING_USE_EXCEPTIONS
    if (n != 0) throw invalid();
#endif
    return;
  }

  bell b = 0;
  if ( pn != "X" && pn != "x" && pn != "-" ) {
    bool first = true;
    for (char const* q = pn.c_str(); *q; ) {
      bell p = bell::read_extended(q, &q);
#if RINGING_USE_EXCEPTIONS
      if (p >= n || p < b || !first && (p-b) % 2) throw invalid(pn);
#endif
      if (first && (p-b) % 2) b = 1;  // Implicit place at lead
      append_swaps_between(swaps, b, p);
      first = false;
    }
  }
  append_swaps_between(swaps, b, n);
}

change::change(int n, vector<bell> const& places)
  : n(n)
{
  bell b = 0;
  for ( bell p : places ) {
#if RINGING_USE_EXCEPTIONS
    if (p >= n || p < b || (p-b) % 2) throw invalid();
#endif
    append_swaps_between(swaps, b, p);
  }
#if RINGING_USE_EXCEPTIONS
  if ((n-b) % 2) throw invalid();
#endif
  append_swaps_between(swaps, b, n);
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
string change::print(int flags) const
{
  string p;
  p.reserve(n);

  if (n != 0) {
    bell b = 0;
    for ( bell s : swaps ) {
      while (b < s) { p += b.to_char(); ++b; }
      b += 2;
    }
    while (b < n) { p += b.to_char(); ++b; }

    if (p.empty()) {
      if (flags & C_LCROSS) p = "x";
      else if (flags & C_DASH) p = "-";
      else p = "X";  // For backwards compatibility we make this the default
    }
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
int change::count_places() const {
  int count = 0;
  bell b = 0;
  for ( bell s : swaps ) {
    count += s-b;
    b = s + 2;
  }
  count += n-b;
  return count;
}

vector<bell> change::places() const {
  vector<bell> pl; pl.reserve(n);
  bell b = 0;
  for ( bell s : swaps ) {
    while (b < s) pl.push_back(b++);
    b = s + 2;
  }
  while (b < n) pl.push_back(b++);
  return pl;
} 

// Return whether it's odd or even
int change::sign(void) const {
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

bool have_same_places( const change &a, const change &b )
{
  if ( a.bells() != b.bells() ) 
    throw logic_error("Mismatched numbers of bells");

  for ( int i=0; i<a.bells(); ++i )
    if ( a.findplace(i) && b.findplace(i) )
      return true;

  return false;
}

RINGING_END_NAMESPACE
