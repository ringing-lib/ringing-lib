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
#else
#include <iostream>
#include <iomanip>
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


void multtab::swap( multtab &other )
{
  partend.swap( other.partend );
  rows.swap( other.rows );
  table.swap( other.table );
  RINGING_PREFIX_STD swap( colcount, other.colcount );
}

multtab &multtab::operator=( const multtab &other ) 
{ 
  multtab(other).swap(*this);
  return *this;
}

bool multtab::is_representative( const row &r )
{
  for ( row x( partend * r ); x != r; x = partend * x )
    if ( x < r )
      return false;

  return true;
}

row multtab::make_representative( const row &r )
{
  row res(r);

  for ( row x( partend * r ); x != r; x = partend * x )
    if ( x < res )
      res = x;

  return res;
}

void multtab::init( const vector< row > &r )
{
  rows.reserve( r.size() / partend.order() );

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
}

multtab::pre_col_t 
multtab::compute_pre_mult( const row &r )
{
  // This only makes sense if r commutes with the partend
  if ( r * partend != partend * r )
    throw logic_error
      ( "Attempted to add a precomputed premultiplication to the "
	"multiplication table that does not commute with the part end." );

  size_t i(0);
  for ( vector< row >::const_iterator ri( rows.begin() );
	ri != rows.end(); ++ri )
    {
      table[ i++ ].push_back( find( r * *ri ) );
    }
  
  return pre_col_t( colcount++, this );
}

multtab::post_col_t 
multtab::compute_post_mult( const row &r )
{
  size_t i(0);
  for ( vector< row >::const_iterator ri( rows.begin() ); 
	ri != rows.end(); ++ri )
    {
      table[ i++ ].push_back( find( *ri * r ) );
    }
  
  return post_col_t( colcount++, this );
}

multtab::row_t 
multtab::find( const row &r )
{
  row r2( make_representative( r ) );

  for ( row_t i; i.n != rows.size(); ++i.n )
    if ( rows[i.n] == r2 ) 
      return i;

  assert( false );
  return row_t();
}

RINGING_END_NAMESPACE
