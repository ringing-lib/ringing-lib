// -*- C++ -*- group.cpp - Class representing a permutation group
// Copyright (C) 2003, 2009, 2011, 2017 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/row.h>
#include <ringing/group.h>
#include <ringing/extent.h>
#include <ringing/mathutils.h>

#if RINGING_OLD_INCLUDES
#include <set.h>
#include <vector.h>
#include <algo.h>
#include <limits.h>
#else
#include <set>
#include <vector>
#include <algorithm>
#include <limits>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

// This algorithm is non-recursive because some groups are big
// enough that they cause a stack overflow when calculating them.
void generate_group( set<row>& s, const row& r0, 
                     const vector<row>& generators )
{
  vector<row> x( 1u, r0 );

  while (x.size()) {
    row r = x.back(); x.pop_back();

    for ( vector<row>::const_iterator 
	    i( generators.begin() ), e( generators.end() );
	  i != e;  ++i )
    {
      const row r2( r * *i );
      if ( s.insert( r2 ).second )
        x.push_back(r2);
    }
  }
}

bool is_direct_product_of_symmetric_groups( vector< vector<bell> > const& o,
                                            size_t gsize )
{
  size_t const largest_factorial
    = sizeof(size_t) == 2 ? 8       //  8!  < 2^16 < (8+1)!
    : sizeof(size_t) == 4 ? 12      //  12! < 2^32 < (12+1)! 
    : sizeof(size_t) == 8 ? 20      //  20! < 2^64 < (20+1)!
    : /* default */ 12;

  // Is the group \Prod S_{n_i} for some \{ n_i \}?
  size_t sz = 1;
  for ( vector< vector<bell> >::const_iterator i=o.begin(), e=o.end(); 
          i!=e; ++i ) {
    if ( i->size() > largest_factorial ) 
      return false;
    size_t const f = factorial(i->size());
    if ( f > numeric_limits<size_t>::max() / sz )
      return false; // We'd overflow
    sz *= f;
    if ( sz > gsize )
      return false;
  }
    
  return sz == gsize;
}

RINGING_END_ANON_NAMESPACE

group group::symmetric_group(int nw, int nh, int nt)
{
  if (!nt) nt = nh + nw;
  group g; 
  g.b = nt;
  g.v.clear();
  g.v.reserve( factorial(nw) );
  copy( extent_iterator(nw, nh, nt), extent_iterator(), 
	back_inserter(g.v) );
  g.calc_orbit_space();
  return g;
}

group group::alternating_group(int nw, int nh, int nt)
{
  if (!nt) nt = nh + nw;
  group g; 
  g.b = nt;
  g.v.clear();
  g.v.reserve( factorial(nw) / 2 );
  copy( incourse_extent_iterator(nw, nh, nt), incourse_extent_iterator(), 
	back_inserter(g.v) );
  g.calc_orbit_space();
  return g;
}

group group::trivial_group(int nt) 
{
  return group( row(nt) ); 
}

group group::dihedral_group_c(int nw, int nh, int nt)
{
  if (!nt) nt = nh + nw;
  return group( row::cyclic(nh+nw, nh), row::reverse_rounds(nw, nh, nt) );
}

group group::dihedral_group_r(int nw, int nh, int nt)
{
  if (!nt) nt = nh + nw;
  return group( row::pblh(nh+nw, nh), row::reverse_rounds(nw, nh, nt) );
}

group group::cyclic_group_c(int nw, int nh, int nt)
{
  if (!nt) nt = nh + nw;
  return group( row::cyclic(nh+nw, nh), row(nt) );
}

group group::cyclic_group_r(int nw, int nh, int nt)
{
  if (!nt) nt = nh + nw;
  return group( row::pblh(nh+nw, nh), row(nt) );
}
  
group group::direct_product( group const& a, group const& b )
{
  // Create a conjugate version of b with all bells shifted right by
  // a.bells() places.
  group b2( b.conjugate( row::cyclic(a.bells() + b.bells(), 0, -a.bells()) ) );

  vector<row> gens;
  gens.reserve(a.size() + b.size());
  copy( a.begin(), a.end(), back_inserter(gens) );
  copy( b2.begin(), b2.end(), back_inserter(gens) );
  return group(gens);
}

group::group( const row& gen )
  : b( gen.bells() )
{
  set< row > s;
  generate_group( s, row(gen.bells()), vector<row>(1u, gen) );
  v.reserve(s.size());
  copy( s.begin(), s.end(), back_inserter(v) );
  calc_orbit_space();
}

