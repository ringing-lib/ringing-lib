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
#if RINGING_OLD_INCLUDES
#include <memory.h>
#else
#include <memory>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <stdlib.h>
#else
#include <cctype>
#include <cstdlib>
#endif
#include <ringing/row.h>
#include <ringing/stuff.h>

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

// *********************************************************************
// *                    Functions for class change                     *
// *********************************************************************

// Assign place notation to a change
change& change::set(int num, const char *pn)
{
  n = num; swaps.erase(swaps.begin(), swaps.end());
  if(*pn == '\0') return *this;
  bell b, c, d;
  if(*pn == 'X' || *pn == 'x' || *pn == '-')
    c = 0;
  else {
    b.from_char(*pn); if(b > 0 && b & 1) c = 1; else c = 0;
    while(*pn != '\0') {
      b.from_char(*pn);
      if(b >= c) {
	for(d = c; d < b-1; d = d + 2) swaps.push_back(d);
	c = b + 1;
      }
      ++pn;
    }
  }
  for(d = c; d < n-1; d = d + 2) swaps.push_back(d);
  return *this;
}

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
char *change::print(char *pn) const
{
  char *p = pn;

  if(n != 0) {
    bell i = 0;
    vector<bell>::const_iterator s;
    for(s = swaps.begin(); s != swaps.end(); s++) { // Find the next swap
      while(i < *s) { *p++ = i.to_char(); i = i + 1; } // Write all the places
      i = i + 2;
    }
    // Write the remaining places
    while(i < n) { *p++ = i.to_char(); i = i + 1; } 
    if(p == pn) *p++ = 'X';
  }
  *p = '\0';
  return pn;
}

// Check whether a particular swap is done
int change::findswap(bell which) const
{
  if(which < 0) return 0;
  if(n == 0) return 0;
  for(vector<bell>::const_iterator s = swaps.begin(); 
      s != swaps.end() && *s <= which; s++)
    if(*s == which) return 1;
  return 0;
}

// Check whether a particular place is made
int change::findplace(bell which) const
{
  if(n == 0) return 1;
  for(vector<bell>::const_iterator s = swaps.begin(); 
      s != swaps.end() && *s <= which; s++)
    if(*s == which || *s == which-1) return 0;
  return 1;
}

// Swap or unswap a pair of bells
// Return 1 if we swapped them, 0 if we unswapped them
int change::swap(bell which)
{
  if(which + 1 > n)
    return -1;

  vector<bell>::iterator s;

  for(s = swaps.begin(); s != swaps.end() && *s <= which + 1; s++) {
    if(*s == which) { // The swap is already there, so take it out
      swaps.erase(s);
      return 0;
    }
    if(*s == which - 1) { // The swap before it is there. Replace it
                          // with our new swap.
      *s++ = which;
      if(*s == which + 1) // The swap after it is there too. Take it out.
	swaps.erase(s);
      return 1;
    }
    if(*s == which + 1) { // The swap after it is there. Replace it with
                          // our new swap.
      *s = which;
      return 1;
    }
  }
  // OK, we just need to add it.
  swaps.insert(s, which);
  return 1;
}

// Does it contain internal places?
int change::internal(void) const
{
  if(n == 0) return 0;
  if(!swaps.empty() && swaps[0] > 1) return 1;
  vector<bell>::const_iterator s = swaps.begin();
  bell b = swaps[0];
  for(++s; s != swaps.end(); ++s) {
    if((*s - b) > 2) return 1;
    b = *s;
  }
  if((n - *s) > 2) return 1;
  return 0;
}

// Write it to a stream
ostream& operator<<(ostream& o, const change& c)
{
  auto_ptr<char> s1(new char[c.bells() + 1]);
  char *s = s1.get();
  c.print(s);
  o << s;
  return o;
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

// Construct a row from a string
row::row(char *s)
{
  *this = s;
}

// Assign a string value to a row
row& row::operator=(char *s)
{
  data = vector<bell>(strlen(s));
  char *t;
  for(t = s; *t != '\0'; t++)
    data[t - s].from_char(*t);
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
  for(i = 0; i < bells() && i < r.bells(); i++)
    product.data[i] = data[r.data[i]];
  for(;i < m; i++)
    product.data[i] = i;
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
char *row::print(char *s) const
{
  char *t = s;
  if(!data.empty())
    for(vector<bell>::const_iterator i = data.begin(); 
	i != data.end(); ++i)
      *t++ = i->to_char();
  *t = 0;
  return s;
}

// Write it to a stream
ostream& operator<<(ostream &o, const row& r)
{
  if(!r.data.empty())
    for(vector<bell>::const_iterator i = r.data.begin(); 
	i != r.data.end(); ++i)
      o << *i;
  return o;
}

// Set it to rounds
row& row::rounds(void)
{
  for(int i = 0; i < bells(); i++)
    data[i] = i;
  return *this;
}

// Return rounds on n bells
row row::rounds(int n)
{
  row r(n);
  r.rounds();
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
    c.swap(i);
  r *= c;
  for(i = h+1; i < n-1; i += 2)
    c.swap(i);
  r *= c;
  return r;
}

// Check whether it is rounds
int row::isrounds(void) const
{
  if(data.empty()) return 1;
  for(int i = 0; i < bells(); i++)
    if(data[i] != i) return 0;
  return 1;
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
char *row::cycles(char *result) const
{
  if(data.empty()) {
    *result = '\0';
    return result;
  }
  auto_ptr<char> done1(new char[bells()]);
  char *done = done1.get();
  int i;

  for(i = 0;i < bells();i++) done[i] = 0;
  i = 0;
  char *s = result;
  for(;;) {
    while(i < bells() && done[i]) i++; // Find the next bell we haven't got yet
    if(i == bells()) break;
    do {
      *s++ = bell(i).to_char();        // Write it to the string
      done[i] = 1;              // Remember that we've done it
      i = data[i];              // Find the next one in this cycle
    } while(!done[i]);          // until the finish the cycle
    *s++ = ',';                 // Write a separator
  }
  *(s-1) = '\0';                // Get rid of the final separator
  return result;
}

// Return the order of a row
int row::order(void) const
{
  if(data.empty()) return 1;
  auto_ptr<char> cyc1(new char[2*bells()]);
  char *cyc = cyc1.get(), *s;
  int o = 1;

  // We do this by expressing the row as disjoint cycles, then by taking the
  // lowest common multiple of the lengths of all the cycles.
  // This may seem like overkill. We could just keep multiplying the row by
  // itself until we got back to rounds. That would be OK for smaller numbers
  // but the maximum order on 16 bells is (I think) 4x5x7 = 140. It's quicker
  // to do it this way.

  cycles(cyc);                  // Get the cycles representation
  for(s = strtok(cyc,",");s != NULL;s = strtok(NULL,","))   // For each cycle in the string
    o = lcm(o,strlen(s));
  return o;
}

// Return the sign of a row
int row::sign(void) const
{
  if(data.empty()) return 1;
  auto_ptr<char> c1(new char[bells()*2]);
  char *c = c1.get();
  cycles(c);                    // Express it in cycles
  int sgn = (strlen(c) + 1) & 1; // Just take the length of the string
  return sgn ? 1 : -1;
}


// *********************************************************************
// *                    Functions for class row_block                  *
// *********************************************************************

// Constructor
row_block::row_block(const vector<change> &c) 
  : vector<row>(c.size() + 1), ch(c)
{
  if(ch.size() > 0) {
    (*this)[0] = row::rounds(ch[1].bells());
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















