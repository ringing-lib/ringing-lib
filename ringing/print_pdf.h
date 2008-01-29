// -*- c++ -*- print_pdf.h : printing things in PDF
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

#ifndef RINGING_PRINT_PDF_H
#define RINGING_PRINT_PDF_H

#include <ringing/common.h>

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
#else
#include <list>
#include <map>
#include <set>
#include <iostream>
#endif
#include <ringing/print.h>
#include <ringing/pdf_fonts.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class RINGING_API counting_streambuf : public streambuf {
private:
  int c;
  streambuf& sb;

public:
  counting_streambuf(streambuf* s) : c(0), sb(*s) {}
  int sync() { 
    int n = pptr() - pbase(); c += n;
    return sb.sputn(pptr(), n); 
  }
  int overflow(int ch = EOF) { 
    int n = pptr() - pbase(); 
    if(n && sync()) return EOF; pbump(-n);
    if(ch != EOF) { ++c; sb.sputc(ch); }
    return 0;
  }
  int get_count() { if(pptr() != pbase()) sync(); return c; }
  void reset_count() { c = 0; }
};

// This represents a PDF file.
class RINGING_API pdf_file {
private:
  counting_streambuf csb;
  ostream os;
  int obj_count;
  map<int, int> offsets;
  int width, height;
  bool landscape;
  int pages;
  int stream_start;
  map<string, string> fonts;
  int font_counter;

public:
  pdf_file(ostream& o, bool is_landscape = false, 
           int pagewidth = 590, int pageheight = 835);
 ~pdf_file() { end(); }

  void start();
  void end();
  int start_object(int n = 0);
  void end_object();
  void start_stream();
  void end_stream();
  void start_page();
  void end_page();
  void output_catalogue();
  void output_info();
  void output_pages();
  const string& get_font(const string& f);
  bool get_landscape() { return landscape; }
  void output_string(const string& s);
  int get_width() { return width; }
  int get_height() { return height; }

  template<class T> pdf_file& operator<<(const T& t)
    { os << t; return *this; }
};

class RINGING_API printrow_pdf;
class RINGING_API printpage_pdf;

class RINGING_API drawline_pdf {
private:
  const printrow_pdf& p;
  bell bellno;
  printrow::options::line_style s;
  list<int> l;
  int curr;
  
public:
  drawline_pdf(const printrow_pdf& pr, bell b, 
	   printrow::options::line_style st) : 
    p(pr), bellno(b), s(st), curr(-1) {}
  void add(const row& r);
  void output(pdf_file& f);

  RINGING_FAKE_DEFAULT_CONSTRUCTOR( drawline_pdf )
  RINGING_FAKE_COMPARATORS( drawline_pdf )
  RINGING_FAKE_ASSIGNMENT( drawline_pdf )
};

class RINGING_API rule_pdf {
private:
  float x, y, l;

public:
  rule_pdf(float _x, float _y, float _l) : x(_x), y(_y), l(_l) {}
  void output(pdf_file& f);

  RINGING_FAKE_DEFAULT_CONSTRUCTOR( rule_pdf )
  RINGING_FAKE_COMPARATORS( rule_pdf )
  RINGING_FAKE_ASSIGNMENT( rule_pdf )
};

class RINGING_API circle_pdf {
private:
  float x, y, r;
  char op;
  bool set_colour;
  colour c;

public:
  circle_pdf(float _x, float _y, float _r, char _op, colour _c) : 
    x(_x), y(_y), r(_r), op(_op), set_colour(true), c(_c) {}
  circle_pdf(float _x, float _y, float _r, char _op) : 
    x(_x), y(_y), r(_r), op(_op), set_colour(false) {}
  void output(printpage_pdf& pp);

  RINGING_FAKE_DEFAULT_CONSTRUCTOR( circle_pdf )
  RINGING_FAKE_COMPARATORS( circle_pdf )
  RINGING_FAKE_ASSIGNMENT( circle_pdf )
};

struct text_bit {
  float x, y;
  text_style::alignment al;
  bool squash;
  string s;

  RINGING_FAKE_COMPARATORS( text_bit )
};

class RINGING_API printrow_pdf : public printrow::base {
private:
  printpage_pdf& pp;
  int currx, curry;
  bool in_column;
  row lastrow;
  int count;
  int gapcount;
  printrow::options opt;
  charwidths cw;
  list<pair<string, int> > rows;
  list<text_bit> text_bits;
  list<rule_pdf> rules;
  list<circle_pdf> circles;

  list<drawline_pdf> drawlines;
  friend class drawline_pdf;
  bool has_line(int b) const { return opt.lines.find(b) != opt.lines.end(); }
  
  void start();
  void start_column();
  void end_column();
  void grid();

public:
  printrow_pdf(printpage_pdf& p, const printrow::options& op) 
    : pp(p), in_column(false), lastrow(8), opt(op) { start(); }
  ~printrow_pdf() { if(in_column) end_column(); }
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

class RINGING_API printpage_pdf : public printpage {
protected:
  pdf_file f;

public:
  printpage_pdf(ostream& o, const dimension& w, const dimension& y, 
		bool ls = false);
  printpage_pdf(ostream& o, bool ls = false);
  ~printpage_pdf();
  void text(const string t, const dimension& x, const dimension& y,
       text_style::alignment al, const text_style& s);
  void new_page();

private:
  friend class printrow;
  friend class printrow_pdf;
  friend class drawline_pdf;
  friend class circle_pdf;
  printrow::base* new_printrow(const printrow::options& o) 
    { return new printrow_pdf(*this, o); }

protected:
  void set_colour(const colour& c, bool nonstroke = false);
  void landscape_mode();
  void circle(float x, float y, float r, char op);
  void gsave() { f << "q\n"; }
  void grestore() {f << "Q\n"; }
};

RINGING_END_NAMESPACE

#endif
