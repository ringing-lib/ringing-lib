// -*- c++ -*- print_svg.h : SVG output
// Copyright (C) 2017 Martin Bright <martin@boojum.org.uk>

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

#ifndef RINGING_PRINT_SVG_H
#define RINGING_PRINT_SVG_H

#include <ringing/common.h>
#include <ringing/dom.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <list.h>
#include <map.h>
#include <set.h>
#include <iostream.h>
#include <iomanip.h>
#include <sstream.h>
#else
#include <list>
#include <map>
#include <set>
#include <iostream>
#include <iomanip>
#include <sstream>
#endif
#include <ringing/print.h>
#include <ringing/dom.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class printrow_svg;
class printpage_svg;

class RINGING_API drawline_svg {
private:
  const printrow_svg& p;
  bell bellno;
  printrow::options::line_style s;
  string data;
  int curr;
  
public:
  drawline_svg(const printrow_svg& pr, bell b,
               printrow::options::line_style st);

  void add(const row& r);
  void output(dom_element parent);
  
  RINGING_FAKE_DEFAULT_CONSTRUCTOR( drawline_svg )
  RINGING_FAKE_COMPARATORS( drawline_svg )
  RINGING_FAKE_ASSIGNMENT( drawline_svg )
};

class RINGING_API printrow_svg : public printrow::base {
private:
  printpage_svg& pp;
  float currx, curry;
  bool in_column;
  row lastrow;
  int count;
  printrow::options opt;
  dom_element col;
  dom_element numbers;
  
  list<drawline_svg> drawlines;
  friend class drawline_svg;
  bool has_line(int b) const { return opt.lines.find(b) != opt.lines.end(); }
  
//  void start();
  void start_column();
  void end_column();
  void grid();

public:
  printrow_svg(printpage_svg& p, const printrow::options& op)
    : pp(p), in_column(false), lastrow(8), opt(op) { }
  ~printrow_svg() { if(in_column) end_column(); }
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

class RINGING_API printpage_svg : public printpage {
protected:
  static const char* ns;
  dom_document doc;
  dom_element root;
  int ph;
  
public:
  printpage_svg(const string& filename, const dimension& w, const dimension& h);
  ~printpage_svg();
  void text(const string t, const dimension& x, const dimension& y,
       text_style::alignment al, const text_style& s);
  void new_page();

private:
  friend class printrow;
  friend class printrow_svg;
  friend class drawline_svg;
  printrow::base* new_printrow(const printrow::options& o) 
    { return new printrow_svg(*this, o); }

protected:
  static string convert_dim(const dimension& d);
  static string format_float(float f);
  static string convert_col(const colour& c);
  static string convert_font(const text_style& s);
};

RINGING_END_NAMESPACE

#endif
