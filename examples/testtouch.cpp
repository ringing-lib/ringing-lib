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

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <stdexcept.h>
#else
#include <iostream>
#include <stdexcept>
#endif
#include <ringing/method.h>
#include <ringing/touch.h>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

int main()
{
  cout << "Constructing leaves...\n";
  touch_changes lead("-1-1-1-1-1-", 6);
  touch_changes lhplain("2", 6);
  touch_changes lhbob("4", 6);

  cout << "Constructing nodes...\n";
  touch_child_list p, b, wh, touch;

  cout << "Building tree...\n";
  p.push_back(1, &lead); p.push_back(1, &lhplain);
  b.push_back(1, &lead); b.push_back(1, &lhbob);
  wh.push_back(1, &b); wh.push_back(3, &p); wh.push_back(1, &b);
  touch.push_back(2, &wh);

  cout << "This should be 2*WH of Plain Bob Minor:\n";
  try {
    touch_node::iterator i;
    row r(6); r.rounds(); cout << r << endl;
    for(i = touch.begin(); i != touch.end(); ++i) {
      r *= *i;
      cout << r << " " << *i << endl;
    }
  }
  catch(exception& e) {
    cout << "Exception caught: " << e.what() << endl;
  }
  return 0;
}
