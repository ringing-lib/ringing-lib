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

class multtab;

RINGING_START_DETAILS_NAMESPACE 

// These would all be member classes of multtab,
// but that causes compiler problems in MSVC-5.
class multtab_row_t
{
public:
  // The number_of_bells parameter is ignored.  It is here
  // to give the constructor of multtab::row_t and row the 
  // same interface.
  explicit multtab_row_t( int number_of_bells = 0 ) : n(0u) {}
  bool isrounds() const { return n == 0; }
  size_t index() const { return n; }
  
  friend class multtab;
  friend class multtab_row_iterator;
  RINGING_FAKE_COMPARATORS( multtab_row_t );

private:
  size_t n; 
};

// Iterate through the rows in the table
class RINGING_API multtab_row_iterator 
  : public RINGING_STD_CONST_ITERATOR( forward_iterator_tag, multtab_row_t )
{
 public:
  // These typedefs are needed to compile get the code to 
  // compile in gcc-2.95.x.
  typedef forward_iterator_tag iterator_category;
  typedef multtab_row_t value_type;
  typedef ptrdiff_t difference_type;
  typedef const multtab_row_t *pointer;
  typedef const multtab_row_t &reference;
  
  const multtab_row_t &operator*() const  { return r; }
  const multtab_row_t *operator->() const { return &r; }
  
  multtab_row_iterator &operator++() { ++r.n; return *this; }
  multtab_row_iterator operator++(int) 
    { multtab_row_iterator tmp(*this); ++*this; return tmp; }  
  bool operator==( const multtab_row_iterator &other ) const
    { return r.index() == other.r.index(); }
  bool operator!=( const multtab_row_iterator &other ) const
    { return r.index() != other.r.index(); }

 private:  
  friend class multtab;
  multtab_row_iterator( int n ) { r.n = n; }

 private:
  multtab_row_t r;
};

class multtab_post_col_t
{
private:
  multtab_post_col_t( size_t n, multtab *t )
    : n(n), t(t) 
  {}
  
  friend multtab_row_t 
  operator*( multtab_row_t r, multtab_post_col_t c );

  friend class multtab;

public:
  RINGING_FAKE_DEFAULT_CONSTRUCTOR( multtab_post_col_t );
  RINGING_FAKE_COMPARATORS( multtab_post_col_t );

private:
  size_t n; 
  multtab *t;
};

class multtab_pre_col_t
{
private:
  multtab_pre_col_t( size_t n, multtab *t ) 
    : n(n), t(t) 
  {}
  
  friend multtab_row_t 
  operator*( multtab_pre_col_t c, multtab_row_t r );

  friend class multtab;

public:
  RINGING_FAKE_DEFAULT_CONSTRUCTOR( multtab_pre_col_t );  
  RINGING_FAKE_COMPARATORS( multtab_pre_col_t );  

private:
  size_t n; 
  multtab *t;
};

RINGING_END_DETAILS_NAMESPACE

// --------------------------------------------------------------
// 
// The main class
class multtab
{
public:
  multtab() {}

  // A swap function and assignment operator
  void swap( multtab &other );
  multtab &operator=( const multtab &other );

  // Initialises a multiplication table with the rows in the 
  // range [first, last).
  template < class InputIterator >
  multtab( InputIterator first, InputIterator last )
    : partend( first->bells() ), colcount( 0u ), 
      rows( make_vector( first, last ) ), table( rows.size() )
  {}

  // As above but use factor out some part-end.
  template < class InputIterator >
  multtab( InputIterator first, InputIterator last, const row &partend )
    : partend( partend ), colcount( 0u )
  { init( make_vector( first, last ) ); }

  typedef RINGING_DETAILS_PREFIX multtab_row_t      row_t;
  typedef RINGING_DETAILS_PREFIX multtab_post_col_t post_col_t;
  typedef RINGING_DETAILS_PREFIX multtab_pre_col_t  pre_col_t;

  // Primarily for debugging.  Prints out the multiplication table
  void dump( ostream &os ) const;
  
  // Precompute the products of the set with the given row.
  pre_col_t compute_pre_mult( const row &x );   // (x * r) for all rows r
  post_col_t compute_post_mult( const row &x ); // (r * x) for all rows r
  
  // Convert a row into a precomputed table offset.
  row_t find( const row &r );

  // The number of rows in the table
  size_t size() const { return table.size(); }

  // Iterate through the rows in the multiplication table
  typedef RINGING_DETAILS_PREFIX multtab_row_iterator row_iterator;
  row_iterator begin_rows() const { return row_iterator(0); }
  row_iterator end_rows() const   { return row_iterator(size()); }

  // Operators to do optimised multiplication of rows:
  friend row_t RINGING_DETAILS_PREFIX operator*( row_t r, post_col_t c )
    { return c.t->table[ r.index() ][ c.n ]; }
  friend row_t RINGING_DETAILS_PREFIX operator*( pre_col_t c, row_t r )
    { return c.t->table[ r.index() ][ c.n ]; }

private:
  // A helper to do what the templated constructor of vector does
  // (we can't use that directly because MSVC doesn't have it).
  template < class InputIterator >
  static vector< row > make_vector( InputIterator first, InputIterator last )
  {
    vector< row > r;
    copy( first, last, back_inserter( r ) );
    return r;
  }

  // Each coset of the group generated by the part end has one member
  // which is used to represent the whole coset. 
  bool is_representative( const row &r );
  row make_representative( const row &r );

  void init( const vector< row > &r );

  row partend;
  size_t colcount;
  vector< row > rows;
  vector< vector< row_t > > table;
};

RINGING_END_NAMESPACE

RINGING_DELEGATE_STD_SWAP( multtab )

#endif // RINGING_MULTTAB_H

