// print_pdf.cpp - PDF printing stuff
// Copyright (C) 2002 Martin Bright <martin@boojum.org.uk>

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

// File structure and object numbers:
//  1 Catalog(ue)
//  2 Document information dictionary
//
//  4 Page 1 content stream
//  5 Page 1 content stream length
//  6 Page 1 dictionary
//  ... and so on
//
//  3 Page tree

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_INCLUDES
#include <iomanip.h>
#else
#include <iomanip>
#endif
#include <cmath>
#include <ringing/streamutils.h>
#include <ringing/print_pdf.h>

RINGING_START_NAMESPACE

pdf_file::pdf_file(ostream& o, bool l, int w, int h)
  : csb(o.rdbuf()), os(&csb), width(w), height(h), landscape(l)
{
  start();
}

void pdf_file::start()
{
  os << "%PDF-1.4\n";
  obj_count = 3;
  pages = 0;
  font_counter = 0;
  output_catalogue();
  output_info();
}

void pdf_file::end()
{
  output_pages();
  int xref_offset = csb.get_count();
  os << "xref\n0 " << obj_count+1 << "\n0000000000 65535 f \n";
  for(int i = 1; i <= obj_count; i++)
    os << setfill('0') << setw(10) << offsets[i] << " 00000 n \n";
  os << "trailer\n  << /Size " << obj_count+1
     << "\n     /Root 1 0 R\n     /Info 2 0 R\n  >>\nstartxref\n"
     << xref_offset << "\n%%EOF\n";
}

int pdf_file::start_object(int n) 
{ 
  if(n == 0) n = ++obj_count;
  offsets[n] = csb.get_count();
  os << n << " 0 obj\n";
  return n;
}

void pdf_file::end_object()
{
  os << "endobj\n\n";
}

void pdf_file::start_stream()
{
  start_object();
  os << "  << /Length " << obj_count+1 << " 0 R >>\nstream\n";
  stream_start = csb.get_count();
  os.setf(ios::fixed);
  os << setprecision(2);
}

void pdf_file::end_stream()
{
  int length = csb.get_count() - stream_start;
  os.unsetf(ios::fixed);
  os << "\nendstream\n";
  end_object();
  start_object();
  os << "  " << length << '\n';
  end_object();
}

void pdf_file::start_page()
{
  ++pages;
  start_stream();
}

void pdf_file::end_page()
{
  end_stream();
  start_object();
  os << "  << /Type /Page\n     /Parent 3 0 R\n     /Contents "
     << obj_count - 2 << " 0 R\n  >>\n";
  end_object();
}

void pdf_file::output_catalogue()
{
  start_object(1);
  os << "  << /Type /Catalog\n     /Pages 3 0 R\n  >>\n";
  end_object();
}

void pdf_file::output_info()
{
  start_object(2);
  os << "  << /Creator (Ringing Class Library \\(" RINGING_PACKAGE " "
    RINGING_VERSION "\\)) >>\n";
  end_object();
}

void pdf_file::output_pages()
{
  start_object(3);
  os << "  << /Type /Pages\n     /Count " << pages << "\n     /Kids [ ";
  {
    for(int i = 0; i < pages; i++)
      os << (6 + i * 3) << " 0 R ";
  }
  os << "]\n     /MediaBox [0 0 " << width << ' ' << height << "]\n";
  if(landscape) os << "     /Rotate 90\n";
  os << "     /Resources << /Procset [/PDF /Text]\n"
        "                   /Font << \n";
  map<string, string>::const_iterator i;
  int j = obj_count;
  for(i = fonts.begin(); i != fonts.end(); ++i)
    os << "                          /" << (*i).second
       << ' ' << ++j << " 0 R\n";
  os << "                         >>\n"
        "                >>\n"
        "  >>\n";
  end_object();

  for(i = fonts.begin(); i != fonts.end(); ++i) {
    start_object();
    os << "  << /Type /Font /Subtype /Type1"
       << " /Name /" << (*i).second 
       << " /BaseFont /" << (*i).first << " >>\n";
    end_object();
  }
}

