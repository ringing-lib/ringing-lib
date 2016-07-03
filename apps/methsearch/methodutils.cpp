// -*- C++ -*- methodutils.h - utility functions missing from the ringing-lib
// Copyright (C) 2002, 2003, 2004, 2005, 2009, 2010, 2011
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

#include "methodutils.h"
#include <string>
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#include <iostream.h>
#else
#include <stdexcept>
#include <iostream>
#endif
#if RINGING_OLD_C_INCLUDES
#include <stddef.h>
#include <assert.h>
#else
#include <cstddef>
#include <cassert>
#endif
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/streamutils.h>
#include <ringing/mathutils.h>
#include <iterator>


RINGING_USING_NAMESPACE
RINGING_USING_STD

bool is_cyclic_le( const row &lh, int hunts )
{
  const int n( lh.bells() - hunts );

  // Rounds doesn't count...
  if ( lh[hunts] == hunts ) 
    return false;

  {
    for ( int i=0; i<hunts; ++i )
      if ( lh[i] != i )
	return false;
  }

  {
    for ( int i=hunts+1; i<lh.bells(); ++i )
      if ( (lh[i] - hunts) % n != (lh[i-1] + 1 - hunts) % n )
	return false;
  }

  return true;
}

bool is_division_false( const method &m, const change &c, 
                        size_t div_start, size_t cur_div_len )
{
  size_t const len = m.length();
  if ( len - div_start < 3 || len - div_start == cur_div_len-1 )
    return false;

  row r( c.bells() );

  vector< row > rows( 1, r );
  rows.reserve( cur_div_len );

  for ( int i = div_start; i < len; ++i )
    {
      r *= m[i];
      rows.push_back( r );
    }

  if ( find( rows.begin(), rows.end(), r * c ) != rows.end() )
    return true;

  return false;
}

bool is_too_many_places( const method &m, const change &c, size_t max, 
                         size_t stopoff )
{
  for ( int i=0; i<c.bells(); ++i )
    if ( c.findplace(i) )
      {
	size_t count(2u);

	for ( ; count <= size_t(m.length())+1; ++count ) {
          size_t o = m.length() - count + 1;
          if ( o == stopoff || !m[o].findplace(i) )
	    break;
        }

	if ( count > max )
	  return true;
      }

  return false;
}

bool has_consec_places( const change &c, size_t max_count )
{
  size_t count(0);
  for ( int i=0; i<c.bells() && count <= max_count ; ++i )
    if ( c.findplace(i) )
      ++count;
    else
      count = 0;

  return count > max_count;
}

bool division_bad_parity_hack( const method &m, const change &c, 
                               size_t div_start, size_t cur_div_len )
{
  // It should be possible to do this easily without calculating rows.

  row r( c.bells() );

  vector< row > rows( 1, r );
  rows.reserve( cur_div_len );

  for ( int i = div_start; i < m.length(); ++i )
  {
    r *= m[i];
    rows.push_back( r );
  }
  
  rows.push_back( r * c );

  assert( rows.size() == cur_div_len );

  size_t even[2] = { 0u, 0u }, odd[2] = { 0u, 0u };
  
  for ( unsigned int i = 0; i < rows.size(); ++i )
    {
      if ( rows[i].sign() == +1 )
	++even[i%2];
      else
	++odd[i%2];
    } 

  if ( even[0] != odd[0] || even[1] != odd[1] )
    return true; // Bad
    
  return false; // OK
}

bool has_rotational_symmetry( const method &m )
{
  const int n( m.size() );

  {
    // Rotational symmetry about a change
    for ( int i=0; i<n/2+1; ++i )
      {
	// Try m[i] as the rotational symmetry point
	bool ok = true;

	for ( int j=0; j<n/2+1 && ok; ++j )
	  if ( m[ (i+j)%n ] != m[ (n+i-j)%n ].reverse() )
	    ok = false;

	if (ok)
	  return true;
      }
  }
  {
    // Rotational symmetry about a row
    for ( int i=0; i<n/2+1; ++i )
      {
	// Try m[i] / m[(i+1)%n] as the rotational symmetry point
	bool ok = true;

	for ( int j=0; j<n/2 && ok; ++j )
	  if ( m[ (i+j+1)%n ] != m[ (n+i-j)%n ].reverse() )
	    ok = false;

	if (ok)
	  return true;
      }
  }
  return false;
}

