// -*- C++ -*- testbase.cpp - test the base classes
// Copyright (C) 2001 Martin Bright <martin@boojum.org.uk>

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

#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <stdexcept.h>
#include <iterator.h>
#else
#include <iostream>
#include <iterator>
#include <stdexcept>
#endif
#include <ringing/method.h>
#include <ringing/extent.h>
#include <ringing/place_notation.h>
#include <string>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

void print_row_block(const row_block& b)
{
  cout << "\t" << b[0] << " " 
       << ((b[0].sign() < 0) ? '-' : '+') << " " << endl;
  for(unsigned int i = 1;i < b.size();i++)
    cout << b.get_changes()[i-1] << "\t" << b[i]  << " " 
	 << ((b[i].sign() < 0) ? '-' : '+') << " " << endl;
}

int main()
{
  {
    cout << "Testing row constructor & printing...\n";
    row r("13572468");
    cout << "This should be Queens: " << r << endl;
    r = "15263748";
    cout << "This should be Tittums: " << r << endl;
  }
  
  {
    cout << "\nTesting parity...\n";
    cout << "(null) has parity: " << row().sign() 
	 << "  (should be +1)" << endl;
    cout << "123456 has parity: " << row("123456").sign() 
	 << "  (should be +1)" << endl;
    cout << "12436758 has parity: " << row("12436758").sign() 
	 << "  (should be -1)" << endl;
  }

  {
    cout << "\nTesting change constructor, operators & printing...\n";
    row r(8);
    r.rounds();
    change c(8,"18");
    cout << r << " * " << c << " = " << (r * c) << endl;
    c.set(8,"1278");
    cout << r << " * " << c << " = " << (r * c) << endl;
    cout << c << " contains " << c.count_places() << " places.\n";
    c.set(8,"123678");
    cout << r << " * " << c << " = " << (r * c) << endl;
    c.set(8,"12");
    cout << c << " reversed is " << c.reverse() << endl;
    cout << "\nTesting place notation expansion...\n";
    string s("&-5-4.5-5.36.4-4.5-4-1,1");
    cout << "Place notation " << s << " becomes:\n";
    interpret_pn(8, s.begin(), s.end(), ostream_iterator<change>(cout, " "));
  }

  {
    cout << "\nTesting row operations...\n";
    row r1("13572468"), r2("21436587"), r3("6347251");
    cout << r1 << " * " << r2 << " = " << (r1 * r2) << endl;
    cout << (r1 * r2) << " / " << r2 << " = " << ((r1 * r2) / r2) << endl;
    cout << r1 << "^-1 = " << r1.inverse() << endl;
    cout << r3 << "^-1 = " << r3.inverse() << endl;
    char s[20];
    cout << r1 << " contains the cycles " << r1.cycles(s) << endl;
    cout << r1 << " has order " << r1.order() << endl;
    cout << r2 << " contains the cycles " << r2.cycles(s) << endl;
    cout << r2 << " has order " << r2.order() << endl;
    cout << r3 << " contains the cycles " << r3.cycles() << endl;
    cout << r3 << " has order " << r3.order() << endl;
    cout << "Plain Bob lead head = " << row::pblh(8) << endl;
    cout << "Grandsire lead head = " << row::pblh(8,2) << endl;
  }

  {
    cout << "\nTesting static row constuctors\n";
    cout << "This should be queens on 8 and then on 7\n";
    cout << row::queens(8) << endl;
    cout << row::queens(7) << endl;
    cout << "\nThis should be kings on 8 and then on 7\n";
    cout << row::kings(8) << endl;
    cout << row::kings(7) << endl;
    cout << "\nThis should be tittums on 8 and then on 7\n";
    cout << row::tittums(8) << endl;
    cout << row::tittums(7) << endl;
    cout << "\nThis should be reverse_rounds on 8 and then on 7\n";
    cout << row::reverse_rounds(8) << endl;
    cout << row::reverse_rounds(7) << endl;
  }

  {
    cout << "\nTesting method operations...\n";
    method m("&-5-4.5-5.36.4-4.5-4-1,1",8,"Bristol");

    cout << "This should be a lead of Bristol Major...\n";
    print_row_block((row_block)m);

    char s[40];
    cout << "Full name: " << m.fullname(s) << endl;
    cout << "Lead head code: " << m.lhcode() << endl;
  }

  cout << "\nThere are "
       << distance( extent_iterator( 5, 1, 8 ), extent_iterator() )
       << " tenors-together lead-ends on eight bells" << endl;
    
  return 0;
}
 
