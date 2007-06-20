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
  : nw(nw), nh(nh), nt(nt), end(false), 
    r(nt), s(r.print())
{}

extent_iterator::extent_iterator( unsigned int nw, unsigned int nh )
  : nw(nw), nh(nh), nt(nw+nh), 
    end(false),
    r(nt), s(r.print())
{}

extent_iterator &extent_iterator::operator++()
{
  end = ! next_permutation( s.begin() + nh, s.end() + (nh + nw - nt),
			    bellsym_cmp() );
  if (!end) r = s.c_str();
  return *this;
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
  row r(nt);
  vector<bell> v;
  r.swap(v);

  for ( size_t i=nh; i<nw+nh; ++i )
  {
    size_t const fact = factorial(nw + nh - i - 1);
    v[i] = n / fact + nh; n %= fact;
     
    for ( bell b(nh); b <= v[i]; ++b )
      if ( find( v.begin()+nh, v.begin()+i, b ) != v.begin()+i )
        ++v[i];
  }

  r.swap(v);
  return r;
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
      end = true;
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
  : nw(nw), nh(nh), end(false), c(nw+nh)
{
  stk.push_back(nh); 
  if ( nw>1 ) next(); 
}

changes_iterator::changes_iterator( unsigned int nw, unsigned int nh, 
				    unsigned int nt )
  : nw(nw), nh(nh), end(false), c(nt) 
{
  stk.push_back(nh); 
  if ( nw>1 ) next(); 
}

row random_row( unsigned int nw, unsigned int nh, unsigned int nt )
{
  int idx( random_int( factorial(nw) ) );
  string s( nt, '?' );
  unsigned i;
  for (i=0; i<nh; ++i) s[i] = bell(i).to_char();
  for (i=nh; i<nw+nh; ++i)
    {
      const int fact( factorial(nw-i+nh-1) );
      unsigned int b = idx / fact;
      idx %= fact;
      
      for (unsigned ob=0; ob<=b; ++ob)
	for (unsigned j=0; j<i; ++j)
	  if ( bell(ob).to_char() == s[j] )
	    ++b;
      
      s[i] = bell(b).to_char();
    }
  for (i=nw+nh; i<nt; ++i) s[i] = bell(i).to_char();
  return s;
}


RINGING_END_NAMESPACE

