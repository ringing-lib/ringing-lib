// -*- C++ -*- testproof.cpp - test the proof class
// Copyright (C) 2002 Mark Banner <mark@standard8.co.uk>

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
#else
#include <iostream>
#include <stdexcept>
#endif
#include <ringing/proof.h>
#include <ringing/touch.h>
#include <string>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

int main()
{
  vector<row> row_block;
  vector<row> row_block2;
  bool quicktest = false;
  {
    cout << "\nTesting True block - result should be true\n";
    row r("13572468");
    row_block.push_back(r);
    r = "15263748";
    row_block.push_back(r);
    r = "12345678";
    row_block.push_back(r);
    r = "75312468";
    row_block.push_back(r);

    proof<std::vector<row>::iterator> p(row_block.begin(), row_block.end(), quicktest);

    cout << "Result: " << p << endl;
  }
  row_block2 = row_block;
  {
    cout << "Testing false block - lines 2 and 6 match\n";
    row r("13456782");
    row_block.push_back(r);
    r = "15263748";
    row_block.push_back(r);
    
    proof<std::vector<row>::iterator> p(row_block.begin(), row_block.end(), quicktest);

    cout << "Result: " << p << endl;

    cout << "Reuse proof object with true block\n";
    p.prove(row_block2.begin(), row_block2.end(), quicktest);

    cout << "Result: " << p << endl;
  }

  {
    cout << "Testing true block plus unknown block\n";

    cout << "Next result should be true.\n";
    row_block.erase(row_block.begin(), row_block.end());

    row r("13456782");
    row_block.push_back(r);
    r = "14567823";
    row_block.push_back(r);
    
    proof<std::vector<row>::iterator> p(row_block2.begin(),
					row_block2.end(),
					row_block.begin(),
					row_block.end(),
					quicktest);

    cout << "Result: " << p << endl;
    
    cout << "Next result should be false - lines 2 and 7 match.\n";

    r = "15263748";
    row_block.push_back(r);

    p.prove(row_block2.begin(),
	    row_block2.end(),
	    row_block.begin(),
	    row_block.end(),
	    quicktest);

    cout << "Result: " << p << endl;
  }
  {
    cout << "Testing more than one extent\n";

    // First construct a 720 of Plain Bob Minor
    touch_changes lead("-1-1-1-1-1-", 6);
    touch_changes lhplain("2", 6);
    touch_changes lhbob("4", 6);
    touch_changes lhsingle("1234", 6);
    
    touch_child_list p, b, s, w, wh, whw, whwsh, touch;
    
    p.push_back(1, &lead); p.push_back(1, &lhplain);
    b.push_back(1, &lead); b.push_back(1, &lhbob);
    s.push_back(1, &lead); s.push_back(1, &lhsingle);
    w.push_back(1, &b); w.push_back(4, &p);
    wh.push_back(1, &b); wh.push_back(3, &p); wh.push_back(1, &b);
    whw.push_back(1, &wh); whw.push_back(1, &w);
    touch.push_back(2, &whw);
    whwsh.push_back(1, &wh); whwsh.push_back(1, &b);
    whwsh.push_back(3, &p); whwsh.push_back(1, &s);
    touch.push_back(1, &whwsh);

    touch.push_back(2, &whw);
    touch.push_back(1, &whwsh);
    
    try {
      touch_node::iterator i;
      vector<row> pbminor;
      row r(6); r.rounds();
      // Don't put the first row on, as otherwise we have rounds twice
      for(i = touch.begin(); i != touch.end(); ++i) {
	r *= *i;
	pbminor.push_back(r);
      }

      proof<std::vector<row>::const_iterator> p(pbminor.begin(), pbminor.end(), quicktest);
      if (!p)
	{
	  cerr << "Standard 720 of Bob Minor is false - is touch right?\n";
	}

      cout << "\nFirst the partial proof (should be true):\n";

      vector<row> pbminor2 = pbminor;
      p.prove(pbminor.begin(), pbminor.end(), pbminor2.begin(), pbminor2.end(), 2, quicktest);

      cout << p << endl;

      cout << "Full 2 extents (should be true):\n";
      // put the blocks together
      vector<row>::iterator j;
      for (j = pbminor2.begin(); j != pbminor2.end(); j++)
	{
	  pbminor.push_back(*j);
	}

      p.prove(pbminor.begin(), pbminor.end(), 2, quicktest);

      cout << p << endl;

      cout << "Put in a false row and retest (full - should be false)\n";
      r.rounds();
      pbminor[360] = r;

      p.prove(pbminor.begin(), pbminor.end(), 2, quicktest);

      cout << p << endl;

      cout << "Retest partial function (result should be false)\n";

      // pbminor2 is still true, three extents this time
      p.prove(pbminor2.begin(), pbminor2.end(), pbminor.begin(), pbminor.end(), 3, quicktest);

      cout << p << endl;
    }
    catch(exception& e) {
      cout << "Exception caught: " << e.what() << endl;
    }

  }
  return 0;
}
 
