// -*- C++ -*- testtouch.cpp - test the touch classes
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

#if 0

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#else
#include <iostream>
#endif
#include <ringing/touch.h>
#include <ringing/proof.h>
#include <ringing/music.h>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

int main()
{
  // We will construct WHWH of Plain Bob Minor
  touch t;

  touch::pointer lead = t.new_node(6, "X1X1X1X1X1X");

  touch::pointer plainlh = t.new_node(6, "12");

  touch::pointer boblh = t.new_node(6, "14");

  touch::pointer plain = t.new_node();
  (*plain).push_back(lead);
  (*plain).push_back(plainlh);

  touch::pointer bob = t.new_node();
  (*bob).push_back(lead);
  (*bob).push_back(boblh);

  touch::pointer wh = t.new_node();
  (*wh).push_back(bob);
  (*wh).push_back(plain, 3);
  (*wh).push_back(bob); 

  (*(t.root())).push_back(wh, 2);

  list<row> l;
  row r(6); r.rounds();
  transform(t.begin(), t.end(), back_insert_iterator<list<row> >(l),
	    permute(r));

  copy(l.begin(), l.end(), ostream_iterator<row>(cout, "\n"));

  cout << endl << "Music: " << music(l.begin(), l.end()) << endl;

  return 0;
}

#else

int main() { return 1; }

#endif

