// -*- C++ -*- touch.cpp - Classes for touches
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <algo.h>
#else 
#include <algorithm>
#endif
#include <ringing/touch.h>

RINGING_START_NAMESPACE


// *********************************************************************
// *                Functions for class touch_changes                  *
// *********************************************************************

class touch_changes::iterator : public touch_node::iterator_base {
private:
  vector<change>::const_iterator i;
public:
  iterator() {}
 ~iterator() {}
  change operator*() const { return *i; }
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

touch_node::const_iterator touch_changes::begin() const
{ return touch_node::const_iterator(new iterator(c.begin())); }

touch_node::const_iterator touch_changes::end() const
{ return touch_node::const_iterator(new iterator(c.end())); }

// *********************************************************************
// *               Functions for class touch_child_list                *
// *********************************************************************

class touch_child_list::iterator : public touch_node::iterator_base {
private:
  list<entry>::const_iterator i, last;
  touch_node::const_iterator ci;
  int count;
public:
  iterator() {}
  ~iterator() {}
  change operator*() const { return *ci; }
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


touch_node::iterator_base& touch_child_list::iterator::operator++()
{
  ++ci;
  if(ci == (*i).second->end()) {
    ++count;
    if(count != (*i).first) {
      ci = (*i).second->begin();
    } else {
      ++i;
      if(i != last) {
	count = 0;
	ci = (*i).second->begin();
      }
    }
  }
  return *this;
}


touch_node::const_iterator touch_child_list::begin() const {
  return touch_node::const_iterator(new iterator(ch.begin(), ch.end()));
}
touch_node::const_iterator touch_child_list::end() const {
  return touch_node::const_iterator(new iterator(ch.end(), ch.end())); 
}

// *********************************************************************
// *                    Functions for class touch                      *
// *********************************************************************

void touch::push_back( touch_node *node )
{
  if ( !nodes.get() ) nodes.reset( new touch_node_list );
  nodes->push_back( node ); 
}

touch::touch_node_list::~touch_node_list()
{
  for_each( begin(), end(), delete_pointers() );
}


RINGING_END_NAMESPACE
