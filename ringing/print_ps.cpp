// print_ps.cpp - PostScript printing stuff
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include <ringing/print_ps.h>

RINGING_START_NAMESPACE

const string printpage_ps::def_string =
"%%BeginProlog\n"
"%%BeginResource: procset " RINGING_PACKAGE " " RINGING_VERSION " 0\n"
"/BD {bind def} bind def\n"
"/C {dup stringwidth pop 2 div neg offset neg rmoveto show} BD\n"
"/TR {dup stringwidth pop neg 4 -1 roll add 3 -1 roll moveto show} BD\n"
"/TC {dup stringwidth pop 2 div neg 4 -1 roll add 3 -1 roll moveto show} BD\n"
"/TL {3 1 roll moveto show} BD\n"
"/F {exch findfont exch scalefont setfont} BD\n"
"/O {\n"
"  newpath \n"
"  xspace mul xstart add ypos yspace add \n"
"  3 -1 roll 2 mul 0 360 arc closepath fill\n"
"} BD\n"
"/D {0 offset neg rmoveto show} BD\n"
"/DR {dup stringwidth pop neg offset neg rmoveto show} BD\n"
"/E {currentpoint gsave translate 0.8 1 scale 0 0 M C grestore} BD\n"
"/PB {\n"
"  dup 20 div setlinewidth\n"
"  exch 1 add xspace mul xstart add ypos yspace add \n"
"  newpath 2 copy 5 -1 roll 2 mul 3 div 0 360 arc stroke\n"
"  moveto\n"
"} BD \n"
"/MR {xspace mul xstart add ypos yspace add moveto} BD\n"
"/M {moveto} BD\n"
"/L {lineto} BD\n"
"/R {rlineto} BD\n"
"/S {stroke} BD\n"
"/N {newpath} BD\n"
"/SL {setlinewidth} BD\n"
"/SC {setrgbcolor} BD\n"
"/SG {setgray} BD\n"
"/buf 1 string def\n"
"/W { \n"
"   /xpos xstart def\n"
"   {\n"
"      buf dup 0 4 -1 roll put\n"
"      xpos dup xspace add /xpos exch def ypos M C\n"
"   } forall\n"
"   /ypos ypos yspace sub def\n"
"} BD\n"
"/RO {\n"
"  0 SL N\n"
"  xstart xspace 2 div sub ypos yspace 2 div add M\n"
"  xspace mul 0 R S\n"
"} BD\n"
"/G {yspace mul ypos exch sub /ypos exch def} BD\n"
"/X {/xstart exch def} BD\n"
"/Y {/ypos exch def} BD\n"
"/DL1 { 1 exch { dup 49 ge exch dup 57 le 3 -1 roll \n"
"and { 48 sub exch pop }\n"
"{ 2 index exch get exch { dup dup type /stringtype eq\n"
"{ 2 index exch DL1 pop } { exec } ifelse } repeat pop 1 } ifelse }\n"
"forall pop } BD \n"
"/DL { DLD exch DL1 pop } BD\n"
"/DLD 4 dict def\n"
"/defline { exch 0 get DLD 3 1 roll exch put } BD\n"
"(u) { xspace yspace neg R } defline\n"
"(d) { xspace neg yspace neg R } defline\n"
"(p) { 0 yspace neg R } defline\n"
"/J { yspace mul neg exch xspace mul xstart add \n"
"currentpoint pop neg add exch rmoveto } BD\n"
"%%EndResource\n"
"%%EndProlog\n\n";

const string printpage_ps::header_string =
"%%Creator: Ringing Class Library (" RINGING_PACKAGE " " 
RINGING_VERSION ") \n"
"%%DocumentSuppliedResources: procset " RINGING_PACKAGE " "
RINGING_VERSION " 0\n"
"%%DocumentNeededResources: (atend)\n"
"%%EndComments\n\n";

printpage_ps::printpage_ps(ostream& o) : os(o), eps(false)
{
  pages = 1;
  os << "%!PS-Adobe-3.0\n" << "%%Pages: (atend)\n" << header_string 
     << def_string << "%%Page: 1\nsave\n";
}

