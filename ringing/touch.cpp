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
#include <ringing/touch.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

touch::iterator::iterator(const touch_node& root)
{
  trail.push(place(root));
  ch = current_node().changes.begin();
  while(!trail.empty() && ch == current_node().changes.end())
    next_node();
}

void touch::iterator::next_node()
{
  // Move to the next node
  if(!current_node().children.empty()) {
    // Move down
    trail.push(place(current_node()));
    ch = current_node().changes.begin();
  } else {
    for(;;) {
      // Repeat this node
      ++(trail.top().count);
      if(trail.top().count < (*(trail.top().curr)).first) {
	ch = current_node().changes.begin(); break;
      } else {
	// Move across
	trail.top().count = 0;
	++(trail.top().curr);
	if(trail.top().curr != trail.top().end) {
	  ch = current_node().changes.begin(); break;
	} else {
	  // Move up
	  trail.pop();
	  if(trail.empty()) break;
	}
      }
    }
  }
}

touch::iterator& touch::iterator::operator++()
{
  if(!trail.empty()) ++ch;
  while(!trail.empty() && ch == current_node().changes.end())
    next_node();
  return *this;
} 

RINGING_END_NAMESPACE
