// row.cpp - Classes for row and changes
// Copyright (C) 2001 Martin Bright <M.Bright@dpmms.cam.ac.uk>

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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <stdlib.h>
#else
#include <cctype>
#include <cstdlib>
#endif
#if RINGING_OLD_INCLUDES
#include <bvector.h>
#endif
#include <ringing/row.h>

RINGING_USING_STD

RINGING_START_NAMESPACE

// An hcf function. Uses, ahhhh, ze Euclidean algorizm.
int hcf(int a, int b)
{
  int r;

  // Make sure we have a >= b
  if(a < b) { r = a; a = b; b = r; }

  do {
    r = a % b;
    a = b;
    b = r;
  } while(r > 0);

  // a is now the last non-zero remainder we had.
  return a;
}

char bell::symbols[] = "1234567890ETABCDFGHJKLMNPQRSUVWYZ";
const unsigned int bell::MAX_BELLS = 33;

bell::invalid::invalid()
  : invalid_argument("The bell supplied was invalid") {}

// *********************************************************************
// *                    Functions for class change                     *
// *********************************************************************

// Construct from place notation to a change
change::change(int num, const char *pn)
{
  n = num; swaps.erase(swaps.begin(), swaps.end());
  if(*pn == '\0') return;
  bell b, c, d;
  if(*pn == 'X' || *pn == 'x' || *pn == '-')
    c = 0;
  else {
    b.from_char(*pn); if(b > 0 && b & 1) c = 1; else c = 0;
    while(*pn != '\0') {
      b.from_char(*pn);
#if RINGING_USE_EXCEPTIONS
      if(b >= n || b < c-1) throw invalid();
#endif
      if(b >= c) {
	for(d = c; d < b-1; d = d + 2) swaps.push_back(d);
	c = b + 1;
      }
      ++pn;
    }
  }
  for(d = c; d < n-1; d = d + 2) swaps.push_back(d);
}

// Construct from place notation to a change
change::change(int num, const string& pn)
{
  string::const_iterator p = pn.begin();
  n = num; swaps.erase(swaps.begin(), swaps.end());
  if(p == pn.end()) return;
  bell b, c, d;
  if(*p == 'X' || *p == 'x' || *p == '-')
    c = 0;
  else {
    b.from_char(*p); if(b > 0 && b & 1) c = 1; else c = 0;
    while(p != pn.end()) {
      b.from_char(*p);
#if RINGING_USE_EXCEPTIONS
      if(b >= n || b < c-1) throw invalid();
#endif
      if(b >= c) {
	for(d = c; d < b-1; d = d + 2) swaps.push_back(d);
	c = b + 1;
      }
      ++p;
    }
  }
  for(d = c; d < n-1; d = d + 2) swaps.push_back(d);
}

change::invalid::invalid() 
  : invalid_argument("The change supplied was invalid") {}

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

  if(n != 0) {
    bell i = 0;
    vector<bell>::const_iterator s;
    for(s = swaps.begin(); s != swaps.end(); s++) { // Find the next swap
      while(i < *s) { p += i.to_char(); i = i + 1; } // Write all the places
      i = i + 2;
    }
    // Write the remaining places
    while(i < n) { p += i.to_char(); i = i + 1; } 
    if(p.empty()) p = "X";
  }
  return p;
}

char *change::print(char *pn) const
{
  return strcpy( pn, print().c_str() );
}


// Check whether a particular swap is done
bool change::findswap(bell which) const
{
  if(which < 0) return 0;
  if(n == 0) return 0;
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
  if(which + 1 > n)
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
  if(n == 0) return false;
  if(!swaps.empty() && swaps[0] > 1) return true;
  vector<bell>::const_iterator s = swaps.begin();
  bell b = swaps[0];
  for(++s; s != swaps.end(); ++s) {
    if((*s - b) > 2) return true;
    b = *s;
  }
  if((n - *s) > 2) return true;
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
      b = b - 1;
    else if(*s == b)
      b = b + 1;
  return b;
}


// *********************************************************************
// *                    Functions for class row                        *
// *********************************************************************

// Return rounds on n bells
row::row(int num)
  : data(num)
{
  rounds();
}
 
