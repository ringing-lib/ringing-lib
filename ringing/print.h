// -*- c++ -*-
// print.h - printing stuff

#ifndef METHLIB_PRINT_H
#define METHLIB_PRINT_H

#ifdef __GNUG__
#pragma interface
#endif

#include <iostream.h>
#include <map.h>
#include <string>
#include "dimension.h"
#include "row.h"

class printpage;

struct text_style {
  string font;
  int size;

  enum alignment { left, right, centre };
};

class printrow {
public:
  // options : Options for outputting blue line
  // These together form a blue line `style'
  struct options {
    enum o_flags {
      numbers = 0x01,		// Print rows
      miss_numbers = 0x02       // Miss out numbers underneath lines
    };
    unsigned int flags;		// Flags as above
    text_style style;           // Text style
    dimension xspace, yspace;	// Spacing between places and rows

    struct line_style {
      dimension width;
    };
    map<int, line_style> lines; // What lines draw in what styles

    void defaults();
  };

  // This is the class from which all implementations are derived
  class base {
  public:
    virtual ~base() {}
    virtual void print(const row& r) = 0; // Print a row
    virtual void rule() = 0;
    virtual void set_position(const dimension& x, const dimension& y) = 0;
    virtual void new_column(const dimension& gap) = 0;
    virtual void set_options(const options& o) = 0;
    virtual const options& get_options() = 0;
    virtual void dot(int i) = 0;
    virtual void placebell(int i) = 0;
    virtual void text(const string& t, const dimension& x, 
		      text_style::alignment al, bool between, bool right) = 0;
  };

private:
  base* pr;

  // Stop people calling these
  printrow(const printrow&);
  printrow& operator=(const printrow&);

public:
  printrow(printpage& pp);
  printrow(printpage& pp, const options& o);
  ~printrow() { delete pr; }

  printrow& operator<<(const row& r) { pr->print(r); return *this; }
  void rule() { pr->rule(); }
  void set_position(const dimension& x, const dimension& y)  
    { pr->set_position(x, y); }
  void new_column(const dimension& gap)
    { pr->new_column(gap); }
  void set_options(const options& o)
    { pr->set_options(o); }
  const options& get_options()
    { return pr->get_options(); } 
  void dot(int i)
    { pr->dot(i); }
  void placebell(int i)
    { pr->placebell(i); } 
  void text(const string& t, const dimension& x, 
	    text_style::alignment al, bool between, bool right)
    { pr->text(t, x, al, between, right); }
};

class printpage {
public:
  virtual ~printpage() {}
  virtual void text(const string t, const dimension& x, const dimension& y, 
	       text_style::alignment al, const text_style& s) = 0;
protected:
  friend class printrow;
  virtual printrow::base* new_printrow(const printrow::options&) = 0;
};

#endif
