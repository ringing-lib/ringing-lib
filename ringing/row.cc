// row.cc - Classes for row and changes
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include RINGING_C_HEADER(ctype)
#include RINGING_LOCAL_HEADER(row)
#include RINGING_LOCAL_HEADER(stuff)
#include RINGING_C_HEADER(stdlib)
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


// *********************************************************************
// *                    Functions for class change                     *
// *********************************************************************

// Construct a change from place notation
change::change(int num, char *pn)
{
  swaps = new char[(n = num)/2+1];
  *this = pn;
}

// Copy constructor
change::change(const change& c)
{
  if(c.swaps) {
    swaps = new char[(n=c.n)/2+1];
    char *s1 = swaps, *s2 = c.swaps;
    while(*s2 != 100)
      *s1++ = *s2++;
    *s1 = 100;
  } else {
    swaps = NULL;
  }
}

// Assign place notation to a change
change& change::operator=(char *pn)
{
  char *buff = new char[n];
  int i = 0, b;
  while(i < n && (buff[i] = toupper(pn[i])) != '\0') i++;
  char *s = swaps;
  // Allowed to miss out the first bell
  b = row::c_to_b(buff[0]);
  if(b > 0 && b & 1) i = 1; else i = 0;
  for(;i < n - 1;i++)
    if(strchr(buff,row::b_to_c(i)) == NULL)
      *s++ = i++;
  *s = 100;
  return *this;
}

// Assign a change to a change
change& change::operator=(const change &c)
{
  if(swaps == NULL || n != c.n) {
    delete[] swaps;
    swaps = new char[(n = c.n)/2 + 1];
  }
  for(int i = 0;i < (n / 2 + 1);i++)
    swaps[i] = c.swaps[i];
  return *this;
}

// Compare two changes
int change::operator==(const change& c) const
{
  if(n != c.n) return 0;
  char *s1 = swaps, *s2 = c.swaps;
  while(*s1 != 100)
    if(*s1++ != *s2++) return 0;
  return (*s2 == 100);
}

// Return the reverse of a change
change change::reverse(void) const
{
  change c(n);
  char *s1 = swaps, *s2 = c.swaps;
  while(*s1 != 100) s1++;       // Find the end of the list
  s1--;                         // Skip back over the terminator
  while(s1 >= swaps)            // Copy all the swaps
    *s2++ = n - 2 - *s1--;      // and reverse them
  *s2 = 100;
  return c;
}

// Print place notation to a string
char *change::print(char *pn) const
{
  char *p = pn;

  int i = 0;
  for(char *s = swaps;*s != 100;s++) { // Find the next swap
    while(i < *s) *p++ = row::b_to_c(i++); // Write all the places
    i+= 2;
  }
  while(i < n) *p++ = row::b_to_c(i++); // Write the remaining places
  if(p == pn) *p++ = 'X';
  *p = '\0';
  return pn;
}

// Check whether a particular swap is done
int change::findswap(int which) const
{
  if(which < 0) return 0;
  for(char *s = swaps;*s <= which;s++)
    if(*s == which) return 1;
  return 0;
}

// Check whether a particular place is made
int change::findplace(int which) const
{
  for(char *s = swaps;*s <= which;s++)
    if(*s == which || *s == which-1) return 0;
  return 1;
}

// Swap or unswap a pair of bells
// Return 1 if we swapped them, 0 if we unswapped them
int change::swap(int which)
{
  char *s;

  for(s = swaps;*s <= which + 1;s++) {
    if(*s == which) { // The swap is already there, so take it out
      while(*s != 100) { // Just copy all the others back one place
        *s = *(s+1);
        s++;
      }
      return 0;
    }
    if(*s == which - 1) { // The swap before it is there. Replace it
                          // with our new swap.
      *s++ = which;
      if(*s == which + 1) // The swap after it is there too. Take it out.
        while(*s != 100) {
          *s = *(s+1);
          s++;
        }
      return 1;
    }
    if(*s == which + 1) { // The swap after it is there. Replace it with
                          // our new swap.
      *s = which;
      return 1;
    }
  }
  // OK, we just need to add it.
  // First shift all the ones after it, along one.
  // Find the end
  char *t;
  for(t = s;*t != 100;t++);
  // Work backwards and shift the others along
  while(t >= s) { *(t+1) = *t; t--; }
  // Add our swap
  *s = which;
  return 1;
}

// Does it contain internal places?
int change::internal(void) const
{
  if(swaps[0] > 1) return 1;
  char *s;
  for(s = swaps+1; *s != 100; s++)
    if((*s - *(s-1)) > 2) return 1;
  if((n - *s) > 2) return 1;
  return 0;
}

// Write it to a stream
ostream& operator<<(ostream& o, const change& c)
{
  char *s = new char[c.bells() + 1];
  c.print(s);
  o << s;
  delete[] s;
  return o;
}

// Return whether it's odd or even
int change::sign(void) const
{
  int i = 1;
  char *s = swaps;
  while(*s++ != 100) i = -i;
  return i;
}

// Apply a change to a position
int& operator*=(int& i, const change& c)
{
  if(c.findswap(i-1))
    i--;
  else if(c.findswap(i))
    i++;
  return i;
}

// *********************************************************************
// *                    Functions for class changes                    *
// *********************************************************************

// Return the cumulative effect of all the changes
row changes::asrow(void) const
{
  row r(bells());
  r.rounds();
  const_iterator i = begin();
  while(i < end())
    r *= *i++;
  return r;
}

// *********************************************************************
// *                    Functions for class row                        *
// *********************************************************************