// Construct a row from a string
row::row(const char *s)
  : data(strlen(s))
{
  for(const char *t = s; *t != '\0'; t++)
    data[t - s].from_char(*t);
#if RINGING_USE_EXCEPTIONS
  vector<bool> found(bells());
  int i;
  for(i=0; i<bells(); ++i) found[i] = false;
  for(i=0; i<bells(); ++i) 
    if (data[i] < 0 || data[i] >= (int) data.size() || found[data[i]])
      throw invalid();
    else
      found[data[i]] = true;
#endif
}

row::invalid::invalid()
  : invalid_argument("The row supplied was invalid")
{}

// Assign a string value to a row
row& row::operator=(const char *s)
{
  row(s).swap(*this);
  return *this;
}

// Compare two rows
int row::operator==(const row& r) const
{
  return data == r.data;
}

// Transpose one row by another
row row::operator*(const row& r) const
{
  int m = (bells() < r.bells()) ? r.bells() : bells();
  row product(m);
  int i;
  for(i = 0; i < r.bells(); i++)
    if(r.data[i] < bells())
      product.data[i] = data[r.data[i]];
    else
      product.data[i] = r.data[i];
  for(;i < m; i++)
    product.data[i] = data[i];
  return product;
}

row& row::operator*=(const row& r)
{
  *this = *this * r;
  return *this;
}

// Divide one row by another
row row::operator/(const row& r) const
{
  int m = (bells() < r.bells()) ? r.bells() : bells();
  row quotient(m);
  int i;
  for(i = 0; i < bells() && i < r.bells(); i++)
    quotient.data[r.data[i]] = data[i];
  for(; i < m; i++)
    quotient.data[i] = i;
  return quotient;
}

row& row::operator/=(const row& r)
{
  *this = *this / r;
  return *this;
}

// Find the inverse of a row (same as dividing rounds by it)
row row::inverse(void) const
{
  if(data.empty())
    return row();
  row result(bells());
  for(int i = 0; i < bells(); i++)
    result.data[data[i]] = i;
  return result;
}

// Apply a change to a row
row& operator*=(row& r, const change& c)
{
  while(r.bells() < c.bells())
    r.data.push_back(r.bells());

  char t;

  if(c.n != 0 && !r.data.empty()) {
    vector<bell>::const_iterator s;
    for(s = c.swaps.begin(); s != c.swaps.end() && *s < (r.bells() - 1); s++) {
      t = r.data[*s];
      r.data[*s] = r.data[*s + 1];
      r.data[*s + 1] = t;
    }
  }
  return r;
}

row row::operator*(const change& c) const
{
  row result(*this);
  result *= c;
  return result;
}

// Print it to a string
string row::print() const
{
  string s;

  if(!data.empty())
    for(vector<bell>::const_iterator i = data.begin(); 
	i != data.end(); ++i)
      s += i->to_char();
  return s;
}

char *row::print(char *s) const
{
  return strcpy( s, print().c_str() );
}

// Set it to rounds
row& row::rounds(void)
{
  for(int i = 0; i < bells(); i++)
    data[i] = i;
  return *this;
}

// Return queens on n bells
row row::queens(const int n)
{
  row r(n);

  int half = 0;
  if (n % 2 == 1)
    {
      half = (n + 1) / 2;
    }
  else
    {
      half = n / 2;
    }
  {
    for (int i = 0; i <= half; i++)
      {
	r.data[i] = (i * 2);
      }
  }
  {
    for (int i = 0; i < (n / 2); i++)
      {
	r.data[i + half] = (i * 2) + 1;
      }
  }
  return r;
}

// Return kings on n bells
row row::kings(const int n)
{
  row r(n);

  int half = 0;
  if (n % 2 == 1)
    {
      half = (n + 1) / 2;
    }
  else
    {
      half = n / 2;
    }
  {
    for (int i = 0; i <= half; i++)
      {
	r.data[i] = (half * 2) - (i * 2) - 2;
      }
  }
  {
    for (int i = 0; i < (n / 2); i++)
      {
	r.data[i + half] = (i * 2) + 1;
      }
  }

  return r;
}