const string& pdf_file::get_font(const string& f)
{
  map<string, string>::const_iterator i;
  if((i = fonts.find(f)) == fonts.end()) {
    i = fonts.insert(pair<string, string>
		     (f, make_string() << 'F' << ++font_counter)).first;
  }
  return (*i).second;
}

void pdf_file::output_string(const string& s)
{
  string::const_iterator i;
  os << '(' << oct;
  for(i = s.begin(); i != s.end(); ++i) {
    switch(*i) {
      case '\n' : os << "\\n"; break;
      case '\r' : os << "\\r"; break;
      case '\t' : os << "\\t"; break;
      case '\b' : os << "\\b"; break;
      case '\f' : os << "\\f"; break;
      case '(' : os << "\\("; break;
      case ')' : os << "\\)"; break;
      case '\\': os << "\\\\"; break;
      default:
	if(isprint(*i))
	  os << *i;
	else
	  os << '\\' << setw(3) << static_cast<int>(*i);
	break;
    }
  }
  os << dec << ')';
}

printpage_pdf::printpage_pdf(ostream& o, const dimension& w, 
			     const dimension& h, bool l) 
  : f(o, l, static_cast<int>(w.in_points()), static_cast<int>(h.in_points()))
{
  f.start_page();
  if(l) landscape_mode();
}

printpage_pdf::printpage_pdf(ostream& o, bool l) : f(o, l)
{
  f.start_page();
  if(l) landscape_mode();
}

printpage_pdf::~printpage_pdf()
{
  f.end_page();
}

void printpage_pdf::new_page()
{
  f.end_page();
  f.start_page();
  if(f.get_landscape()) landscape_mode();
}

void printpage_pdf::landscape_mode()
{
  f << "0 1 -1 0 " << f.get_width() << " 0 cm\n";
}

void printpage_pdf::set_colour(const colour& c, bool nonstroke)
{
  if(c.grey) 
    f << c.red << ' ' << (nonstroke ? 'g' : 'G') << '\n';
  else
    f << c.red << ' ' << c.green << ' ' << c.blue << ' '
      << (nonstroke ? "rg\n" : "RG\n");
}

void printpage_pdf::circle(float x, float y, float r, char op)
{
  f << x - r << ' ' << y << " m "
    << x - r << ' ' << y + r/2 << ' ' 
    << x - r/2 << ' ' << y + r << ' ' 
    << x << ' ' << y + r << " c "
    << x + r/2 << ' ' << y + r << ' '
    << x + r << ' ' << y + r/2 << ' '
    << x + r << ' ' << y << " c "
    << x + r << ' ' << y - r/2 << ' '
    << x + r/2 << ' ' << y - r << ' '
    << x << ' ' << y - r << " c "
    << x - r/2 << ' ' << y - r << ' '
    << x - r << ' ' << y - r/2 << ' '
    << x - r << ' ' << y << " c "
    << op << '\n';
}

void printpage_pdf::arrow(float x0, float y0, float x1, float y1, 
                          float headsize)
{
  float theta = atan2(y1-y0, x1-x0);
  f << x0 << ' ' << y0 << " m "
    << x1 << ' ' << y1 << " l "
    << x1-headsize/2*cos(theta + 3.1416/6) << ' ' 
    << y1-headsize/2*sin(theta + 3.1416/6) << " m "
    << x1 << ' ' << y1 << " l "
    << x1-headsize/2*cos(theta - 3.1416/6) << ' ' 
    << y1-headsize/2*sin(theta - 3.1416/6) << " l "
    << "S\n"; // "h B\n";
}

void printpage_pdf::text(const string t, const dimension& x, 
			const dimension& y, text_style::alignment al, 
			const text_style& s)
{
  charwidths cw;
  if(!cw.font(s.font)) cw.font("Helvetica");
  float x1 = x.in_points();
  switch(al) {
    case text_style::right :
      x1 -= cw(t) * s.size / 10000.0f;
      break;
    case text_style::centre :
      x1 -= cw(t) * s.size / 20000.0f;
      break;
  default:
    // I.e. left, assume we don't need to do
    // anything here - but put in default to 
    // stop warnings in build.
    break;
  }
  
  f << "q "; set_colour(s.col, true);
  f << "BT\n/"
    << f.get_font(cw.font()) << ' ' << (s.size / 10.0) << " Tf\n"
    << x1 << ' ' << y.in_points() << " Td\n";
  f.output_string(t);
  f << " Tj\n"
    << "ET Q\n";
}

