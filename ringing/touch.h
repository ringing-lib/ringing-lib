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

RINGING_START_NAMESPACE

RINGING_USING_STD

template<class T>
class cloning_pointer {
private:
  T* p;
public:
  cloning_pointer() : p(0) {}
  cloning_pointer(T* q) : p(q) {}
  cloning_pointer(const cloning_pointer<T>& cp) { p = (cp.p)->clone(); }
  ~cloning_pointer() { delete p; }
  cloning_pointer& operator=(const cloning_pointer<T>& cp) {
    if(p) delete p;
    p = (cp.p)->clone();
    return *this;
  }
  T& operator*() { return *p; }
  const T& operator*() const { return *p; }
  operator bool() const { return p; }
  bool operator!() const { return !p; }
};

class touch_node {
public:
  class iterator_base {
  public:
    virtual change operator*() = 0;
    virtual const change operator*() const = 0;
    virtual iterator_base& operator++() = 0;
    virtual bool operator==(const iterator_base& i) const = 0;
    virtual ~iterator_base() {}
    virtual iterator_base* clone() = 0; 
  };

  class iterator {
  private:
    cloning_pointer<iterator_base> bp;
  public:
    // Default copy constructor and copy assignment should work
    iterator(iterator_base* b) : bp(b) {}
    iterator() : bp(0) {}
    change operator*() { return **bp; }
    const change operator*() const { return **bp; }
    iterator& operator++() { if(bp) ++*bp; return *this; }
    bool operator==(const iterator& i) const 
      { return (!bp && !(i.bp)) || (*bp == *(i.bp)); }
    bool operator!=(const iterator& i) const
      { return !operator==(i); }
  };

  virtual iterator begin() = 0;
  virtual iterator end() = 0;
  virtual ~touch_node() {}
};

class touch_changes : public touch_node {
private:
  vector<change> c;

public:

  class iterator : public touch_node::iterator_base {
  private:
    vector<change>::const_iterator i;
  public:
    iterator() {}
    ~iterator() {}
    change operator*() { return *i; }
    const change operator*() const { return *i; }
    touch_node::iterator_base& operator++() { ++i; return *this; }
    bool operator==(const touch_node::iterator_base& ib) const {
      const iterator* j = dynamic_cast<const iterator*>(&ib);
      return (j && (i == j->i));
    }
    touch_node::iterator_base* clone() { return new iterator(*this); }
  private:
    friend class touch_changes;
    iterator(vector<change>::const_iterator j) : i(j) {}
  };

  touch_changes() {}
  touch_changes(const vector<change>& cc) : c(cc) {}
  template <class InputIterator> 
  touch_changes(InputIterator a, InputIterator b) : c(a,b) {}
  touch_changes(const string& pn, int b) {
    interpret_pn(b, pn.begin(), pn.end(),
		 back_insert_iterator<vector<change> >(c));
  }
  ~touch_changes() {}

  touch_node::iterator begin()
    { return touch_node::iterator(new iterator(c.begin())); }
  touch_node::iterator end() 
    { return touch_node::iterator(new iterator(c.end())); }
};

class touch_child_list : public touch_node {
public:
  typedef pair<int, touch_node*> entry;

private:
  list<entry> ch;

public:
  class iterator : public touch_node::iterator_base {
  private:
    list<entry>::const_iterator i, last;
    touch_node::iterator ci;
    int count;
  public:
    iterator() {}
    ~iterator() {}
    change operator*() { return *ci; }
    const change operator*() const { return *ci; }
    touch_node::iterator_base& operator++();
    bool operator==(const touch_node::iterator_base& ib) const {
      const iterator* j = dynamic_cast<const iterator*>(&ib);
      return (j && (i == j->i && (i == last || ci == j->ci)));
    }
    touch_node::iterator_base* clone() { return new iterator(*this); }
  private:
    friend class touch_child_list;
    iterator(list<entry>::const_iterator j, 
	     list<entry>::const_iterator k) : i(j), last(k)
      { if(i != last) { count = 0; ci = (*i).second->begin(); } }
  };

  touch_child_list() {}
  ~touch_child_list() {}

  touch_node::iterator begin() {
    return touch_node::iterator(new iterator(ch.begin(), ch.end()));
  }
  touch_node::iterator end() {
    return touch_node::iterator(new iterator(ch.end(), ch.end())); 
  }

  list<entry>& children() { return ch; }
  void push_back(int i, touch_node* tn) {
    ch.push_back(pair<int, touch_node*>(i, tn));
  }
};

RINGING_END_NAMESPACE

#endif