printpage_ps::printpage_ps(ostream& o, int x0, int y0, int x1, int y1)
  : os(o), eps(true)
{
  pages = 0;
  os << "%!PS-Adobe-3.0 EPSF-3.0\n"
     << "%%BoundingBox: " << x0 << ' ' << y0 
     << ' ' << x1 << ' ' << y1
     << "%%Pages: 0\n" << header_string << def_string;
}

printpage_ps::~printpage_ps()
{
  if(!eps) os << "restore showpage\n";
  os <<"\n%%Trailer\n";
  if(!eps) os << "%%Pages: " << pages << "\n";
  os << "%%DocumentNeededResources:";
  if(!used_fonts.empty()) {
    os << " font ";
    copy(used_fonts.begin(), used_fonts.end(), 
	 ostream_iterator<string>(os, " "));
    os << '\n';
  } 
  os << "%%EOF\n";
}


void printpage_ps::new_page()
{
  pages++;
  os << "restore showpage\n\n%%Page: " << pages << "\nsave\n";
}

void printpage_ps::set_text_style(const text_style& s)
{
  os << '/' << s.font << ' ' << s.size << " F\n";
  set_colour(s.col);
  add_font(s.font);
}

void printpage_ps::set_colour(const colour& c)
{
  if(c.grey) 
    os << c.red << " SG\n";
  else
    os << c.red << ' ' << c.green << ' ' << c.blue << " SC\n";
}

// NOTE: At the moment this is quite likely to break when printing
// unusual types of characters.  Putting more left brackets than right
// brackets in the string will _really_ confuse it.  This should be
// put right by somebody who has a PostScript manual.
void printpage_ps::text(const string t, const dimension& x, 
			const dimension& y, text_style::alignment al, 
			const text_style& s)
{
  set_text_style(s);
  os << x.in_points() << ' ' << y.in_points() << " (";
  string::const_iterator i;
  for(i = t.begin(); i != t.end(); i++)
    if(isprint(*i)) os << *i;
  os << ") ";
  switch(al) {
    case text_style::left: os << "TL\n"; break;
    case text_style::right: os << "TR\n"; break;
    case text_style::centre: os << "TC\n"; break;
  }
}

// This function prints a row: it actually writes the code to print
// the numbers, and adds the relevant bit to the end of each of the lines
void printrow_ps::print(const row& r)
{
  // Start a new column if we need to
  if(!in_column) start_column();

  // Print the row
  if(opt.flags & printrow::options::numbers) {
    pp.os << '(';
    for(int j = 0; j < r.bells(); j++) 
      if((opt.flags & printrow::options::miss_numbers) && has_line(r[j]))
	pp.os << ' ';
      else
	pp.os << r[j];
    pp.os << ") W\n";
    gapcount = 0;
  } else
    gapcount++;
  lastrow = r;

  // Add the various bits of lines to the end of the line
  list<drawline_ps>::iterator i;
  for(i = drawlines.begin(); i != drawlines.end(); i++)
    (*i).add(r);
}

void printrow_ps::fill_gap()
{
  if(gapcount > 0) {
    pp.os << gapcount << " G\n";
    gapcount = 0;
  }
}

void printrow_ps::rule()
{
  if(!in_column) return;
  fill_gap();
  pp.os << lastrow.bells() << " RO\n";
}

void printrow_ps::set_position(const dimension& x, const dimension& y)
{
  if(in_column) end_column();
  currx = static_cast<int>(x.in_points());
  curry = static_cast<int>(y.in_points());
}

void printrow_ps::new_column(const dimension& d)
{
  if(in_column) end_column();
  currx += static_cast<int>(d.in_points());
}

void printrow_ps::start()
{
  // Select the font we'll be using
  pp.set_text_style(opt.style);
  // Define xspace and yspace
  pp.os << "/xspace " << opt.xspace.in_points() << " def /yspace "
	<< opt.yspace.in_points() << " def\n";
  // Find the height of an X
  pp.os << "gsave N 0 0 M (X) true charpath pathbbox grestore\n"
	<< "exch pop sub 2 div neg /offset exch def pop\n";
}