// This function prints a row: it actually writes the code to print
// the numbers, and adds the relevant bit to the end of each of the lines
void printrow_pdf::print(const row& r)
{
  // Start a new column if we need to
  if(!in_column) start_column();

  // Print the row
  count++;
  if(opt.flags & printrow::options::numbers) {
    string s(r.bells(), ' ');
    map<bell, printrow::options::line_style>::const_iterator i;
    for(int j = 0; j < r.bells(); j++) 
      if(!(opt.flags & printrow::options::miss_numbers) 
	 || ((i = opt.lines.find(r[j])) == opt.lines.end())
	 || ((*i).second.crossing))
	s[j] = r[j].to_char();
    rows.push_back(pair<string, int>(s, gapcount));
    gapcount = 0;
  } else
    gapcount++;
  
  lastrow = r;

  // Add the various bits of lines to the end of the line
  list<drawline_pdf>::iterator j;
  for(j = drawlines.begin(); j != drawlines.end(); j++)
    (*j).add(r);
}

void printrow_pdf::rule()
{
  if(!in_column) return;
  rules.push_back(rule_pdf(currx - opt.xspace.in_points()/2, 
			   curry - opt.yspace.in_points() * (count - 0.5f),
			   opt.xspace.in_points() * lastrow.bells()));
}

void printrow_pdf::set_position(const dimension& x, const dimension& y)
{
  if(in_column) end_column();
  currx = static_cast<int>(x.in_points());
  curry = static_cast<int>(y.in_points());
}

void printrow_pdf::move_position(const dimension& x, const dimension& y)
{
  if(in_column) end_column();
  currx += static_cast<int>(x.in_points());
  curry += static_cast<int>(y.in_points());
}

void printrow_pdf::start()
{
  if(!cw.font(opt.style.font))
    cw.font("Helvetica");
}

void printrow_pdf::start_column()
{
  map<bell, printrow::options::line_style>::iterator i;
  for(i = opt.lines.begin(); i != opt.lines.end(); i++)
    drawlines.push_back(drawline_pdf(*this, (*i).first, (*i).second));
  in_column = true;
  count = gapcount = 0;
  pp.set_colour(opt.style.col, true);
  pp.set_colour(opt.style.col, false);
  pp.f << opt.style.size / 200.0 << " w\n";
}

