// -*- C++ -*- multtab.cpp - A precomputed multiplication table of rows
// Copyright (C) 2002 Richard Smith <richard@ex-parrot.com>

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
#include <ringing/multtab.h>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <iomanip.h>
#include <set.h>
#else
#include <iostream>
#include <iomanip>
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

RINGING_START_ANON_NAMESPACE

class group
{
public:
  group( const vector<row> &generators );

  typedef set<row>::const_iterator const_iterator;
  const_iterator begin() const { return s.begin(); }
  const_iterator end()   const { return s.end();   }
  size_t         size()  const { return s.size();  }

private:
  void generate_recursive( const row& r, const vector<row>& generators );

  set<row> s;
};

void group::generate_recursive( const row& r, 
				const vector<row>& generators )
{
  for ( vector<row>::const_iterator 
	  i( generators.begin() ), e( generators.end() );
	i != e;  ++i )
    {
      const row r2( r * *i );
      if ( s.insert( r2 ).second )
	generate_recursive( r2, generators );
    }
}

group::group( const vector<row> &gens )
{
  set< row > group;
  size_t b(0); // Number of bells

  for ( vector<row>::const_iterator i( gens.begin() ), e( gens.end() );
	i != e;  ++i )
    if ( i->bells() > b ) 
      b = i->bells();

  generate_recursive( row(b), gens );
}


RINGING_END_ANON_NAMESPACE

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

bool multtab::is_representative( const row& r ) const
{
  for ( vector<row>::const_iterator i( pends.begin() ), e( pends.end() ); 
	i != e; ++i )
    {
      row x( *i * r );
      if ( x < r )
	return false;
    }

  return true;
}

row multtab::make_representative( const row& r ) const
{
  row res(r);

  for ( vector<row>::const_iterator i( pends.begin() ), e( pends.end() ); 
	i != e; ++i )
    {
      row x( *i * r );
      if ( x < res ) res = x;
    }

  return res;
}

void multtab::init( const vector< row >& r, const vector< row >& gens )
{
  {
    group g( gens );
    copy( g.begin(), g.end(), back_inserter(pends) );
  }

  rows.reserve( r.size() / pends.size() );

  for ( vector<row>::const_iterator i( r.begin() ), e( r.end() ); i != e; ++i )
    if ( is_representative( *i ) )
      rows.push_back( *i );

  table.resize( rows.size() );
}

void multtab::dump( ostream &os ) const
{
  const int width( (int)ceil( log10( (float)table.size() ) ) );
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

  os << "Part ends: { ";
  copy( pends.begin(), pends.end(), ostream_iterator<row>(os, " ") );
  os << "}" << endl;
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

  for ( size_t i(0); i < table.size(); ++i )
    table[i].push_back( find( r * rows[i] ) );
  
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

  for ( size_t i(0); i < table.size(); ++i )
    table[i].push_back( find( rows[i] * r ) );

  cols.push_back( make_pair( r, pre_mult ) );
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