void printrow_ps::start_column()
{
  map<int, printrow::options::line_style>::iterator i;
  for(i = opt.lines.begin(); i != opt.lines.end(); i++)
    drawlines.push_back(drawline_ps(*this, (*i).first, (*i).second));
  pp.os << currx << " X " << curry << " Y\n";
  in_column = true;
  gapcount = 0;
}

void printrow_ps::end_column()
{
  if(!drawlines.empty()) {
    list<drawline_ps>::iterator i;
    pp.os << "gsave\n";
    for(i = drawlines.begin(); i != drawlines.end(); i++)
      (*i).output(pp.os, currx, curry);
    pp.os << "grestore\n";
    drawlines.erase(drawlines.begin(), drawlines.end());
  }
  in_column = false;
}

void printrow_ps::dot(int i)
{
  fill_gap();
  if(i == -1) {
    map<int, printrow::options::line_style>::const_iterator j;
    for(j = opt.lines.begin(); j != opt.lines.end(); j++)
      if((*j).first >= 0) dot((*j).first);
  } else {
    int j = 0;
    while(j < lastrow.bells() && lastrow[j] != i) j++;
    if(j < lastrow.bells()) {
      map<int, printrow::options::line_style>::const_iterator k;
      k = opt.lines.find(i);
      if(k != opt.lines.end()) {
	pp.os << "gsave ";
	pp.set_colour((*k).second.col);
	pp.os << "0 SL " << (*k).second.width.in_points() 
	   << ' ' << j << " O grestore\n";
      }
    }
  }
}

void printrow_ps::placebell(int i)
{
  fill_gap();
  int j = 0;
  while(j < lastrow.bells() && lastrow[j] != i) j++;
  if(j < lastrow.bells()) {
    pp.os << lastrow.bells() << ' ' << opt.style.size << " PB (" 
       << j+1 << ((j < 9) ? ") C\n" : ") E\n");
  }
}

void printrow_ps::text(const string& t, const dimension& x, 
		       text_style::alignment al, bool between, bool right)
{
  if(right) pp.os << lastrow.bells(); else pp.os << '0';
  pp.os << " MR ";
  if(right) pp.os << x.in_points(); else pp.os << -(x.in_points());
  if(between) pp.os << " yspace 2 div neg "; else pp.os << " 0 ";
  pp.os << " R (";
  string::const_iterator i;
  for(i = t.begin(); i != t.end(); i++)
    if(isprint(*i)) pp.os << *i;
  pp.os << ") ";
  switch(al) {
    case text_style::left: pp.os << "D\n"; break;
    case text_style::right: pp.os << "DR\n"; break;
    case text_style::centre: pp.os << "C\n"; break;
  }
}

void drawline_ps::add(const row& r)
{
  int j, b;
  b = (bell == -1) ? 0 : bell;
  for(j = 0; j < r.bells() && r[j] != b; j++);
  if(j == r.bells()) j = -1;
  if(curr == -1)
    l.push_back(j);
  else {
    if(j != -1) {
      if(bell != -1 || (j != curr && p.has_line(r[curr]))) {
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

void drawline_ps::output(ostream& o, int x, int y)
{
  list<int>::iterator i;
  int t, count;
  bool in_line = false;
  i = l.begin(); count = -1;
  p.pp.set_colour(s.col);
  o << s.width.in_points() << " SL N " << x << ' ' << y << " M\n";
  while(i != l.end()) {
    t = (*i);
    while(i != l.end() && ((t >= -1 && *i >= -1) || (t == *i))) 
	{ t = *i; i++; count++; }
    if(t <= -2 && !in_line) { o << '('; in_line = true; }
    if(count > 1 && t <= -2) o << count;
    switch(t) {
      case -2: o << 'u'; break;
      case -4: o << 'd'; break;
      case -3: o << 'p'; break;
      default: 
	if(in_line) { o << ") DL "; in_line = false; } 
	o << t << ' ' << count << " J "; 
	break;
    }
    count = 0;
  }
  if(in_line) o << ") DL ";
  o << "S \n";
}

RINGING_END_NAMESPACE
