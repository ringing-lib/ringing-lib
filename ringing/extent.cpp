// -*- C++ -*- extent.h - Classes for iterating through an extent
// Copyright (C) 2001, 2002, 2003, 2005, 2007
// Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#else
#include <cassert>
#endif
#if RINGING_OLD_INCLUDES
#include <algo.h>
#else
#include <algorithm>
#endif

#include <ringing/extent.h>
#include <ringing/mathutils.h>


RINGING_START_NAMESPACE

RINGING_USING_STD

struct extent_iterator::bellsym_cmp
{
  bool operator()( char a, char b )
  {
    bell aa, bb;
    aa.from_char(a);
    bb.from_char(b);
    return aa < bb;
  }
};

extent_iterator::extent_iterator( unsigned int nw, unsigned int nh, 
				  unsigned int nt )
  : nw(nw), nh(nh), nt(nt), end_(false), r(nt)
{}

extent_iterator::extent_iterator( unsigned int nw, unsigned int nh )
  : nw(nw), nh(nh), nt(nw+nh), end_(false), r(nt)
{}

extent_iterator &extent_iterator::operator++()
{
  vector<bell> v; r.swap(v);
  end_ = ! next_permutation( v.begin() + nh, v.end() + (nh + nw - nt) );
  if (!end_) r.swap(v);
  return *this;
}

extent_iterator &extent_iterator::operator--()
{
  if (end_) { // back rounds on working bells
    vector<bell> v(nt);
    for ( unsigned i=0; i<nh; ++i ) v[i] = i;
    for ( unsigned i=nh; i<nw+nh; ++i ) v[i] = nw+nh-1 - i + nh;
    for ( unsigned i=nw+nh; i<nt; ++i ) v[i] = i;
    r.swap(v);
  } 
  else {
    vector<bell> v; r.swap(v);
    prev_permutation( v.begin() + nh, v.end() + (nh + nw - nt) );
    r.swap(v);
  }
  return *this;
}

bool extent_iterator::operator<( const extent_iterator& i ) const
{
  if (i.end_) return !end_;
  else if (end_) return false;
  else return r < i.r; 
}

extent_iterator extent_iterator::end( unsigned nw, unsigned nh, unsigned nt )
{ 
  extent_iterator e(nw, nh, nt); 
  e.end_ = true; 
  return e;
}

RINGING_API size_t
position_in_extent( row const& r, unsigned nw, unsigned nh, unsigned nt )
{
  size_t const b = r.bells();

  if (b > nt) 
    throw out_of_range( "Row has too many bells" );
  for ( size_t i=0; i<nh; ++i )
    if ( r[i] != i )
      throw out_of_range( "Row does not have fixed trebles" );
  for ( size_t i=nh+nw; i<nt; ++i )
    if ( r[i] != i )
      throw out_of_range( "Row does not have fixed tenors" );

  size_t x = 0u;
  for ( size_t i=nh; i<nw+nh; ++i )
  {
    x *= nw + nh - i;
    for ( size_t j=i+1; j<nw+nh; ++j )
      if ( j < b && r[j] < r[i] ) 
        ++x;
  }

  return x;
}

RINGING_API row
nth_row_of_extent( size_t n, unsigned nw, unsigned nh, unsigned nt )
{
  vector<bell> v(nt);

  for ( unsigned i=0; i<nh; ++i ) v[i] = i;
  for ( unsigned i=nh; i<nw+nh; ++i )
  {
    size_t const fact = factorial(nw + nh - i - 1);
    bell b = n / fact + nh; n %= fact;
     
    for ( bell ob(nh); ob <= b; ++ob )
      for ( unsigned j=0; j<i; ++j )
        if ( ob == v[j] ) {
          ++b; break;
        }

    v[i] = b;
  }
  for ( unsigned i=nw+nh; i<nt; ++i ) v[i] = i;
  return row(v);
}

RINGING_API int
sign_of_nth_row_of_extent( size_t n )
{
  // This algorithm is documented here:
  // <http://ex-parrot.com/~richard/r-t/2007/06/001763.html>
  int sign = +1;
  for ( unsigned i = 1, f = i; n; f *= ++i ) {
    sign *= (n % i) % 2 == 0 ? +1 : -1;
    n /= i;
  }
  return sign;
}

extent_iterator extent::begin() const
{
  return extent_iterator(nw, nh, nt); 
}

extent_iterator extent::end() const
{ 
  return extent_iterator::end(nw, nh, nt); 
}

row extent::operator[]( size_t n ) const 
{
  return nth_row_of_extent(n, nw, nh, nt);
}

size_t extent::operator[]( row r ) const
{
  return position_in_extent( r, nw, nh, nt );
}

size_t extent::size() const
{
  return factorial(nw); 
}


RINGING_API row
nth_row_of_incourse_extent( size_t n, unsigned nw, unsigned nh, unsigned nt )
{
  row r( nth_row_of_extent( n*2, nw, nh, nt ) );
  if ( r.sign() < 0 ) {
    change c(nt); 
    c.swappair(nw+nh-2); 
    r *= c;
  }
  return r;
}


incourse_extent_iterator &incourse_extent_iterator::operator++() 
{
  do { 
    ++ei; 
  } while ( !ei.is_end() && ei->sign() < 0);
  return *this;
}

void changes_iterator::next()
{
  if ( nw == 0 || stk.size() == nw && stk.back() == nw+nh-1 )
    {
      end_ = true;
      return;
    }
  
  while ( stk.back() > nh+1 && !c.findswap( stk.back()-2 ) )
    stk.pop_back();
  
  if ( c.findswap( stk.back()-2 ) )
    {
      c.swappair( stk.back() - 2 );
      --stk.back();
    }
  
  while ( stk.back() < nw+nh-1 )
    {
      c.swappair( stk.back() );
      stk.push_back( stk.back() + 2 );
    }
}

changes_iterator::changes_iterator( unsigned int nw, unsigned int nh )
  : nw(nw), nh(nh), end_(false), c(nw+nh)
{
  stk.push_back(nh); 
  if ( nw>1 ) next(); 
}

changes_iterator::changes_iterator( unsigned int nw, unsigned int nh, 
				    unsigned int nt )
  : nw(nw), nh(nh), end_(false), c(nt) 
{
  stk.push_back(nh); 
  if ( nw>1 ) next(); 
}

row random_row( unsigned int nw, unsigned int nh, unsigned int nt )
{
  return nth_row_of_extent( random_int( factorial(nw) ), nw, nh, nt );
}


RINGING_END_NAMESPACE

