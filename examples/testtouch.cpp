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

touch make_touch() 
{
  touch t;

  cout << "Constructing leaves...\n";
  touch_changes *lead, *lhplain, *lhbob;

  t.push_back( lead    = new touch_changes( "&-3-4-2-3-4-5", 6 ) );
  t.push_back( lhplain = new touch_changes( "2", 6 ) );
  t.push_back( lhbob   = new touch_changes( "4", 6 ) );

  cout << "Constructing nodes...\n";
  touch_child_list *p, *b, *whw, *head;

  t.push_back( p    = new touch_child_list );
  t.push_back( b    = new touch_child_list );
  t.push_back( whw  = new touch_child_list );
  t.push_back( head = new touch_child_list );

  cout << "Building tree...\n";
  p->push_back( 1, lead ); p->push_back( 1, lhplain );
  b->push_back( 1, lead ); b->push_back( 1, lhbob );

  whw->push_back( 2, p ); whw->push_back( 1, b );  whw->push_back( 1, p ); 
  whw->push_back( 1, b ); whw->push_back( 2, p );  whw->push_back( 1, b );
  whw->push_back( 2, p );

  cout << "Setting the touch head...\n";
  head->push_back(3, whw);
  t.set_head( head );

  return t;
}

int main()
{
  cout << "Constructing leaves...\n";
  touch_changes lead("-1-1-1-1-1-", 6);
  touch_changes lhplain("2", 6);
  touch_changes lhbob("4", 6);

  cout << "Constructing nodes...\n";
  touch_child_list p, b, wh, t;

  cout << "Building tree...\n";
  p.push_back(1, &lead); p.push_back(1, &lhplain);
  b.push_back(1, &lead); b.push_back(1, &lhbob);
  wh.push_back(1, &b); wh.push_back(3, &p); wh.push_back(1, &b);
  t.push_back(2, &wh);

  // Check that operator-> is working correctly on the touch node iterator.
  cout << "The first change of Plain Bob Minor is " 
       << t.begin()->print() << "\n";

  cout << "This should be 2*WH of Plain Bob Minor:\n";

#if RINGING_USE_EXCEPTIONS
  try 
#endif
  {
    touch_node::const_iterator i;
    row r(6); r.rounds(); cout << r << endl;
    for(i = t.begin(); i != t.end(); ++i) {
      r *= *i;
      cout << r << " " << *i << endl;
    }
  }
#if RINGING_USE_EXCEPTIONS
  catch(exception& e) {
    cout << "Exception caught: " << e.what() << endl;
  }
#endif

  {
    touch t2 = make_touch();
    
    cout << "\n\n"
	 << "3*WHW of Cambridge Surprise Minor is "
	 << distance( t2.begin(), t2.end() )
	 << " changes long." << endl;
  }

  return 0;
}

