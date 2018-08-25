// print_ps.cpp - SVG output
// Copyright (C) 2018 Martin Bright <martin@boojum.org.uk>

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

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <iterator>
#include <sstream>

#include <ringing/print_svg.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

const char* printpage_svg::ns = "http://www.w3.org/2000/svg";

// TODO: handle stdout as well
printpage_svg::printpage_svg(string const& filename)
: doc(filename, dom_document::out, dom_document::file),
root(doc.create_document(ns, "svg"))
{
}

printpage_svg::~printpage_svg()
{
  doc.finalise();
}

void printpage_svg::text(const string t, const dimension& x, const dimension& y,
                    text_style::alignment al, const text_style& s)
{
  dom_element e = root.add_elt(ns, "text");
  e.add_content(t);
  e.add_attr(ns, "x", convert_dim(x).c_str());
  e.add_attr(ns, "y", convert_dim(y).c_str());
  if(al != text_style::left)
    e.add_attr(ns, "text-anchor", al == text_style::right ? "end" : "middle");
  e.add_attr(ns, "font", s.font.c_str());
  e.add_attr(ns, "font-size", to_string(s.size).c_str());
  e.add_attr(ns, "fill", convert_col(s.col).c_str());
}

void printpage_svg::new_page()
{
}

void printrow_svg::set_position(const dimension& x, const dimension& y)
{
  if(in_column) end_column();
  currx = x.in_points();
  curry = y.in_points();
}

void printrow_svg::move_position(const dimension& x, const dimension& y)
{
  if(in_column) end_column();
  currx += x.in_points();
  curry += y.in_points();
}

void printrow_svg::start_column()
{
  // Create a group for the column
  col = pp.root.add_elt(printpage_svg::ns, "g");
  // Translate the group to the current position
  ostringstream os;
  os << "transform(" << fixed << setprecision(2) << currx << ',' << curry << ')';
  col.add_attr(printpage_svg::ns, "translate", os.str().c_str());
  // Create a text element inside it
  numbers = col.add_elt(printpage_svg::ns, "text");
  numbers.add_attr(printpage_svg::ns, "text-anchor", "middle");
  // Create a drawline object for each line
  map<bell, printrow::options::line_style>::iterator i;
  for(i = opt.lines.begin(); i != opt.lines.end(); i++)
    drawlines.push_back(drawline_svg(*this, (*i).first, (*i).second));
  count = 0;
  in_column = true;
}

void printrow_svg::end_column()
{
  
}

void printrow_svg::print(const row& r)
{
  if(!in_column) start_column();
  
  // Print the row
  if(opt.flags & printrow::options::numbers) {
    map<bell, printrow::options::line_style>::const_iterator i;
    string s;
    list<float> pos;
    // Build a list of characters and of positions
    for(int j = 0; j < r.bells(); j++)
      if(!((opt.flags & printrow::options::miss_numbers)
         && ((i = opt.lines.find(r[j])) != opt.lines.end())
           && !((*i).second.crossing))) {
        s += r[j].to_char();
        pos.push_back(j * opt.xspace.in_points());
      }
    if(!s.empty()) {
      // Create a tspan element
      dom_element ts = numbers.add_elt(printpage_svg::ns, "tspan");
      ts.add_content(s);
      {
        // Add a list of x-coordinates
        ostringstream os;
        os << fixed << setprecision(2);
        for(list<float>::const_iterator i = pos.begin(); i != pos.end(); ++i) {
          if(i != pos.begin()) os << ' ';
          os << *i;
        }
        ts.add_attr(printpage_svg::ns, "x", os.str().c_str());
      }
      {
        // Add a y-coordinate
        ostringstream os;
        os << fixed << setprecision(2) << count * opt.yspace.in_points();
        ts.add_attr(printpage_svg::ns, "y", os.str().c_str());
      }
    }
  }
  count++;
  lastrow = r;
  
  // Add the various bits of lines to the end of the line
  list<drawline_svg>::iterator i;
  for(i = drawlines.begin(); i != drawlines.end(); i++)
    (*i).add(r);
}

void printrow_svg::rule()
{
}

void printrow_svg::dot(int i)
{
  
}

void printrow_svg::placebell(int i)
{
  
}

void printrow_svg::text(const string& t, const dimension& x,
                        text_style::alignment al, bool between, bool right)
{
  
}

void printrow_svg::grid()
{
  
}

void drawline_svg::add(const row& r)
{
  
}

RINGING_END_NAMESPACE
