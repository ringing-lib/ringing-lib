// -*- C++ -*- multtab.cpp - A precomputed multiplication table of rows
// Copyright (C) 2002, 2003, 2004, 2005 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/multtab.h>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <iomanip.h>
#include <map.h>
#include <set.h>
#else
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#endif
#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#include <math.h>
#else
#include <cassert>
#include <cmath>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

int multtab::bells() const
{
  return rows.empty() ? 0 : rows.front().bells();
}

void multtab::swap( multtab &other )
{
  rows.swap( other.rows );
  table.swap( other.table );
  pends.swap( other.pends );
  cols.swap( other.cols );
}

multtab &multtab::operator=( const multtab &other ) 
{ 
  multtab(other).swap(*this);
  return *this;
}

row multtab::make_post_representative( const row& r ) const
{
  if ( postgroup.size() < 2 ) return r;

  row res;

  for ( group::const_iterator i( postgroup.begin() ), e( postgroup.end() ); 
	i != e; ++i )
    {
      row const x( r * *i );
      if ( (res.bells() == 0 || x < res) && 
	   // Handle case where pends and postgroup are not subsets of rows
	   RINGING_PREFIX_STD find( rows.begin(), rows.end(), 
				    x ) != rows.end() )
	res = x;
    }

  return res;
}

row multtab::make_representative( const row& r ) const
{
  row res(r);

  for ( group::const_iterator i( pends.begin() ), e( pends.end() ); 
	i != e; ++i )
    {
      row const x( make_post_representative( *i * r ) );
      if ( x.bells() && x < res )
	res = x;
    }

  return res;
}

void multtab::init( const vector< row >& r )
{
  // This, and the two functions above, are complicated by having to
  // handle the case where the part end group is not a subset of row 
  // list.  (This occurs when, for example, generating a list of 
  // 8-bell course ends with the standard nine-part part end
  // group, <13425678, 12345786>.)
  

  rows = r; // so that we can call make_representative, below

  set<row> rows2;

  for ( vector<row>::const_iterator i( r.begin() ), e( r.end() ); 
	i != e; ++i ) 
    rows2.insert( make_representative(*i) );

  // TODO:  Better error checking for this:
  assert( rows2.size() == r.size() / pends.size() );

  // Force the capacity to be reduced.  
  // (Calling clear doesn't necessarily do this.)
  vector<row>().swap(rows);
  rows.reserve( r.size() / pends.size() );

  copy( rows2.begin(), rows2.end(), back_inserter(rows) );

  table.resize( rows.size() );
}

void multtab::dump( ostream &os ) const
{
  const int width( (int)ceil( log10( (float)table.size() ) ) );

  // Column headings
  if ( table.size() )
    {
      os << string( width + 5 + rows[0].bells(), ' ' );
      for ( size_t j = 0; j < table[0].size(); ++j )
	os << setw(width) << j << " ";
      os << "\n";
    }

  for ( row_t i; i.n != table.size(); ++i.n )
    {
      os << setw(width) << i.n << ")  " << rows[i.n] << "  ";
      for ( vector< row_t >::const_iterator j( table[i.n].begin() );
	    j != table[i.n].end(); ++j )
	{
	  os << setw(width) << j->n << " ";
	}
      os << "\n";
    }

  os << endl;
}

multtab::pre_col_t 
multtab::compute_pre_mult( const row &r )
{
  // This only makes sense if r commutes with the partends
  {
    for ( vector<row>::const_iterator i( pends.begin() ), e( pends.end() ); 
	  i != e; ++i )
      if ( r * *i != *i * r )
	throw logic_error
	  ( "Attempted to add a precomputed premultiplication to the "
	    "multiplication table that does not commute with the part ends" );
  }

  {
    for ( size_t i(0); i < cols.size(); ++i )
      if ( cols[i].second == pre_mult && cols[i].first == r )
	return pre_col_t( i, this );
  }

  // Build up a map in O(n) to get O(ln n) lookups.
  // The alternative -- doing direct lookups -- is
  // O(n ln n).  For a 1-part table on 8 bells, this
  // improves speed a factor of over 100.
  map< row, row_t > finder;
  {
    for ( size_t i(0); i < table.size(); ++i )
      finder[ rows[i] ] = row_t::from_index(i);
  }

  for ( size_t i(0); i < table.size(); ++i )
    table[i].push_back( finder[ make_representative( r * rows[i] ) ] );
  
  cols.push_back( make_pair( r, pre_mult ) );
  return pre_col_t( cols.size() - 1, this );
}

multtab::post_col_t 
multtab::compute_post_mult( const row &r )
{
  {
    for ( size_t i(0); i < cols.size(); ++i )
      if ( cols[i].second == post_mult && cols[i].first == r )
	return post_col_t( i, this );
  }

  // Build up a map in O(n) to get O(ln n) lookups.
  // The alternative -- doing direct lookups -- is
  // O(n ln n).  For a 1-part table on 8 bells, this
  // improves speed a factor of over 100.
  map< row, row_t > finder;
  {
    for ( size_t i(0); i < table.size(); ++i )
      finder[ rows[i] ] = row_t::from_index(i);
  }

  for ( size_t i(0); i < table.size(); ++i )
    table[i].push_back( finder[ make_representative( rows[i] * r ) ] );

  cols.push_back( make_pair( r, post_mult ) );
  return post_col_t( cols.size() - 1, this );
}

multtab::row_t 
multtab::find( const row &r ) const
{
  row r2( make_representative( r ) );

  for ( row_t i; i.n != rows.size(); ++i.n )
    if ( rows[i.n] == r2 ) 
      return i;

  assert( false );
  return row_t();
}

row multtab::find( const multtab::row_t &r ) const
{
  return rows[r.n];
}

RINGING_END_NAMESPACE
