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
#include <algorithm.h>
#else 
#include <algorithm>
#endif
#include <ringing/touch.h>

RINGING_START_NAMESPACE

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
