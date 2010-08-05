// -*- C++ -*- row_calc.h - classes to implement a simple row calculator
// Copyright (C) 2009, 2010 Richard Smith <richard@ex-parrot.com>

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


#ifndef METHSEARCH_ROW_CALC_INCLUDED
#define METHSEARCH_ROW_CALC_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <string>
#if RINGING_OLD_INCLUDES
#include <iterator.h>
#else
#include <iterator>
#endif
#include <ringing/row.h>
#include <ringing/pointers.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

class row_calc
{
public:
  row_calc( int b, string const& str );

  int bells() const { return b; }

  class const_iterator;
  const_iterator begin() const;
  const_iterator end() const;

  class expr
  {
  public:
    class node {
    public:
      virtual ~node() {}
      virtual row evaluate() = 0;
      virtual void restart() = 0;
      virtual int count_vectors() const = 0;
      virtual bool reads_stdin() const = 0;

    protected:
      node() {}
    private:
      node( node const& );
      node& operator=( node const& );
    };
  
    explicit expr( node* n = NULL ) : n(n) {}
    row evaluate() { return n->evaluate(); }
    void restart() { n->restart(); }
    int count_vectors() const { return n->count_vectors(); }
    bool reads_stdin() const { return n->reads_stdin(); }

  private:
    shared_pointer<node> n;
  };

private:
  int b;
  expr e;  
  int v;
};


class row_calc::const_iterator
{
public:
  typedef input_iterator_tag iterator_category;
  typedef row                value_type;
  typedef const row&         reference;
  typedef const row*         pointer;
  typedef ptrdiff_t          difference_type;

  const value_type& operator* () const { return  val; }
  const value_type* operator->() const { return &val; }

  const_iterator() : rc(0) {}

  const_iterator& operator++()
    { increment(); return *this; }

  const_iterator operator++(int)
    { const_iterator tmp(*this); ++*this; return tmp; }

  friend bool operator==( const const_iterator& x, const const_iterator& y )
    { return x.rc == y.rc; }

  friend bool operator!=( const const_iterator& x, const const_iterator& y )
    { return x.rc != y.rc; }

private:
  void increment();

  friend class row_calc;
  explicit const_iterator( row_calc* rc ) : rc(rc) { increment(); }

  row_calc* rc;
  value_type val;
};

inline row_calc::const_iterator row_calc::begin() const
{
  return const_iterator(const_cast<row_calc*>(this));
}

inline row_calc::const_iterator row_calc::end() const
{
  return const_iterator();
}


#endif // METHSEARCH_ROW_CALC_INCLUDED

