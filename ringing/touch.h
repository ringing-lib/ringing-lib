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
#include <stack.h>
#else
#include <vector>
#include <list>
#include <iterator>
#include <stack>
#endif

#include <ringing/row.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class touch;

class touch_node {
public:
  typedef touch_node *pointer;
  typedef list<pair<int, touch_node *> > child_list;

  vector<change> changes;
  child_list children;

  virtual ~touch_node() {}

  // protected:
  friend class touch;
  touch_node() {}
  touch_node(const touch_node& n) :
    changes(n.changes), children(n.children) {}

public:
  void push_back(pointer p, int i = 1) {
    children.push_back(pair<int, touch_node *>(i, p));
  }

#if RINGING_PREMATURE_MEMBER_INSTANTIATION
  bool operator<(const touch_node &) const;
  bool operator==(const touch_node &) const;
  bool operator!=(const touch_node &) const;
  bool operator>(const touch_node &) const;
#endif
};

struct touch_iterator_place {
  int count;
  touch_node::child_list::const_iterator curr, end;
  touch_iterator_place(const touch_node& n) : count(0) {
    curr = n.children.begin(); end = n.children.end();
  } 
  bool operator==(const touch_iterator_place& p) const {
    return count == p.count && curr == p.curr;
  }
  bool operator!=(const touch_iterator_place& p) const {
    return !operator==(p);
  }

#if RINGING_PREMATURE_MEMBER_INSTANTIATION
  touch_iterator_place();
  bool operator<(const touch_iterator_place &) const;
  bool operator>(const touch_iterator_place &) const;
#endif
};

class touch {
protected:
  list<touch_node> impl;

public:
  typedef touch_node *pointer;
  typedef const touch_node *const_pointer;
  pointer bad_pointer() { return &*impl.end(); }

  // Construct an empty touch with only a root node
  touch() : impl(1) {}

protected:
  // Construct an empty touch with the given node as root
  touch(const touch_node& n) { impl.push_back(n); }

public:
  virtual ~touch() {}

  // Return a pointer to the root node
  const_pointer root() const { return &*impl.begin(); }
  pointer root() { return &*impl.begin(); }

  // Create a new empty node
  virtual pointer new_node() { 
    return &*impl.insert(impl.end(), touch_node()); 
  }
  virtual pointer new_node(int b, const string& s) {
    pointer i = &*impl.insert(impl.end(), touch_node());
    interpret_pn(b, s.begin(), s.end(), 
		 back_insert_iterator<vector<change> >((*i).changes));
    return i;
  }

  // Remove all nodes which you can't get to from the root
  void prune();

  class iterator {
  public:
    typedef change value_type;
    typedef ptrdiff_t difference_type;
    typedef forward_iterator_tag iterator_category;
    typedef const change &reference;
    typedef const change *pointer;

  private:
    typedef touch_iterator_place place;

    stack<place> trail;
    vector<change>::const_iterator ch;
    touch_node dummy_node;

    touch_node& current_node() {
      return *((*(trail.top().curr)).second);
    }

    void next_node();

  public:
    iterator() {}
    iterator(const touch& t);
    iterator& operator++();
    iterator operator++(int) {
      iterator i = *this; ++(*this); return i;
    }
    const change& operator*() { return *ch; }
    bool operator==(const iterator& i) const {
      return (trail.empty() && i.trail.empty()) 
	  || (trail == i.trail && ch == i.ch); 
    }
    bool operator!=(const iterator& i) const {
      return !operator==(i); 
    }
  };

  iterator begin() { return iterator(*this); }
  iterator end() { return iterator(); }

};

RINGING_END_NAMESPACE

#endif
