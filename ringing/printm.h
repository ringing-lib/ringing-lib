// -*- C++ -*- printm.h - Printing of whole methods
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

#ifndef RINGING_PRINTM_H
#define RINGING_PRINTM_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include <ringing/print.h>
#include <ringing/method.h>

RINGING_START_NAMESPACE

class printmethod {
private:
  const method* m;

public:
  printrow::options opt;
  dimension hgap, vgap;
  int total_rows;
  int rows_per_column;
  int columns_per_set;
  int sets_per_page;
  dimension xoffset, yoffset;
  list<pair<int,int> > rules;

  enum number_mode_t { miss_never, miss_always, miss_column, miss_lead };
  number_mode_t number_mode;
  enum pn_mode_t { pn_none, pn_first, pn_all };
  pn_mode_t pn_mode;
  int placebells;
    
  void defaults(); 

  printmethod() : m(0) {}
  printmethod(const method& mm) : m(&mm) {}
  void set_method(const method& mm) { m = &mm; }
  const method& get_method() { return *m; }

  void print(printpage& pp);

  float total_width();
  float total_height();

  void fit_to_space(const dimension& width, const dimension& height, 
		    bool vgap_mode, float aspect);

private:
  bool needrule(int i);
  int find_pnextra();
  static int divd(int a, int b) { return (a - a % b) / b; }
  static int divu(int a, int b) 
    { return (a % b == 0) ? (a / b) : divd(a,b) + 1; }
};

RINGING_END_NAMESPACE

#endif