char row::symbols[] = "1234567890ETABCDFGHJKLMNPQRSUVWYZ";

// Construct a row from a string
row::row(char *s)
{
  data = new char[n = strlen(s)];
  *this = s;
}

// Copy constructor
row::row(const row& r)
{
  if(r.data) {
    data = new char[n = r.n];
    memcpy(data, r.data, n);
  } else
    data = NULL;
}

// Assign a string value to a row
row& row::operator=(char *s)
{
  char *t;
  for(int i = 0;i < n;i++) {
    t = strchr(s,b_to_c(i));
    if(t != NULL)
      data[t - s] = i;
  }
  return *this;
}

// Copy one row to another
row& row::operator=(const row& r)
{
  if(r.data != NULL) {
    if(n != r.n) {
      delete[] data;
      data = new char[n = r.n];
    }
    memcpy(data,r.data,n);
  } else
    data = NULL;
  return *this;
}

// Compare two rows
int row::operator==(const row& r) const
{
  for(int i = 0;i < n;i++)
    if(data[i] != r.data[i]) return 0;
  return 1;
}

// Transpose one row by another
row row::operator*(const row& r) const
{
  row product(n);
  for(int i = 0;i < n;i++)
    product.data[i] = data[r.data[i]];
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
  row quotient(n);
  for(int i = 0;i < n;i++)
    quotient.data[r.data[i]] = data[i];
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
  row result(n);
  for(int i = 0;i < n;i++)
    result.data[data[i]] = i;
  return result;
}

// Apply a change to a row
row& operator*=(row& r, const change& c)
{
  char t;

  for(char *s = c.swaps;*s != 100;s++) {
    t = r.data[*s];
    r.data[*s] = r.data[*s + 1];
    r.data[*s + 1] = t;
  }
  return r;
}

row row::operator*(const change& c) const
{
  row result(*this);
  result *= c;
  return result;
}

// Convert a bell number to a printable char
char row::b_to_c(char b)
{
  if(b < 33)
    return row::symbols[b];
  else
    return '*';
}

// Convert a char to a bell number
char row::c_to_b(char c)
{
  int b = -1;
  c = toupper(c);
  for(int i = 0;i < 33;i++)
    if(b_to_c(i) == c) b = i;
  return b;
}

// Print it to a string
char *row::print(char *s) const
{
  char *t = s;
  for(int i = 0;i < n;i++)
    *t++ = b_to_c(data[i]);
  *t = 0;
  return s;
}

// Write it to a stream
ostream& operator<<(ostream &o, const row& r)
{
  char *s = new char[r.bells()+1];
  r.print(s);
  o << s;
  delete[] s;
  return o;
}

// Set it to rounds
row& row::rounds(void)
{
  for(int i = 0;i < n;i++)
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
  for(int i = 0;i < n;i++)
    if(data[i] != i) return 0;
  return 1;
}

// Return which plain bob lead head it is
int row::ispblh(void) const
{
  int h;
  for(h = 0; data[h] == h; h++);
  if(h == 0) return 0;
  row l = pblh(n, h), r = l;
  for(int i = 1;i < n;i++) {
    if(*this == r) return i;
    r *= l;
  }
  return 0;
}

// Express it as a product of disjoint cycles
char *row::cycles(char *result) const
{
  char *done = new char[n];
  int i;

  for(i = 0;i < n;i++) done[i] = 0;
  i = 0;
  char *s = result;
  for(;;) {
    while(i < n && done[i]) i++; // Find the next bell we haven't got yet
    if(i == n) break;
    do {
      *s++ = symbols[i];        // Write it to the string
      done[i] = 1;              // Remember that we've done it
      i = data[i];              // Find the next one in this cycle
    } while(!done[i]);          // until the finish the cycle
    *s++ = ',';                 // Write a separator
  }
  *(s-1) = '\0';                // Get rid of the final separator
  delete[] done;
  return result;
}

// Return the order of a row
int row::order(void) const
{
  buffer cyc(2*n);
  char *s;
  int o = 1, i;

  // We do this by expressing the row as disjoint cycles, then by taking the
  // lowest common multiple of the lengths of all the cycles.
  // This may seem like overkill. We could just keep multiplying the row by
  // itself until we got back to rounds. That would be OK for smaller numbers
  // but the maximum order on 16 bells is (I think) 4x5x7 = 140. It's quicker
  // to do it this way, especially as Euclid's algorithm is pretty quick.

  cycles(cyc);                  // Get the cycles representation
  for(s = strtok(cyc,",");s != NULL;s = strtok(NULL,","))   // For each cycle in the string
    o = lcm(o,strlen(s));
  return o;
}

// Return the sign of a row
int row::sign(void) const
{
  char *c = new char[n*2];
  cycles(c);                    // Express it in cycles
  int sgn = (strlen(c) + 1) & 1; // Just take the length of the string
  delete[] c;
  return sgn ? 1 : -1;
}


// *********************************************************************
// *                    Functions for class row_block                  *
// *********************************************************************

// Constructor
row_block::row_block(const changes &c) : rows(c.length() + 1,c.bells())
{
  ch = &c;
  (*this)[0].rounds();
  recalculate(0);
}

// Another constructor
row_block::row_block(const changes &c, const row &r) : rows(c.length() + 1,c.bells())
{
  ch = &c;
  (*this)[0] = r;
  recalculate();
}

// Recalculate rows from changes
row_block& row_block::recalculate(int start)
{
  int i = this->length() - 1;
  for(;start < i;start++)
    (*this)[start + 1] = (*this)[start] * (*ch)[start];
  return *this;
}

RINGING_END_NAMESPACE















