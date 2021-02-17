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

#include <ringing/print_svg.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

const char* printpage_svg::ns = "http://www.w3.org/2000/svg";

string printpage_svg::convert_col(const colour& c)
{
  ostringstream os;
  if(c.grey) {
    int n = static_cast<int>(c.red * 255);
    os << "rgb(" << n << ',' << n << ',' << n << ')';
  } else
    os << "rgb(" << static_cast<int>(c.red * 255)
      << ',' << static_cast<int>(c.green * 255)
      << ',' << static_cast<int>(c.blue * 255) << ')';
  return os.str();
}

string printpage_svg::convert_dim(const dimension& d)
{
  ostringstream os;
  os << setprecision(2) << fixed << static_cast<float>(d.n) / d.d;
  switch(d.u) {
    case dimension::points:
      os << "pt";
      break;
    case dimension::inches:
      os << "in";
      break;
    case dimension::cm:
      os << "cm";
      break;
    case dimension::mm:
      os << "mm";
      break;
  }
  return os.str();
}

string printpage_svg::format_float(float f)
{
  ostringstream os;
  os << setprecision(2) << fixed << f;
  return os.str();
}

// TODO: do this properly
void printpage_svg::convert_font(const text_style& s, dom_element e)
{
  ostringstream os;
  e.add_attr(ns, "font-size", format_float((s.size)/10.0f).c_str());
  e.add_attr(ns, "font-family", s.font.c_str());
}

void printpage_svg::init(float w, float h)
{
  dom_element root = doc.create_document(ns, "svg");
  root.add_attr(ns, "version", "1.1");
  root.add_attr(ns, "width", format_float(w).c_str());
  root.add_attr(ns, "height", format_float(h).c_str());
  page = root.add_elt(ns, "g");
  // Translate the origin to bottom left
  ostringstream os;
  os << "translate(0," << fixed << setprecision(2) << h << ')';
  page.add_attr(printpage_svg::ns, "transform", os.str().c_str());
}

printpage_svg::printpage_svg(const string& filename, float w, float h)
: doc(filename, dom_document::out, dom_document::file)
{
  init(w, h);
}

printpage_svg::printpage_svg(float w, float h)
: doc("", dom_document::out, dom_document::stdio)
{
  init(w, h);
}

printpage_svg::~printpage_svg()
{
  doc.finalise();
}

void printpage_svg::text(const string t, const dimension& x, const dimension& y,
                    text_style::alignment al, const text_style& s)
{
  dom_element e = page.add_elt(ns, "text");
  e.add_content(t);
  e.add_attr(ns, "x", format_float(x.in_points()).c_str());
  e.add_attr(ns, "y", format_float(-(y.in_points())).c_str());
  if(al != text_style::left)
    e.add_attr(ns, "text-anchor", al == text_style::right ? "end" : "middle");
  convert_font(s, e);
  e.add_attr(ns, "fill", convert_col(s.col).c_str());
}

void printpage_svg::new_page()
{
}

void printrow_svg::set_position(const dimension& x, const dimension& y)
{
  if(in_column) end_column();
  currx = x.in_points();
  curry = -y.in_points();
}

void printrow_svg::move_position(const dimension& x, const dimension& y)
{
  if(in_column) end_column();
  currx += x.in_points();
  curry -= y.in_points();
}

void printrow_svg::start_column()
{
  // Create a group for the column
  col = pp.page.add_elt(printpage_svg::ns, "g");
  // Translate the group to the current position
  ostringstream os;
  os << "translate(" << fixed << setprecision(2) << currx << ',' << curry << ')';
  col.add_attr(printpage_svg::ns, "transform", os.str().c_str());
  // Create a grid element if needed
  if(opt.grid_type > 0)
    grid = col.add_elt(printpage_svg::ns, "path");
  // Create a text element
  numbers = col.add_elt(printpage_svg::ns, "text");
  numbers.add_attr(printpage_svg::ns, "text-anchor", "middle");
  printpage_svg::convert_font(opt.style, numbers);
  numbers.add_attr(printpage_svg::ns, "fill", printpage_svg::convert_col(opt.style.col).c_str());
  // Create a drawline object for each line
  map<bell, printrow::options::line_style>::iterator i;
  for(i = opt.lines.begin(); i != opt.lines.end(); i++)
    drawlines.push_back(drawline_svg(*this, (*i).first, (*i).second));
  count = 0;
  in_column = true;
}

