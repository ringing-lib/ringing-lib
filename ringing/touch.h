// -*- C++ -*- touch.h - Classes for touches
// Copyright (C) 2001 Martin Bright <M.Bright@dpmms.cam.ac.uk>

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

#ifndef RINGING_TOUCH_H
#define RINGING_TOUCH_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <list.h>
#include <iterator.h>
#else
#include <vector>
#include <list>
#include <iterator>
#endif

#include <ringing/row.h>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class touch_node {
RINGING_PROTECTED_IMPL:
  class iterator_base {
  public:
    virtual change operator*() const = 0;
    virtual iterator_base& operator++() = 0;
    virtual bool operator==(const iterator_base& i) const = 0;
    virtual ~iterator_base() {}
    virtual iterator_base* clone() = 0; 
  };

public:
  class const_iterator
    : public RINGING_STD_CONST_ITERATOR( forward_iterator_tag, change )
  {
  private:
    cloning_pointer<iterator_base> bp;

    class operator_arrow_proxy 
    {
    public:
      operator_arrow_proxy( const change &c ) : c(c) {}
      const change *operator->() const { return &c; }
      operator const change *() const { return &c; }
    private:
      change c;
    };

  public:
    // These typedefs are needed to compile get the code to 
    // compile in gcc-2.95.x.
    typedef forward_iterator_tag iterator_category;
    typedef change value_type;
    typedef ptrdiff_t difference_type;
    typedef const change *pointer;
    typedef const change &reference;

    // Default copy constructor and copy assignment should work
    const_iterator(iterator_base* b) : bp(b) {}
    const_iterator() : bp(0) {}
    change operator*() const { return **bp; }
    operator_arrow_proxy operator->() const { return **bp; }
    const_iterator& operator++() { if(bp) ++*bp; return *this; }
    const_iterator operator++(int) 
      { const_iterator tmp(*this); ++*this; return tmp; }
    bool operator==(const const_iterator& i) const 
      { return (!bp && !(i.bp)) || (*bp == *(i.bp)); }
    bool operator!=(const const_iterator& i) const
      { return !operator==(i); }

    // I give in.  Why does msvc-6's default assignment operator not work?
    const_iterator &operator=( const const_iterator &o ) 
      { bp = o.bp; return *this; }
  };

  virtual const_iterator begin() const = 0;
  virtual const_iterator end() const = 0;
  virtual ~touch_node() {}
};

class touch_changes : public touch_node {
private:
  vector<change> c;
  class iterator;

public:
  touch_changes() {}
  touch_changes(const vector<change>& cc) : c(cc) {}
  template <class InputIterator> 
  touch_changes(InputIterator a, InputIterator b) : c(a,b) {}
  touch_changes(const string& pn, int b) {
    interpret_pn(b, pn.begin(), pn.end(),
		 back_insert_iterator<vector<change> >(c));
  }
  ~touch_changes() {}

  void push_back( const change &ch ) { c.push_back(ch); }

  virtual touch_node::const_iterator begin() const;
  virtual touch_node::const_iterator end() const;
};

class touch_child_list : public touch_node {
public:
  typedef pair<int, touch_node*> entry;

private:
  list<entry> ch;
  class iterator;

public:
  touch_child_list() {}
 ~touch_child_list() {}

  virtual touch_node::const_iterator begin() const;
  virtual touch_node::const_iterator end() const;

  list<entry>& children() { return ch; }
  const list<entry>& children() const { return ch; }

  void push_back(int i, touch_node* tn) {
    ch.push_back(pair<int, touch_node*>(i, tn));
  }
  void pop_back() { ch.pop_back(); }
};

// A wrapper around a list of touch_nodes to manage their memory
class touch
{
public:
  typedef change value_type;
  typedef touch_node::const_iterator const_iterator;

  const_iterator begin() const 
    { return head ? head->begin() : const_iterator(); }
  const_iterator end() const 
    { return head ? head->end() : const_iterator(); }

  void push_back( touch_node *node );
  void set_head( touch_node *node ) { head = node; }

  touch_node *get_node( size_t idx ) { return (*nodes)[idx]; }
  const touch_node *get_head() const { return head; }
  touch_node *get_head() { return head; }

  touch() : head(0) {}

private:
  class touch_node_list : private vector< touch_node * >
  {
  public:
    using vector< touch_node * >::push_back;
    using vector< touch_node * >::operator[];
   ~touch_node_list();
  };

  touch_node *head;
  shared_pointer< touch_node_list > nodes;
};

RINGING_END_NAMESPACE

#endif