bool has_conventional_symmetry( const method& m )
{
  const int n( m.size() );
  for ( int i=0; i < (n%2==0 ? n/2 : n); ++i )
    {
      // try m[i] as the sym point
      bool ok(true);

      for ( int j=1; ok && j<(n%2==0 ? n/2 : n/2+1); ++j ) 
	if ( m[(i+j) % n] != m[(i-j+n) % n] )
	  ok = false;

      if (ok) return true;
    }

  return false;
}

bool has_mirror_symmetry( const method& m )
{
  const int n( m.size() );
  for ( int i=0; i<n; ++i )
    if ( m[i] != m[i].reverse() )
      return false;

  return true;
}

bool has_glide_symmetry( const method& m )
{
  const int n( m.size() );
  if ( n % 2 == 1 ) return false;

  for ( int i=0; i<n/2; ++i )
    if ( m[i] != m[(i + n/2) % n].reverse() )
      return false;

  return true;
}

string method_symmetry_string( const method& m )
{
  string rv;
  if ( has_conventional_symmetry(m) ) rv += 'P';
  if ( has_mirror_symmetry(m) )       rv += 'M';
  if ( has_glide_symmetry(m) )        rv += 'G';
  if ( has_rotational_symmetry(m) )   rv += 'R';
  return rv;
}

namespace {
inline bell operator*( bell const& b, row const& r ) { return r[b]; }
inline bell& operator*=( bell& b, row const& r ) { return b = r[b]; }
}

string tenors_together_coursing_order( const method& m )
{
  const row lh( m.lh() );

  int i(0);
  bell b( m.bells() - 1 );
  do {
    b *= lh, ++i;
    assert( i <= m.bells() );
  } while ( b < m.bells() - 2 );



  if ( b == m.bells() - 1 ) // 7 and 8 are in different orbits
    throw runtime_error( "Unable to get a tenors together coursing order" );
  
  assert( b == m.bells() - 2 );

  row cg(lh);
  for ( int j=1; j<i; ++j ) cg *= lh;

  make_string ms;  
  ms << b;

  do {
    ms << (b *= cg);
  } while ( b != m.bells() - 1 );

  return ms;
}

bool compare_changes( change const& a, change const& b )
{
  if (a.count_places() == 0)
    return b.count_places() > 0;
  else if (b.count_places() == 0)
    return false;

  // TODO:  This coule be much more efficient
  const string sa(a.print()), sb(b.print());
  vector<bell> pa, pb; 
  transform( sa.begin(), sa.end(), back_inserter(pa), bell::read_char );
  transform( sb.begin(), sb.end(), back_inserter(pb), bell::read_char );
  return pa < pb;
}

