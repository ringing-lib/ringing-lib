// -*- C++ -*- testprint.cpp - test the printing classes
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
#include <ringing/print.h>
#include <ringing/print_ps.h>

using namespace ringing;

int main() {
  method m("&-5-4.5-5.36.4-4.5-4-1,1",8,"Bristol");

  printpage_ps pp(cout);
  {
    printrow pr(pp);

    printrow::options o = pr.get_options();
    {
      printrow::options::line_style s;
      s.width = dimension(0);
      o.lines[-1] = s;
    }
    o.flags |= printrow::options::miss_numbers;
    pr.set_options(o);
    pr.set_position(24,11*72);
    
    row_block b(m);
    for(int i = 0; i < 7; i++) {
      pr << b[0]; pr.dot(1); pr.placebell(1); pr.rule();
      for(int j = 1; j < b.size(); j++) {
	if(i == 0 && (j <= (b.size()-1)/2 || j == (b.size()-1)))
	  pr.text(m[j-1].print(), 12, text_style::right, true, false);
	pr << b[j];
	if((j & 3) == 3) pr.rule();
      }
      pr.new_column(140);
      b[0] = b[b.size() - 1]; b.recalculate();
    }
  }
}
