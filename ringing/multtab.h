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

class multiplication_table;

// These would all be member classes of multiplication_table,
// but that causes compiler problems in MSVC-5.
class multiplication_table_row_t
{
public:
  multiplication_table_row_t() : n(0u) {}
  bool isrounds() const { return n == 0; }
  operator size_t() const { return n; }
  
  friend class multiplication_table;
  
#if RINGING_PREMATURE_MEMBER_INSTANTIATION
  // These are to make templates work and don't really exist
  bool operator< (const multiplication_table_row_t &) const;
  bool operator==(const multiplication_table_row_t &) const;
  bool operator!=(const multiplication_table_row_t &) const;
  bool operator> (const multiplication_table_row_t &) const;
#endif
  
private:
  size_t n; 
};

class multiplication_table_post_col_t
{
public:
  multiplication_table_post_col_t( size_t n, multiplication_table *t )
    : n(n), t(t) 
  {}
  
  friend multiplication_table_row_t 
  operator*( multiplication_table_row_t r, multiplication_table_post_col_t c );

#if RINGING_PREMATURE_MEMBER_INSTANTIATION
  // These are to make templates work and don't really exist
  multiplication_table_post_col_t();
  bool operator< (const multiplication_table_post_col_t &) const;
  bool operator==(const multiplication_table_post_col_t &) const;
  bool operator!=(const multiplication_table_post_col_t &) const;
  bool operator> (const multiplication_table_post_col_t &) const;
#endif
  
private:
  size_t n; 
  multiplication_table *t;
};

class multiplication_table_pre_col_t
{
public:
  multiplication_table_pre_col_t( size_t n, multiplication_table *t ) 
    : n(n), t(t) 
  {}
  
  friend multiplication_table_row_t 
  operator*( multiplication_table_pre_col_t c, multiplication_table_row_t r );
  
#if RINGING_PREMATURE_MEMBER_INSTANTIATION
  // These are to make templates work and don't really exist
  multiplication_table_pre_col_t();
  bool operator< (const multiplication_table_pre_col_t &) const;
  bool operator==(const multiplication_table_pre_col_t &) const;
  bool operator!=(const multiplication_table_pre_col_t &) const;
  bool operator> (const multiplication_table_pre_col_t &) const;
#endif
  
private:
  size_t n; 
  multiplication_table *t;
};


// The main class
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
    copy( first, last, back_inserter( rows ) );
    table.resize( rows.size() );
  }

  typedef multiplication_table_row_t      row_t;
  typedef multiplication_table_post_col_t post_col_t;
  typedef multiplication_table_pre_col_t  pre_col_t;

  // Primarily for debugging.  Prints out the multiplication table
  void dump( ostream &os ) const;
  
  // Precompute the products of the set with the given row.
  pre_col_t compute_pre_mult( const row &r );   // r * x  for all x
  post_col_t compute_post_mult( const row &r ); // x * r  for all x
  
  // Convert a row into a precomputed table offset.
  row_t find( const row &r );

  // The number of rows in the table
  size_t size() const { return table.size(); }

  // Operators to do optimised multiplication of rows:
  friend row_t operator*( row_t r, post_col_t c )
    { return c.t->table[ r ][ c.n ]; }
  friend row_t operator*( pre_col_t c, row_t r )
    { return c.t->table[ r ][ c.n ]; }

private:
  vector< row > rows;
  vector< vector< row_t > > table;
  size_t colcount;
};

RINGING_END_NAMESPACE

#endif // RINGING_MULTTAB_H