group::group( const row& g1, const row& g2 )
  : b( g1.bells() > g2.bells() ? g1.bells() : g2.bells() )
{
  vector<row> gens; gens.reserve(2); 
  gens.push_back(g1); 
  if (g2 != g1) gens.push_back(g2);

  set< row > s;

  size_t b( g1.bells() > g2.bells() ? g1.bells() : g2.bells() );
  generate_group( s, row(b), gens );
  v.reserve(s.size());
  copy( s.begin(), s.end(), back_inserter(v) );
  calc_orbit_space();
}

group::group( const vector<row>& gens )
  : b(0)
{
  if (gens.empty()) { v.push_back( row() ); return; }

  set< row > s;

  vector<row> uniq_gens(gens);
  sort( uniq_gens.begin(), uniq_gens.end() );
  uniq_gens.erase( unique( uniq_gens.begin(), uniq_gens.end() ), 
		   uniq_gens.end() );

  // Find the maximum number of bells in any of the gens
  for ( vector<row>::const_iterator 
	  i( uniq_gens.begin() ), e( uniq_gens.end() );  i != e;  ++i )
    if ( size_t(i->bells()) > b ) 
      b = i->bells();

  generate_group( s, row(b), uniq_gens );
  v.reserve(s.size());
  copy( s.begin(), s.end(), back_inserter(v) );
  calc_orbit_space();
}

group group::conjugate( const row& r ) const
{
  group g;

  g.b = b;
  g.v.clear();  // Default constructor populates it with a single element
  g.v.reserve( v.size() );

  const row ri( r.inverse() );

  for ( vector<row>::const_iterator i( v.begin() ), e( v.end() );
	i != e;  ++i )
    g.v.push_back( ri * *i * r );

  // Keep it sorted so that comparison is cheap(ish)
  sort( g.v.begin(), g.v.end() );
  g.calc_orbit_space();

  return g;
}

bool operator==( const group& a, const group& b )
{
  return a.b == b.b && a.v == b.v;
}

bool operator<( const group& a, const group& b )
{
  return a.b < b.b || a.b == b.b && a.v < b.v;
}

bool operator>( const group& a, const group& b )
{
  return a.b > b.b || a.b == b.b && a.v > b.v;
}

row group::rcoset_label( row const& r ) const
{
  if ( size() == 1 ) return r;

  // |G|=6 is the point at which the O(1) algorithm beats the O(|G|) one.
  if ( size() >= 6 && is_direct_product_of_symmetric_groups( o, size() ) ) 
  {
    // Use an O(1) algorithm.
    vector<bell> label( r.bells() );

    for ( vector< vector<bell> >::const_iterator i=o.begin(), e=o.end(); 
            i != e; ++i ) {
      size_t n = i->size();
      vector<bell> v;
      for ( size_t j=0; j<n; ++j )
        v.push_back( r.find( (*i)[j] ) );

      sort( v.begin(), v.end() );
      for ( size_t j=0; j<n; ++j ) {
        // label[ (*i)[j] ] = v[j];  // lcoset case
        label[ v[j] ] = (*i)[j];
      }
    }

    return row(label);
  }

  // Default to an O(|G|) algorithm
  row label;
  for ( const_iterator i=v.begin(), e=v.end(); i != e; ++i ) 
  {
    row const ir = *i * r;
    if (label.bells() == 0 || ir < label) 
      label = ir;
  }
  return label;
}

row group::lcoset_label( row const& r ) const
{
  row label;
  for ( const_iterator i=v.begin(), e=v.end(); i != e; ++i ) 
  {
    row const ir = r * *i;
    if (label.bells() == 0 || ir < label) 
      label = ir;
  }
  return label;
}

void group::calc_orbit_space() const
{
  vector< set<bell> > orbs( bells() );

  for ( const_iterator i=v.begin(), e=v.end(); i != e; ++i ) 
    for ( size_t j=0; j<bells(); ++j )
      orbs[j].insert( (*i)[j] );
 
  o.clear(); 
  for ( size_t j=0; j<bells(); ++j ) {
    vector<bell> oj( orbs[j].begin(), orbs[j].end() );
    if ( find( o.begin(), o.end(), oj ) == o.end() )
      o.push_back(oj);
  }
}

vector<bell> group::invariants() const
{
  if ( bells() && o.empty() )
    calc_orbit_space();

  vector<bell> rv;
  for ( vector< vector<bell> >::const_iterator i=o.begin(), e=o.end(); 
          i!=e; ++i )
    if ( i->size() == 1 )
      rv.push_back(i->front());
  return rv;
}

RINGING_END_NAMESPACE
