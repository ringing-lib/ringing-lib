// -*- C++ -*- testbase.cc - test the base classes
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

#include <iostream.h>
#include <ringing/method.h>
#include <ringing/mslib.h>

using namespace ringing;

libtype* library::libtypes[] = { &mslib::type };

int print_row_block(const row_block& b)
{
  cout << "\t" << b[0] << " " 
       << ((b[0].sign() < 0) ? '-' : '+') << " " << endl;
  for(int i = 1;i < b.size();i++)
    cout << b.get_changes()[i-1] << "\t" << b[i]  << " " 
	 << ((b[i].sign() < 0) ? '-' : '+') << " " << endl;
}

int main()
{
  {
    cout << "Testing row constructor & printing...\n";
    row r("13572468");
    cout << "This should be Queen's: " << r << endl;
    r = "15263748";
    cout << "This should be Tittums: " << r << endl;
  }

  {
    cout << "\nTesting change constructor, operators & printing...\n";
    row r(8);
    r.rounds();
    change c(8,"18");
    cout << r << " * " << c << " = " << (r * c) << endl;
    c.set(8,"1278");
    cout << r << " * " << c << " = " << (r * c) << endl;
    c.set(8,"123678");
    cout << r << " * " << c << " = " << (r * c) << endl;
    c.set(8,"12");
    cout << c << " reversed is " << c.reverse() << endl;
  }

  {
    cout << "\nTesting row operations...\n";
    row r1("13572468"), r2("21436587"), r3("13245678");
    cout << r1 << " * " << r2 << " = " << (r1 * r2) << endl;
    cout << (r1 * r2) << " / " << r2 << " = " << ((r1 * r2) / r2) << endl;
    cout << r1 << "^-1 = " << r1.inverse() << endl;
    cout << r3 << "^-1 = " << r3.inverse() << endl;
    char s[20];
    cout << r1 << " contains the cycles " << r1.cycles(s) << endl;
    cout << r1 << " has order " << r1.order() << endl;
    cout << r2 << " contains the cycles " << r2.cycles(s) << endl;
    cout << r2 << " has order " << r2.order() << endl;
    cout << r3 << " contains the cycles " << r3.cycles(s) << endl;
    cout << r3 << " has order " << r3.order() << endl;
    cout << "Plain Bob lead head = " << row::pblh(8) << endl;
    cout << "Grandsire lead head = " << row::pblh(8,2) << endl;
  }

  {
    int i;

    cout << "\nTesting method operations...\n";
    method m("-5-4.5-5.36.4-4.5-4-1-4-5.4-4.36.5-5.4-5-1",8,"Bristol");

    cout << "This should be a lead of Bristol Major...\n";
    print_row_block((row_block)m);

    char s[40];
    cout << "Full name: " << m.fullname(s) << endl;
    cout << "Lead head code: " << m.lhcode() << endl;
  }

  {
    cout << "\nTesting method libraries...\n";

    char filename[40], methname[40];

    cout << "File name: ";
    cin >> filename;
    cout << "Method name: ";
    cin >> methname;
    cout << endl;

    library *l = library::open(filename);
    if(l == NULL)
      cout << "Couldn't open file.\n";
    else {
      method *m = l->load(methname);
      if(m == NULL)
	cout << "Couldn't open method.\n";
      else {

	char s[80];
	cout << m->fullname(s) << endl;
	
	print_row_block((row_block)*m);

	cout << "This method is "
	     << ((m->isregular()) ? "" : "ir")
	     << "regular"
	     << ((m->isdouble()) ? " and double" : "")
	     << ".\n";
	cout << "Lead head code: " << m->lhcode() << endl;
      }
    }
  }
    
  return 0;
}
 
