// -*- c++ -*- print_pdf.h : printing things in PDF
// Copyright (C) 2002 Martin Bright <M.Bright@dpmms.cam.ac.uk>

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

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <list.h>
#include <map.h>
#include <set.h>
#include <iostream.h>
#include <strstream.h>
#else
#include <list>
#include <map>
#include <set>
#include <iostream>
#include <strstream>
#endif
#include <ringing/print.h>
#include <ringing/pdf_fonts.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class counting_ostream {
private:
  ostream& os;
  int c;

public:
  counting_ostream(ostream& o) : os(o), c(0) {}
  counting_ostream& operator<<(const char* s) 
    { c += strlen(s); os << s; return *this; }
  counting_ostream& operator<<(char* s) 
    { c += strlen(s); os << s; return *this; }
  counting_ostream& operator<<(char ch)
    { ++c; os << ch; return *this; }
  template<class T> counting_ostream& operator<<(const T& t) { 
    ostrstream oss; oss.flags(os.flags()); 
    oss << t; c += oss.pcount();
    os << t; return *this;
  }
  ostream& get_ostream() { return os; }
  int count() { return c; }
};

class counting_streambuf : public streambuf {
private:
  streambuf& sb;
  int c;

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
class pdf_file {
private:
  ostream os;
  counting_streambuf csb;
  int obj_count;
  map<int, int> offsets;
  int pages;
  int stream_start;
  map<string, string> fonts;
  int font_counter;

public:
  pdf_file(ostream& o) : csb(o.rdbuf()), os(&csb) { start(); }
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
  void output_string(const string& s);

  template<class T> pdf_file& operator<<(const T& t)
    { os << t; return *this; }
};

class printrow_pdf;
class printpage_pdf;

class drawline_pdf {
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

#if RINGING_PREMATURE_MEMBER_INSTANTIATION
  // These are to make templates work and don't really exist
  drawline_pdf();
  bool operator<(const drawline_pdf&) const;
  bool operator==(const drawline_pdf&) const;
  bool operator!=(const drawline_pdf&) const;
  bool operator>(const drawline_pdf&) const;
  drawline_pdf& operator=(const drawline_pdf&);
#endif
};

struct text_bit {
  float x, y;
  text_style::alignment al;
  bool squash;
  string s;
#if RINGING_PREMATURE_MEMBER_INSTANTIATION
  bool operator==(const text_bit &) const;
  bool operator!=(const text_bit &) const;
  bool operator<(const text_bit &) const;
  bool operator>(const text_bit &) const;
#endif
};

class printrow_pdf : public printrow::base {
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

  list<drawline_pdf> drawlines;
  friend class drawline_pdf;
  bool has_line(int b) const { return opt.lines.find(b) != opt.lines.end(); }
  
  void start();
  void start_column();
  void end_column();

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

class printpage_pdf : public printpage {
protected:
  pdf_file f;

public:
  printpage_pdf(ostream& o);
  ~printpage_pdf();
  void text(const string t, const dimension& x, const dimension& y,
       text_style::alignment al, const text_style& s);
  void new_page();

private:
  friend class printrow;
  friend class printrow_pdf;
  friend class drawline_pdf;
  printrow::base* new_printrow(const printrow::options& o) 
    { return new printrow_pdf(*this, o); }

protected:
  void set_colour(const colour& c);
  void landscape_mode();
  void circle(float x, float y, float r, char op);
};

RINGING_END_NAMESPACE

#endif