void printrow_pdf::end_column()
{
  // Draw the grid if desired
  if(opt.grid_type > 0)
    grid();

  float tx = 0, ty = 0;
  pp.f << "BT\n/"
       << pp.f.get_font(cw.font()) << ' ' << (opt.style.size / 10.0) << " Tf\n";
  // Draw random bits of text
  if(!text_bits.empty()) {
    pp.f << "0 Tc 100 Tz\n";
    list<text_bit>::iterator i;
    float x, y; bool squashed = false; float sq = 1.0;
    for(i = text_bits.begin(); i != text_bits.end(); ++i) {
      if((*i).squash != squashed) {
	squashed = (*i).squash;
	if(squashed) { 
	  pp.f << "80 Tz "; sq = 0.8f;
	} else {
	  pp.f << "100 Tz "; sq = 1.0f; 
	}
      }
      x = currx + (*i).x;
      switch((*i).al) {
	case text_style::right :
	  x -= cw((*i).s) * opt.style.size * sq / 10000.0f;
	  break;
	case text_style::centre :
	  x -= cw((*i).s) * opt.style.size * sq / 20000.0f;
	  break;
      default:
	// I.e. left, assume we don't need to do
	// anything here - but put in default to 
	// stop warnings in build.
	break;
      }
      y = curry - opt.style.size * 0.03f - (*i).y;
      pp.f << x - tx << ' ' << y - ty << " Td ";
      pp.f.output_string((*i).s);
      pp.f << " Tj\n";
      tx = x; ty = y;
    }
    text_bits.clear();
    if(squashed) pp.f << "100 Tz\n";
  }
  // Draw the rows
  if(!rows.empty()) {
    list<pair<string, int> >::iterator j;
    string::iterator k;
    int w, w1;
    pp.f << opt.xspace.in_points() << " Tc " 
	 << opt.yspace.in_points() << " TL\n"
	 << currx - tx << ' ' 
	 << curry - opt.style.size * 0.03 - ty << " Td\n";
    for(j = rows.begin(); j != rows.end(); j++) {
      w = 0;
      if((*j).second) 
	pp.f << "0 " << -(*j).second * opt.yspace.in_points() << " Td\n";
      pp.f << '[';
      for(k = (*j).first.begin(); k != (*j).first.end(); k++) {
	w1 = cw(*k) / 2;
	pp.f << (w1 + w) << " (" << *k << ") ";
	w = cw(*k) - w1;
      }
      pp.f << "] TJ T*\n";
    }
    rows.clear();
  }
  pp.f << "ET\n";

  // Draw rule-offs
  {
    list<rule_pdf>::iterator i;
    for(i = rules.begin(); i != rules.end(); i++)
      (*i).output(pp.f);
    rules.clear();
  }

  // Draw circles
  {
    list<circle_pdf>::iterator i;
    for(i = circles.begin(); i != circles.end(); i++)
      (*i).output(pp);
    circles.clear();
  }

  // Draw arrows
  {
    list<arrow_pdf>::iterator i;
    for(i = arrows.begin(); i != arrows.end(); i++)
      (*i).output(pp);
    arrows.clear();
  }

  // Draw the lines
  if(!drawlines.empty()) {
    list<drawline_pdf>::iterator i;
    for(i = drawlines.begin(); i != drawlines.end(); i++)
      (*i).output(pp.f);
    drawlines.erase(drawlines.begin(), drawlines.end());
  }

  in_column = false;
}

void printrow_pdf::grid()
{
  float y1 = curry - opt.yspace.in_points() * (count - 0.5f);
  float y2 = curry + opt.yspace.in_points() * 0.5f;
  int i;
  switch(opt.grid_type) {
    case 1:
      pp.f << "q " << opt.grid_style.width.in_points() << " w ";
      pp.set_colour(opt.grid_style.col);
      for(i = 0; i < lastrow.bells(); ++i) {
	pp.f << (currx + opt.xspace.in_points() * i) << ' '
	     << y1 << " m "
	     << (currx + opt.xspace.in_points() * i) << ' '
	     << y2 << " l\n";
      }
      pp.f << "S Q\n";
      break;
    case 3:
      if (lastrow.bells() % 2) {
        pp.f << "q " << opt.grid_style.width.in_points() << " w ";
        pp.set_colour(opt.grid_style.col);
	pp.f << (currx + opt.xspace.in_points() * (lastrow.bells()-1)) << ' '
	     << y1 << " m "
	     << (currx + opt.xspace.in_points() * (lastrow.bells()-1)) << ' '
	     << y2 << " l\n";
        pp.f << "S Q\n";
      }  
      // fall through
    case 2:
      pp.f << "q ";
      pp.set_colour(opt.grid_style.col, true);
      for(i = 0; i < lastrow.bells() - 1; i += 2) {
	pp.f << (currx + opt.xspace.in_points() * i) << ' '
	     << y1 << ' ' << opt.xspace.in_points() << ' '
	     << y2-y1 << " re\n";
      }
      pp.f << " f Q\n";
      break;
  }
}

void printrow_pdf::dot(int i)
{
  if(i == -1) {
    map<bell, printrow::options::line_style>::const_iterator j;
    for(j = opt.lines.begin(); j != opt.lines.end(); j++)
      if(!(*j).second.crossing) dot((*j).first);
  } else {
    int j = 0;
    while(j < lastrow.bells() && lastrow[j] != i) j++;
    if(j < lastrow.bells()) {
      map<bell, printrow::options::line_style>::const_iterator k;
      k = opt.lines.find(i);
      if(k != opt.lines.end()) {
	circles.push_back(circle_pdf(currx + j * opt.xspace.in_points(),
				     curry - (count - 1) * opt.yspace.in_points(),
				     (*k).second.width.in_points() * 2, 'f',
				     (*k).second.col));
      }
    }
  }
}

