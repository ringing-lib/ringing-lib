// -*- c++ -*-
// print_ps.h : printing things in PostScript

#ifndef METHLIB_PRINT_PS_H
#define METHLIB_PRINT_PS_H

#ifdef __GNUG__
#pragma interface
#endif

#include <list.h>
#include <map.h>
#include <iostream.h>
#include "print.h"

class printpage_ps : public printpage {
protected:
  ostream& os;
  bool eps;
  static const char *def_string;

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

#endif
