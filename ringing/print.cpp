// print.cpp - Printing stuff
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include <ringing/print.h>

RINGING_START_NAMESPACE

void printrow::options::defaults()
{
  flags = numbers;
  style.size = 10;
  style.font = "Helvetica";
  style.col.grey = true; style.col.red = 0;
  xspace.n = 12; xspace.d = 1; xspace.u = dimension::points; 
  yspace.n = 12; yspace.d = 1; yspace.u = dimension::points;
  line_style s; s.width.n = 1; s.width.d = 2; s.width.u = dimension::points;
  s.col.grey = false; s.col.red = 0; s.col.green = 0; s.col.blue = 1.0;
  s.crossing = false;
  lines[1] = s;
}

printrow::printrow(printpage& pp)
{ 
  options o; o.defaults();
  pr = pp.new_printrow(o); 
}

printrow::printrow(printpage& pp, const options& o)
{ 
  pr = pp.new_printrow(o); 
}

RINGING_END_NAMESPACE
