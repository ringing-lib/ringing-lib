// -*- C++ -*- multtab.h - A precomputed multiplication table of rows
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

#ifndef RINGING_MULTTAB_H
#define RINGING_MULTTAB_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include <ringing/row.h>
#if RINGING_OLD_INCLUDES
#include <iosfwd.h>
#include <vector.h>
#include <algo.h>
#include <iterator.h>
#else
#include <iosfwd>
#include <vector>
#include <algorithm>
#include <iterator>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class multiplication_table
{
public:
  multiplication_table() {}

  template < class InputIterator >
  multiplication_table( InputIterator first, InputIterator last )
    : colcount( 0u )
  {
    // Cannot use the templated constructor because MSVC's STL 
    // doesn't have it.
    copy( first, last, back_inserter( table ) );
  }

  struct post_col_t;
  struct pre_col_t;
  struct row_t;

  struct row_t 
  {
  public:
    row_t() : n(0u) {}
    bool isrounds() const { return n == 0; }
    operator size_t() const { return n; }

    friend class multiplication_table;

  private:
    size_t n; 
  };

  struct post_col_t 
  {
  public:
    post_col_t( size_t n, multiplication_table *t ) : n(n), t(t) {}

    friend row_t operator*( row_t r, post_col_t c )
    {
      return c.t->table[ r ].x[ c.n ]; 
    }

  private:
    size_t n; 
    multiplication_table *t;
  };

  struct pre_col_t
  {
  public:
    pre_col_t( size_t n, multiplication_table *t ) : n(n), t(t) {}

    friend row_t operator*( pre_col_t c, row_t r )
    {
      return c.t->table[ r ].x[ c.n ]; 
    }

  private:
    size_t n; 
    multiplication_table *t;
  };

  // Primarily for debugging.  Prints out the multiplication table
  void dump( ostream &os ) const;
  
  // Precompute the products of the set with the given row.
  pre_col_t compute_pre_mult( const row &r );   // r * x  for all x
  post_col_t compute_post_mult( const row &r ); // x * r  for all x
  
  // Convert a row into a precomputed table offset.
  row_t find( const row &r );

  // The number of rows in the table
  size_t size() const { return table.size(); }

private:

  friend row_t operator*( row_t r, post_col_t c );
  friend row_t operator*( pre_col_t c, row_t r );

  struct table_row
  {
    table_row( const row &r ) : r(r) {}
    
    row r;
    vector< row_t > x;
  };

  vector< table_row > table;
  size_t colcount;
};

RINGING_END_NAMESPACE

#endif // RINGING_MULTTAB_H