// Return titums on n bells
row row::titums(const int n)
{
  row r(n);
  int i;
  int j = 0;
  for (i = 0; i < n; i += 2)
    {
      r.data[i] = j++;
    }
  for (i = 1; i < n; i += 2)
    {
      r.data[i] = j++;
    }
  return r;
}

// Return reverse rounds on n bells
row row::reverse_rounds(const int n)
{
  row r(n);
  for (int i = 0; i < n; i++)
    {
      r.data[i] = n - i - 1;
    }
  return r;
}

// Return 1st plain bob lead head on n bells
// h=1 for Plain Bob, 2 for Grandsire etc.
row row::pblh(int n, int h)
{
  row r(n);
  change c(n);
  int i;

  r.rounds();
  for(i = h; i < n-1; i += 2)
    c.swappair(i);
  r *= c;
  for(i = h+1; i < n-1; i += 2)
    c.swappair(i);
  r *= c;
  return r;
}

// Check whether it is rounds
bool row::isrounds(void) const
{
  if(data.empty()) return 1;
  for(int i = 0; i < bells(); i++)
    if(data[i] != i) return false;
  return true;
}

// Return which plain bob lead head it is
int row::ispblh(void) const
{
  if(data.empty()) return 1;
  int h;
  for(h = 0; h < bells() && data[h] == h; h++);
  if(h == 0) return 0;
  row l = pblh(bells(), h), r = l;
  for(int i = 1; i < (bells() - h); i++) {
    if(*this == r) return i;
    r *= l;
  }
  return 0;
}

// Express it as a product of disjoint cycles
string row::cycles() const
{
  // Note: If you change the format of the output of this function,
  // make sure you verify that row::sign() still works.
  string result;

  if (data.empty()) return result;
  vector<bool> done(bells(), false);

  int i = 0;
  for (;;) {
    // Find the next bell we haven't got yet
    while ( i < bells() && done[i] ) ++i;
    if ( i == bells() ) break;
    do {
      result.append( 1, bell(i).to_char() );  // Write it to the string
      done[i] = true;		// Remember that we've done it
      i = data[i];		// Find the next one in this cycle
    } while (!done[i]);		// until the finish the cycle
    result.append(1, ',');	// Write a separator
  }

  result.erase( result.end() - 1 );// Get rid of the final separator
  return result;
}

// Express it as a product of disjoint cycles
char *row::cycles(char *result) const
{
  return strcpy( result, cycles().c_str() );
}

// Return the order of a row
int row::order(void) const
{
  if(data.empty()) return 1;
  int o = 1;

  // We do this by expressing the row as disjoint cycles, then by taking the
  // lowest common multiple of the lengths of all the cycles.
  // This may seem like overkill. We could just keep multiplying the row by
  // itself until we got back to rounds. That would be OK for smaller numbers
  // but the maximum order on 16 bells is (I think) 4x5x7 = 140. It's quicker
  // to do it this way.

  string cyc( cycles() );              // Get the cycles representation
  
  // For each cycle in the string
  string::const_iterator i, j;
  i = j = cyc.begin(); 
  while(j != cyc.end()) {
    while(j != cyc.end() && *j != ',') ++j;
    o = lcm(o, j - i);
    if(j != cyc.end()) i = ++j;
  }

  return o;
}

// Return the sign of a row
int row::sign(void) const
{
  // Express it in cycles and take the length of the string
  return ( cycles().length() & 1 ) ? 1 : -1;
}


// *********************************************************************
// *                    Functions for class row_block                  *
// *********************************************************************

// Constructor
row_block::row_block(const vector<change> &c) 
  : vector<row>(c.size() + 1), ch(c)
{
  if(ch.size() > 0) {
    (*this)[0] = row::rounds(ch[0].bells());
    recalculate();
  }
}

// Another constructor
row_block::row_block(const vector<change> &c, const row &r) 
  : vector<row>(c.size() + 1), ch(c)
{
  (*this)[0] = r;
  recalculate();
}

// Recalculate rows from changes
row_block& row_block::recalculate(int start)
{
  int i = size() - 1;
  for(;start < i;start++)
    (*this)[start + 1] = (*this)[start] * ch[start];
  return *this;
}

RINGING_END_NAMESPACE
