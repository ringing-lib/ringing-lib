// -*- C++ -*- testmusic.cpp - test the base classes
// Copyright (C) 2001 Mark Banner <mark@standard8.co.uk>

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
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/touch.h>
#include <ringing/music.h>
#include <string>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

int main()
{
  // Setup

  // construct a 120 of Plain Bob Doubles
  touch_changes lead("5.1.5.1.5.1.5.1.5", 5);
  touch_changes lhplain("125", 5);
  touch_changes lhbob("145", 5);
  
  touch_child_list p, b, pppb, touch;
  
  p.push_back(1, &lead); p.push_back(1, &lhplain);
  b.push_back(1, &lead); b.push_back(1, &lhbob);
  pppb.push_back(3, &p);
  pppb.push_back(1, &b);
  touch.push_back(3, &pppb);

  touch_node::iterator i;
  vector<row> pbdoubles;
  row r(5); r.rounds();
  // Don't put the first row on, as otherwise we have rounds twice
  for(i = touch.begin(); i != touch.end(); ++i)
    {
      r *= *i;
      pbdoubles.push_back(r);
    }

  {
    int i[2];
    int j = 0;
    cout << "Testing Music Class...\n\n";
    
    cout << "Testing in Plain Bob Doubles (120)\n";

    music mu(5);
    music_details md("???45");
    i[j++] = mu.specify_music(md);
    cout << "0: ???45\n";

    md.Set("54???", 2);
    i[j++] = mu.specify_music(md);
    cout << "1: 54???\n";

    md.Set("12??5", 4);
    i[j++] = mu.specify_music(md);
    cout << "2: 12??5\n";

    md.Set("??345", 6);
    i[j++] = mu.specify_music(md);
    cout << "3: ??345\n";

    md.Set("13524", 6);
    i[j++] = mu.specify_music(md);
    cout << "4: 13524\n";

    mu.process_rows(pbdoubles.begin(), pbdoubles.end());

    cout << "Results for both strokes:\n";

    for (int k = 0; k < j; k++)
      cout << "Total " << k << ": " << mu.Get_Results(i[k]) << " : " << mu.Get_Score(i[k]) << endl;

    cout << "Total Score: " << mu.Get_Score() << endl;

    cout << "Results for handstrokes:\n";

    for (int k = 0; k < j; k++)
      cout << "Total " << k << ": " << mu.Get_Results(i[k], eHandstroke) << " : " << mu.Get_Score(i[k], eHandstroke) << endl;

    cout << "Total Score: " << mu.Get_Score(eHandstroke) << endl;

    cout << "Results for backstrokes:\n";

    for (int k = 0; k < j; k++)
      cout << "Total " << k << ": " << mu.Get_Results(i[k], eBackstroke) << " : " << mu.Get_Score(i[k], eBackstroke) << endl;

    cout << "Total Score: " << mu.Get_Score(eBackstroke) << endl;
  }
  return 0;
}