void printrow_pdf::placebell(int i, int dir)
{
  int j = 0;
  while(j < lastrow.bells() && lastrow[j] != i) j++;
  if(j < lastrow.bells()) {
    float radius = opt.style.size * 0.07f;
    float xoff = lastrow.bells() + 0.5f;
    xoff *= opt.xspace.in_points();
    if (dir < 0) xoff += 2 * radius;
    float yoff = (count - 1) * opt.yspace.in_points();

    circles.push_back(circle_pdf(currx + xoff, curry - yoff, radius, 'S'));
    if (dir > 0)
      arrows.push_back(arrow_pdf(currx + xoff, curry - yoff - radius,
                                 currx + xoff, curry - yoff - 2.5*radius, 
                                 radius));
    else if (dir < 0)
      arrows.push_back(arrow_pdf(currx + xoff, curry - yoff + radius,
                                 currx + xoff, curry - yoff + 2.5*radius,
                                 radius));
    text_bit tb;
    tb.x = xoff;
    tb.y = (count - 1) * opt.yspace.in_points();
    tb.al = text_style::centre;
    tb.squash = (j >= 10);
    tb.s = make_string() << j+1;
    text_bits.push_back(tb);
  }
}

void printrow_pdf::text(const string& t, const dimension& x, 
		       text_style::alignment al, bool between, bool right)
{
  text_bit tb;
  tb.x = right ? 
    (x.in_points() + (lastrow.bells() - 1) * opt.xspace.in_points())
    : -x.in_points();
  tb.y = (between ? (count - 0.5f) : (count - 1)) * opt.yspace.in_points();
  tb.al = al;
  tb.squash = false;
  tb.s = t;
  text_bits.push_back(tb);
}

void drawline_pdf::add(const row& r)
{
  int j;
  for(j = 0; j < r.bells() && r[j] != bellno; j++);
  if(j == r.bells()) j = -1;
  if(curr == -1)
    l.push_back(j);
  else {
    if(j != -1) {
      if(!s.crossing || (j != curr && p.has_line(r[curr]))) {
	if(j == curr) l.push_back(-3);
	else if(j == curr - 1) l.push_back(-4);
	else if(j == curr + 1) l.push_back(-2);
	else l.push_back(j);
      } else l.push_back(j);
    } else
      l.push_back(-1);
  }
  curr = j;
}

void drawline_pdf::output(pdf_file& f)
{
  list<int>::iterator i;
  int t, count, x = 0, y = 0;
  i = l.begin(); count = -1;
  f << s.width.in_points() << " w ";
  p.pp.set_colour(s.col);
  while(i != l.end()) {
    t = (*i);
    while(i != l.end() && ((t >= -1 && *i >= -1) || (t == *i))) 
	{ t = *i; i++; count++; }

    y += count;
    switch(t) {
      case -2: x += count; break;
      case -4: x -= count; break;
      case -3: break;
      default: x = t; break;
    }
    f << p.currx + x * p.opt.xspace.in_points() << ' '
      << p.curry - y * p.opt.yspace.in_points();
    if(t <= -2) f << " l "; else f << " m ";
    count = 0;
  }
  f << "S\n";
}

void rule_pdf::output(pdf_file& f)
{
  f << x << ' ' << y << " m " << x+l << ' ' << y << " l S\n";
}

void circle_pdf::output(printpage_pdf& pp)
{
  pp.gsave();
  if(set_colour) pp.set_colour(c, (op == 'f'));
  pp.circle(x, y, r, op);
  pp.grestore();
}

void arrow_pdf::output(printpage_pdf& pp)
{
  pp.gsave();
  pp.arrow(x0, y0, x1, y1, headsize);
  pp.grestore();
}

RINGING_END_NAMESPACE