void printrow_svg::end_column()
{
  if(!drawlines.empty()) {
    list<drawline_svg>::iterator i;
    for(i = drawlines.begin(); i != drawlines.end(); i++)
      (*i).output(col);
    drawlines.erase(drawlines.begin(), drawlines.end());
  }
  if(opt.grid_type > 0) do_grid();
  in_column = false;
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
      // Add a y-coordinate
      ts.add_attr(printpage_svg::ns, "y", printpage_svg::format_float(count * opt.yspace.in_points()).c_str());
      // Add the correct vertical alignment
      ts.add_attr(printpage_svg::ns, "alignment-baseline", "central");
    }
  }
    // Add the various bits of lines to the end of the line
  list<drawline_svg>::iterator i;
  for(i = drawlines.begin(); i != drawlines.end(); i++)
    (*i).add(r);

  count++;
  lastrow = r;
}

void printrow_svg::rule()
{
  dom_element e = col.add_elt(printpage_svg::ns, "line");
  e.add_attr(printpage_svg::ns, "x1", printpage_svg::format_float(-opt.xspace.in_points() / 2).c_str());
  e.add_attr(printpage_svg::ns, "x2", printpage_svg::format_float((opt.xspace.in_points() * (2*lastrow.bells()-1))/2).c_str());
  e.add_attr(printpage_svg::ns, "y1", printpage_svg::format_float((count - 0.5) * opt.yspace.in_points()).c_str());
  e.add_attr(printpage_svg::ns, "y2", printpage_svg::format_float((count - 0.5) * opt.yspace.in_points()).c_str());
  e.add_attr(printpage_svg::ns, "stroke", printpage_svg::convert_col(opt.style.col).c_str());
}

void printrow_svg::dot(int i)
{
  if(i == -1) { // Draw dots for all bells that have lines
    map<bell, printrow::options::line_style>::const_iterator j;
    for(j = opt.lines.begin(); j != opt.lines.end(); j++)
      if(!(*j).second.crossing) dot((*j).first);
  } else { // Draw dot for one bell
    int j = 0;
    while(j < lastrow.bells() && lastrow[j] != i) j++; // Find the position of the bell
    if(j < lastrow.bells()) {
      map<bell, printrow::options::line_style>::const_iterator k;
      k = opt.lines.find(i); // Find the style information
      if(k != opt.lines.end()) {
        dom_element e = col.add_elt(printpage_svg::ns, "circle");
        e.add_attr(printpage_svg::ns, "cx", printpage_svg::format_float(opt.xspace.in_points() * j).c_str());
        e.add_attr(printpage_svg::ns, "cy", printpage_svg::format_float(opt.yspace.in_points() * (count-1)).c_str());
        e.add_attr(printpage_svg::ns, "fill", printpage_svg::convert_col((*k).second.col).c_str());
        e.add_attr(printpage_svg::ns, "stroke", "none");
        e.add_attr(printpage_svg::ns, "r", printpage_svg::format_float((*k).second.width.in_points() * 2).c_str());
      }
    }
  }
}

void printrow_svg::placebell(int i)
{
  int j = 0;
  while(j < lastrow.bells() && lastrow[j] != i) j++;
  if(j < lastrow.bells()) {
    // Add a text element for the number
    dom_element e = col.add_elt(printpage_svg::ns, "text");
    { // Translate and scale if necessary
      ostringstream os;
      os << setprecision(2) << fixed;
      os << "translate(" << (lastrow.bells() + 1) * opt.xspace.in_points()
         << ',' << (count - 1) * opt.yspace.in_points() << ')';
      if(j > 9)
        os << " scale(0.8,1)";
      e.add_attr(printpage_svg::ns, "transform", os.str().c_str());
    }
    ostringstream os;
    os << (j+1);
    e.add_content(os.str());
    e.add_attr(printpage_svg::ns, "x", "0");
    e.add_attr(printpage_svg::ns, "y", "0");
    e.add_attr(printpage_svg::ns, "text-anchor", "middle");
    printpage_svg::convert_font(opt.style, e);
    e.add_attr(printpage_svg::ns, "fill", printpage_svg::convert_col(opt.style.col).c_str());
    e.add_attr(printpage_svg::ns, "alignment-baseline", "central");
    // Add a circle
    e = col.add_elt(printpage_svg::ns, "circle");
    e.add_attr(printpage_svg::ns, "cx", printpage_svg::format_float((lastrow.bells() + 1) * opt.xspace.in_points()).c_str());
    e.add_attr(printpage_svg::ns, "cy", printpage_svg::format_float((count - 1) * opt.yspace.in_points()).c_str());
    e.add_attr(printpage_svg::ns, "r", printpage_svg::format_float(opt.style.size * .07f).c_str());
    e.add_attr(printpage_svg::ns, "stroke", printpage_svg::convert_col(opt.style.col).c_str());
    e.add_attr(printpage_svg::ns, "fill", "none");
    e.add_attr(printpage_svg::ns, "stroke-width", printpage_svg::format_float(opt.style.size / 200.0).c_str());
  }
}

