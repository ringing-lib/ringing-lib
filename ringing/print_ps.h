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

#ifndef RINGING_PRINT_PS_H
#define RINGING_PRINT_PS_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include RINGING_STD_HEADER(list)
#include RINGING_STD_HEADER(map)
#include RINGING_STD_HEADER(iostream)
#include RINGING_LOCAL_HEADER(print)
RINGING_USING_STD

RINGING_START_NAMESPACE

class printpage_ps : public printpage {
protected:
  ostream& os;
  bool eps;
  static const string def_string;

public:
  printpage_ps(ostream& o);
  printpage_ps(ostream& o, int x0, int y0, int x1, int y1);
  ~printpage_ps() {
    if(eps) os << "restore\n"; else os << "showpage\n";
  }
  void text(const string t, const dimension& x, const dimension& y,
       text_style::alignment al, const text_style& s);

private:
  friend class printrow;
  printrow::base* new_printrow(const printrow::options& o) 
    { return new printrow_ps(os, o); }

protected:
  void set_text_style(const text_style& s);

protected:
  class printrow_ps : public printrow::base {
  private:
    ostream& os;
    int currx, curry;
    bool in_column;
    row lastrow;
    printrow::options opt;

    class drawline {
    private:
      const printrow_ps& p;
      int bell;
      printrow::options::line_style s;
      list<int> l;
      int curr;

    public:
      drawline(const printrow_ps& pr, int b, 
	       printrow::options::line_style st) : 
	p(pr), bell(b), s(st), curr(-1) {}
      void add(const row& r);
      void output(ostream& o, int x, int y);
    };
    list<drawline> drawlines;
    friend class drawline;
    bool has_line(int b) const { return opt.lines.find(b) != opt.lines.end(); }

    void start();
    void start_column();
    void end_column();

  public:
    printrow_ps(ostream& o, const printrow::options& op) 
      : os(o), in_column(false), opt(op), lastrow(8) { start(); }
    ~printrow_ps() { end_column(); }
    void print(const row& r);
    void rule();
    void set_position(const dimension& x, const dimension& y);
    void new_column(const dimension& gap);
    void set_options(const printrow::options& o) { opt = o; }
    const printrow::options& get_options() { return opt; }
    void dot(int i); 
    void placebell(int i);
    void text(const string& t, const dimension& x, 
	      text_style::alignment al, bool between, bool right);
  };

};

RINGING_END_NAMESPACE

#endif