namespace {
char const* old_lhcode_6( bool unicode, method const& m )
{
  row const lh( m.lh() );
  change const c12( m.bells(), "12" );

  bool const seconds( m.back() == c12 );

  // Only handle seconds and sixth place methods
  if ( !seconds && m.back() != change( m.bells(), "1" ) ) {
    if ( m.back() != change(m.bells(), "14") ) return "?";

    if ( lh == "145362" ) return "X";
    if ( lh == "162534" ) return "Y";

    if ( unicode && lh == "136524" ) return "\xCE\xBC"; // mu
    if ( unicode && lh == "152643" ) return "\xCE\xB8"; // theta
    if ( unicode && lh == "164235" ) return "\xCE\xB4"; // delta
    if ( unicode && lh == "146325" ) return "\xCE\xA8"; // Psi
    if ( unicode && lh == "154263" ) return "\xCE\xA3"; // Sigma
    if ( unicode && lh == "135642" ) return "\xCE\xBB"; // lambda

    return "?";
  }

  switch (lh[1])
    {
    case 2:
      if ( lh == "135264" ) return seconds ? "G" : "L";
      if ( lh == "136245" ) return seconds ? "Q" : "?";
      if ( lh == "134625" ) return seconds ? "?" : "W";
      break;
      
    case 3:
      if ( lh == "142635" ) return seconds ? "K" : "O";
      if ( lh == "142563" ) return seconds ? "P" : "?";
      if ( lh == "146532" ) return seconds ? "?" : "U";
      break;
      
    case 4:
      if ( lh == "156342" ) return seconds ? "H" : "M";
      if ( lh == "154632" ) return seconds ? "S" : "?";
      if ( lh == "152364" ) return seconds ? "?" : "T";
      break;
      
    case 5:
      if ( lh == "164523" ) return seconds ? "J" : "N";
      if ( lh == "165324" ) return seconds ? "R" : "?";
      if ( lh == "165243" ) return seconds ? "?" : "V";
      break;
    }

  return "?";
}

char const* old_lhcode_5( method const& m )
{
  row const lh( m.lh() );
  change const c1(m.bells(), "1"),   c2(m.bells(), "12"), 
               c3(m.bells(), "123"), c4(m.bells(), "14");

  bool const b1(m.back() == c1), b2(m.back() == c2), 
             b3(m.back() == c3), b4(m.back() == c4);

  if ( !b1 && !b2 && !b3 && !b4 ) 
    return "?";

  switch (lh[1])
    {
    case 1:
      if ( lh == "12534" ) return b2 ? "G" : b3 ? "T" : "?";
      if ( lh == "12453" ) return b2 ? "K" : b3 ? "S" : "?";
      break;
 
    case 2:
      if ( lh == "13524" ) return b2 ? "A" : b1 ? "C" : "?";
      if ( lh == "13542" ) return b4 ? "Z" : "?";
      if ( lh == "13425" ) return b2 ? "H" : b4 ? "X" : "?";
      if ( lh == "13452" ) return b1 ? "F" : "?";
      break;
 
    case 3:
      if ( lh == "14253" ) return b2 ? "B" : b1 ? "D" : "?";
      if ( lh == "14235" ) return b2 ? "J" : b4 ? "W" : "?";
      if ( lh == "14352" ) return b3 ? "U" : "?";
      if ( lh == "14532" ) return b3 ? "R" : b4 ? "N" : "?";
      break;

    case 4:
      if ( lh == "15234" ) return b1 ? "E" : "?";
      if ( lh == "15243" ) return b4 ? "Y" : "?";
      if ( lh == "15423" ) return b3 ? "Q" : b4 ? "M" : "?";
      if ( lh == "15324" ) return b3 ? "V" : "?";
      break;
    }

  return "?";
}
}

char const* old_lhcode( bool unicode, method const& m ) 
{
  if ( m.bells() == 6 ) 
    return old_lhcode_6(unicode, m);
  else if ( m.bells() == 5 ) 
    return old_lhcode_5(m);
  else
    return "?";
}

unsigned long staticity( method const& m )
{
  unsigned long s = 0ul; //1ul;
  for ( int i=0, b=m.bells(); i!=b-1; ++i)
  {
    int j=0, l=m.size();
    while ( j!=l && ( m[j].findswap(i) || 
                      m[j].findplace(i) && m[j].findplace(i+1) ) ) 
      ++j;
    if (j==l) 
      return static_cast<unsigned long>(-1);

    for ( int k=j; k<j+l; ++k )
    {
      int k0=k;
      while ( k!=j+l && ( m[k%l].findswap(i) || 
                          m[k%l].findplace(i) && m[k%l].findplace(i+1) ) )
        ++k;
      if (k!=k0) {
        // s *= ipower( 2, k-k0-1 );
        s += k-k0-1;
      }
    }
  }
  return s;
}