void printrow_svg::text(const string& t, const dimension& x,
                        text_style::alignment al, bool between, bool right)
{
  dom_element e = col.add_elt(printpage_svg::ns, "text");
  printpage_svg::convert_font(opt.style, e);
  e.add_attr(printpage_svg::ns, "fill", printpage_svg::convert_col(opt.style.col).c_str());
  if(al != text_style::left)
    e.add_attr(printpage_svg::ns, "text-anchor", al == text_style::right ? "end" : "middle");
  e.add_attr(printpage_svg::ns, "alignment-baseline", "central");
  e.add_attr(printpage_svg::ns, "x",
             printpage_svg::format_float((right ?
                                         (x.in_points() + (lastrow.bells() - 1) * opt.xspace.in_points())
                                          : -x.in_points())).c_str());
  e.add_attr(printpage_svg::ns, "y",
             printpage_svg::format_float(((between ? (count - 0.5f) : (count - 1)) * opt.yspace.in_points())).c_str());
  e.add_content(t);
}

void printrow_svg::do_grid()
{
  int i;
  ostringstream os;
  os << setprecision(2) << fixed;
  switch(opt.grid_type) {
    case 1:
      grid.add_attr(printpage_svg::ns, "stroke", printpage_svg::convert_col(opt.grid_style.col).c_str());
      grid.add_attr(printpage_svg::ns, "stroke-width", printpage_svg::format_float(opt.grid_style.width.in_points()).c_str());
      grid.add_attr(printpage_svg::ns, "fill", "none");
      for(i = 0; i < lastrow.bells(); ++i) {
        os << 'M' << opt.xspace.in_points() * i << ' ' << -opt.yspace.in_points() * 0.5;
        os << 'v' << opt.yspace.in_points() * count;
      }
      break;
    case 2:
      grid.add_attr(printpage_svg::ns, "fill", printpage_svg::convert_col(opt.grid_style.col).c_str());
      grid.add_attr(printpage_svg::ns, "stroke", "none");
      for(i = 0; i < lastrow.bells() - 1; i += 2) {
        os << 'M' << opt.xspace.in_points() * i << ' ' << -opt.yspace.in_points() * 0.5;
        os << 'h' << opt.xspace.in_points();
        os << 'v' << opt.yspace.in_points() * count;
        os << 'h' << -opt.xspace.in_points();
        os << 'Z';
      }
      break;
  }
  grid.add_attr(printpage_svg::ns, "d", os.str().c_str());
}

drawline_svg::drawline_svg(const printrow_svg& pr, bell b,
             printrow::options::line_style st)
  : p(pr), bellno(b), s(st), curr(-1)
{
}

void drawline_svg::add(const row& r)
{
  // Find our bell in the row
  int j;
  for(j = 0; j < r.bells() && r[j] != bellno; j++);
  if(j == r.bells()) j = -1; // Not found
  if(j != -1) {
    ostringstream os;
    os << setprecision(2) << fixed;
    // If the bell was not in the previous row, do a moveto
    if(curr == -1)
      os << 'M' << p.opt.xspace.in_points() * j << ' ' << p.opt.yspace.in_points() * p.count;
    else { // Do a lineto
      os << 'L' << p.opt.xspace.in_points() * j << ' ' << p.opt.yspace.in_points() * p.count;
    }
    data += os.str();
  }
  curr = j;
}

void drawline_svg::output(dom_element parent)
{
  dom_element path = parent.add_elt(printpage_svg::ns, "path");
  path.add_attr(printpage_svg::ns, "d", data.c_str());
  path.add_attr(printpage_svg::ns, "fill", "none");
  path.add_attr(printpage_svg::ns, "stroke", printpage_svg::convert_col(s.col).c_str());
  path.add_attr(printpage_svg::ns, "stroke-width", printpage_svg::format_float(s.width.in_points()).c_str());
}

RINGING_END_NAMESPACE
