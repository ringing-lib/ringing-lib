// -*- C++ -*- methodutils.h - utility functions missing from the ringing-lib
// Copyright (C) 2002, 2003, 2004 Richard Smith <richard@ex-parrot.com>

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
#else
#include <stdexcept>
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


RINGING_USING_NAMESPACE
RINGING_USING_STD

change merge_changes( const change &a, const change &b )
{
  if ( a.bells() != b.bells() ) 
    throw logic_error("Mismatched numbers of bells");

  if ( (row() * a * b).order() > 2 )
    throw range_error("Result of change::operator& not a change");
  
  change c( a.bells() );

  for ( int i=0; i<a.bells()-1; ++i )
    if ( a.findswap(i) || b.findswap(i) )
      c.swappair(i);

  return c;
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

bool is_division_false( const method &m, const change &c, unsigned int divlen )
{
  if ( m.length() % divlen < 3 || m.length() % divlen == divlen-1 )
    return false;

  row r( c.bells() );

  vector< row > rows( 1, r );
  rows.reserve( divlen );

  for ( int i = (m.length() / divlen) * divlen; i < m.length(); ++i )
    {
      r *= m[i];
      rows.push_back( r );
    }

  if ( find( rows.begin(), rows.end(), r * c ) != rows.end() )
    return true;

  return false;
}

bool is_too_many_places( const method &m, const change &c, size_t max )
{
  for ( int i=0; i<c.bells(); ++i )
    if ( c.findplace(i) )
      {
	size_t count(2u);

	for ( ; count <= size_t(m.length())+1; ++count )
	  if ( !m[m.length()-count+1].findplace(i) )
	    break;

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
			       unsigned int divlen )
{
  // It should be possible to do this easily without calculating rows.

  row r( c.bells() );

  vector< row > rows( 1, r );
  rows.reserve( divlen );

  {
    for ( int i = (m.length() / divlen) * divlen; 
	  i < m.length(); ++i )
      {
	r *= m[i];
	rows.push_back( r );
      }
  }
  
  rows.push_back( r * c );

  if ( rows.size() != divlen )
    cerr << rows.size() << "!=" << divlen << endl;
  assert( rows.size() == divlen );

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

void do_single_compressed_pn( make_string &os, const change &ch, 
			      bool &might_need_dot, bool is_lh = false )
{
  const int n = ch.bells();
  if ( ch == change( ch.bells(), "X" ) && n % 2 == 0 ) 
    {
      os << '-'; might_need_dot = false;
    } 
  else 
    {
      string p( ch.print() );
      
      if ( p.size() > 1 )
	{
	  if ( p[0] == bell(0).to_char() )
	    p = p.substr(1);
	  if ( !p.empty() && p[ p.size()-1 ] == bell( n-1 ).to_char() )
	    p = p.substr( 0, p.size() - 1 );
	  if ( p.empty() )
	    p =  bell( is_lh ? n-1 : 0 ).to_char();
	}

      if (might_need_dot) os << '.';
      os << p;
      might_need_dot = true;
    }
}

string get_short_compressed_pn( const method &m )
{
  make_string os;

  bool might_need_dot(false);

  if ( m.issym() )
    {
      os << '&';
      for ( int i=0; i < m.length() / 2; ++i)
	do_single_compressed_pn( os, m[i], might_need_dot );
    }
  else
    {
      for ( int i=0; i < m.length() - 1; ++i)
	do_single_compressed_pn( os, m[i], might_need_dot );
    }

  return os;
}


bool has_rotational_symmetry( const method &m )
{
  {
    // Rotational symmetry about a change
    for ( int i=0, n=m.size(); i<n/2+1; ++i )
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
    for ( int i=0, n=m.size(); i<n/2+1; ++i )
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
  for ( int i=0; i<n/2; ++i )
    {
      // try m[i] as the sym point
      bool ok(true);

      for ( int j=1; ok && j<n/2; ++j ) 
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
  for ( int i=0; i<n/2; ++i )
    if ( m[i] != m[(i + n/2) % n].reverse() )
      return false;

  return true;
}

string method_symmetry_string( const method& m )
{
  assert( m.size() % 2 == 0 );

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
  return sa < sb;
}
