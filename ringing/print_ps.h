// -*- c++ -*- print_ps.h : printing things in PostScript
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

#ifndef RINGING_PRINT_PS_H
#define RINGING_PRINT_PS_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <list.h>
#include <map.h>
#include <set.h>
#include <iostream.h>
#include <iomanip.h>
#else
#include <list>
#include <map>
#include <set>
#include <iostream>
#include <iomanip>
#endif
#include <ringing/print.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class printrow_ps;
class printpage_ps;

class drawline_ps {
private:
  const printrow_ps& p;
  bell bellno;
  printrow::options::line_style s;
  list<int> l;
  int curr;
  
public:
  drawline_ps(const printrow_ps& pr, bell b, 
	   printrow::options::line_style st) : 
    p(pr), bellno(b), s(st), curr(-1) {}
  void add(const row& r);
  void output(ostream& o, int x, int y);

  RINGING_FAKE_DEFAULT_CONSTRUCTOR( drawline_ps );
  RINGING_FAKE_COMPARATORS( drawline_ps );
  RINGING_FAKE_ASSIGNMENT( drawline_ps );
};

class printrow_ps : public printrow::base {
private:
  printpage_ps& pp;
  int currx, curry;
  bool in_column;
  row lastrow;
  int gapcount;
  printrow::options opt;
  
  list<drawline_ps> drawlines;
  friend class drawline_ps;
  bool has_line(int b) const { return opt.lines.find(b) != opt.lines.end(); }
  
  void start();
  void start_column();
  void end_column();
  void fill_gap();

public:
  printrow_ps(printpage_ps& p, const printrow::options& op) 
    : pp(p), in_column(false), lastrow(8), opt(op) { start(); }
  ~printrow_ps() { if(in_column) end_column(); }
  void print(const row& r);
  void rule();
  void set_position(const dimension& x, const dimension& y);
  void move_position(const dimension& x, const dimension& y);
  void set_options(const printrow::options& o) { opt = o; }
  const printrow::options& get_options() { return opt; }
  void dot(int i); 
  void placebell(int i);
  void text(const string& t, const dimension& x, 
	    text_style::alignment al, bool between, bool right);
};

class printpage_ps : public printpage {
protected:
  ostream& os;
  bool eps, landscape;
  static const string def_string, header_string;
  int pages;
  set<string> used_fonts;
  int ph;

public:
  // Portrait output
  printpage_ps(ostream& o);
  // Landscape output
  printpage_ps(ostream& o, const dimension& page_height);
  // EPS output
  printpage_ps(ostream& o, int x0, int y0, int x1, int y1);
  ~printpage_ps();
  void text(const string t, const dimension& x, const dimension& y,
       text_style::alignment al, const text_style& s);
  void new_page();
  void add_font(const string& s) { used_fonts.insert(s); }

private:
  friend class printrow;
  friend class printrow_ps;
  friend class drawline_ps;
  printrow::base* new_printrow(const printrow::options& o) 
    { return new printrow_ps(*this, o); }

protected:
  void set_text_style(const text_style& s);
  void set_colour(const colour& c);
  void write_string(const string& s);
  void landscape_mode();
};

RINGING_END_NAMESPACE

#endif
