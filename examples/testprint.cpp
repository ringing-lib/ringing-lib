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

#if RINGING_OLD_INCLUDES
#include <iostream.h>
#else
#include <iostream>
#endif
#include <ringing/method.h>
#include <ringing/print_pdf.h>
#include <ringing/printm.h>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

int main() {
  method m("&-5-4.5-5.36.4-4.5-4-1,1",8,"Bristol");

  printpage_pdf pp(cout);
  printmethod pm(m);

  pm.defaults();
  
  pm.xoffset.n = 1; pm.xoffset.d = 2; pm.xoffset.u = dimension::inches;
  pm.yoffset.n = 11; pm.yoffset.d = 1; pm.yoffset.u = dimension::inches;

  pm.fit_to_space(7*72, 11*72, true, 1.0);

  pm.number_mode = printmethod::miss_lead;
  pm.placebells = 7;

  pm.print(pp);

  return 0;
}
