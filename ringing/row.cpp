// row.cpp - Classes for row and changes
// Copyright (C) 2001, 2008, 2009 Martin Bright <martin@boojum.org.uk>
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#else
#include <cctype>
#include <cstdlib>
#include <cstring>
#endif
#if RINGING_OLD_INCLUDES
#include <bvector.h>
#endif

#include <ringing/row.h>
#include <ringing/mathutils.h>

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0) && defined(_MSC_VER)
// Microsoft have deprecated strcpy in favour of a non-standard
// extension, strcpy_s.  4996 is the warning about it being deprecated.
#pragma warning (disable: 4996)
#endif

RINGING_USING_STD

RINGING_START_NAMESPACE

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
  validate();
}

void row::validate() const
{
#if RINGING_USE_EXCEPTIONS
  vector<bool> found(bells());
  int i;
  for(i=0; i<bells(); ++i) found[i] = false;
  for(i=0; i<bells(); ++i) 
    if (data[i] < 0 || data[i] >= (int) data.size() || found[data[i]])
      throw invalid(print());
    else
      found[data[i]] = true;
#endif
}

row::row(const string &s)
  : data(s.size())
{
  for(size_t i = 0, n = s.size(); i != n; ++i )
    data[i].from_char(s[i]);
  validate();
}

row::row(vector<bell> const& d)
  : data(d)
{
  validate();
}

row::invalid::invalid()
  : invalid_argument("The row supplied was invalid")
{}

row::invalid::invalid(const string& s)
  : invalid_argument("The row '" + s + "' was invalid") {} 

// Assign a string value to a row
row& row::operator=(const char *s)
{
  row(s).swap(*this);
  return *this;
}

row& row::operator=(const string &s)
{
  row(s).swap(*this);
  return *this;
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
  for(; i < bells(); i++)
    quotient.data[i] = data[i];
  for(; i < r.bells(); i++)
    quotient.data[r.data[i]] = i;
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
  while (r.bells() < c.bells())
    r.data.push_back(r.bells());

  if (c.n != 0 && !r.data.empty())
    for ( vector<bell>::const_iterator s = c.swaps.begin(), e = c.swaps.end(); 
          s != e && *s < (r.bells() - 1); ++s )
      RINGING_PREFIX_STD swap( r.data[*s], r.data[*s + 1] );

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
  s.reserve( bells() );

  if(!data.empty())
    for(vector<bell>::const_iterator i = data.begin(); 
	i != data.end(); ++i)
      s += i->to_char();
  return s;
}

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
char *row::print(char *s) const
{
  return strcpy( s, print().c_str() );
}
#endif

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
  if ( n <= 2 ) return r;

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
  if ( n <= 2 ) return r;

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

// Return tittums on n bells
row row::tittums(const int n)
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

  {
    change c(n);
    for(int i = h; i < n-1; i += 2)
      c.swappair(i);
    r *= c;
  }
  {
    change c(n);
    for(int i = h+1; i < n-1; i += 2)
      c.swappair(i);
    r *= c;
  }

  return r;
}

row row::cyclic(int n, int h, int c)
{
  row r(n);
  if ( h < n ) {
    c %= n-h;
    if (c < 0) c += n-h;
    rotate( r.data.begin() + h, r.data.begin() + h + c, r.data.end() );
  }
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
int row::ispblh(int h) const
{
  row l = pblh(bells(), h), r = l;
  for(int i = 1; i < (bells() - h); i++) {
    if(*this == r) return i;
    r *= l;
  }
  return 0;
}

// Return which plain bob lead head it is
int row::ispblh(void) const
{
  if(data.empty()) return 1;
  int h;
  for(h = 0; h < bells() && data[h] == h; h++);
  if(h == 0) return 0;
  return ispblh(h);
}

// Express it as a product of disjoint cycles
string row::cycles() const
{
  // Note: If you change the format of the output of this function,
  // make sure you verify that row::sign() still works.
  string result;
  result.reserve( 2*bells() ); // Although max. final valueis 2*bells()-1 we 
			       // can temporarily exceed this before the
			       // trailing ',' is removed.

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

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
// Express it as a product of disjoint cycles
char *row::cycles(char *result) const
{
  return strcpy( result, cycles().c_str() );
}
#endif

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
  if ( bells() == 0 ) return 1;
  return ( cycles().length() & 1 ) ? 1 : -1;
}

size_t row::hash() const
{
  // The standard FNV hash algorithm.

  // We use 31 as the FNV prime because it is a Mersenne prime: the FNV 
  // algorithm requires a prime, and being of the form 2^n-1 means that the
  // compiler / processor microcode can optimise the multiplication to a 
  // shift and subtract.

  size_t h = bells();
  for ( int i=0, n=bells(); i != n; ++i )
    h = 31*h + data[i];
  return h;
}

int row::find(bell const& b) const
{
  for ( int i=0, n=bells(); i != n; ++i )
    if ( data[i] == b ) 
      return i;

  // Assuming the row is not malformed, we must be on 
  // insufficient bells, so just return b.
  return b;
}

row row::power( int n ) const
{
  size_t const b = bells();
  if (n == 0)
    return row(b);
  else if (n < 0)
    return inverse().power( -n );
  else if (n > b*b) 
    // Only do this optimisation if n is large as order() is expensive
    return power( n % order() );
  else if (n % 2)
    return *this * power( n-1 );
  else {
    row temp = power( n/2 );
    return temp * temp;
  }
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
