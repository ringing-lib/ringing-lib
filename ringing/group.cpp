// -*- C++ -*- group.cpp - Class representing a permutation group
// Copyright (C) 2003 Richard Smith <richard@ex-parrot.com>

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
#include <ringing/row.h>
#include <ringing/group.h>
#include <ringing/extent.h>
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
  g.v.clear();
  g.v.reserve( factorial(nw) / 2 );
  copy( incourse_extent_iterator(nw, nh, nt), incourse_extent_iterator(), 
	back_inserter(g.v) );
  return g;
}

group::group( const row& gen )
{
  set< row > s;
  generate_group_recursive( s, row(gen.bells()), vector<row>(1u, gen) );
  copy( s.begin(), s.end(), back_inserter(v) );
}

group::group( const row& g1, const row& g2 )
{
  vector<row> gens; gens.reserve(2); 
  gens.push_back(g1); gens.push_back(g2);

  set< row > s;

  size_t b( g1.bells() > g2.bells() ? g1.bells() : g2.bells() );
  generate_group_recursive( s, row(b), gens );
  copy( s.begin(), s.end(), back_inserter(v) );
}

group::group( const vector<row>& gens )
{
  set< row > s;

  // Find the maximum number of bells in any of the gens
  size_t b(0);

  for ( vector<row>::const_iterator i( gens.begin() ), e( gens.end() );
	i != e;  ++i )
    if ( i->bells() > b ) 
      b = i->bells();

  generate_group_recursive( s, row(b), gens );
  copy( s.begin(), s.end(), back_inserter(v) );
}

RINGING_END_NAMESPACE
