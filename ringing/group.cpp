// -*- C++ -*- group.cpp - Class representing a permutation group
// Copyright (C) 2003, 2009, 2011 Richard Smith <richard@ex-parrot.com>

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
#else
#include <set>
#include <vector>
#include <algorithm>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

void generate_group_nonrecursive( set<row>& s, const row& r0, 
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

void generate_group_recursive( set<row>& s, const row& r, 
			       const vector<row>& generators )
{
  for ( vector<row>::const_iterator 
	  i( generators.begin() ), e( generators.end() );
	i != e;  ++i )
    {
      const row r2( r * *i );
      if ( s.insert( r2 ).second )
	generate_group_recursive( s, r2, generators );
    }
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
  return g;
}

group::group( const row& gen )
  : b( gen.bells() )
{
  set< row > s;
  generate_group_nonrecursive( s, row(gen.bells()), vector<row>(1u, gen) );
  v.reserve(s.size());
  copy( s.begin(), s.end(), back_inserter(v) );
}

group::group( const row& g1, const row& g2 )
  : b( g1.bells() > g2.bells() ? g1.bells() : g2.bells() )
{
  vector<row> gens; gens.reserve(2); 
  gens.push_back(g1); 
  if (g2 != g1) gens.push_back(g2);

  set< row > s;

  size_t b( g1.bells() > g2.bells() ? g1.bells() : g2.bells() );
  generate_group_nonrecursive( s, row(b), gens );
  v.reserve(s.size());
  copy( s.begin(), s.end(), back_inserter(v) );
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

  generate_group_nonrecursive( s, row(b), uniq_gens );
  v.reserve(s.size());
  copy( s.begin(), s.end(), back_inserter(v) );
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


RINGING_END_NAMESPACE
