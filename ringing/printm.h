// -*- C++ -*- printm.h - Printing of whole methods
// Copyright (C) 2001, 2019, 2020, 2021, 2022, 2024
// Martin Bright <martin@boojum.org.uk>
// and Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_PRINTM_H
#define RINGING_PRINTM_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <list>
#include <vector>

#include <ringing/print.h>
#include <ringing/method.h>

RINGING_START_NAMESPACE

class RINGING_API printmethod {
private:
  const method* m;
  row rounds;

public:
  printrow::options opt;
  dimension hgap, vgap, pngap;
  int total_rows;
  int rows_per_column;
  int columns_per_set;
  int sets_per_page;
  dimension xoffset, yoffset;

  struct rule {
    rule();
    rule(pair<int,int> const& p);

    int offset, repeat;
    printrow::options::line_style style;
    printrow::rule_flags flags;
  };
  list<rule> rules;

  enum number_mode_t { miss_never, miss_always, miss_column, miss_lead };
  number_mode_t number_mode;
  enum pn_mode_t { pn_none, pn_first, pn_first_asym, pn_all, 
                   pn_mask = 0x0f, pn_nox = 0x10, pn_lcross = 0x20 };
  pn_mode_t pn_mode;
  int placebells;
  bool reverse_placebells, placebells_at_rules, placebell_blobs_only;
  string calls;
  bool calls_at_rules;
  dimension calls_voffset;

  struct label {
    explicit label( size_t row_number, string const& text = string(), 
                    text_style::alignment align = text_style::left )
      : row_number(row_number), text(text), align(align) {}

    size_t row_number;
    string text;
    text_style::alignment align;
  };
  list<label> labels;

public:
  void defaults(); 

  printmethod() : m(0) {}
  printmethod(const method& mm) : m(&mm) {}
  void set_method(const method& mm) { m = &mm; }
  const method& get_method() { return *m; }

  void print(printpage& pp);

  float total_width();
  float total_height();
  void get_bbox(float& blx, float& bly, float& urx, float& ury);

  void scale_to_space(const dimension &width, const dimension& height,
		      float aspect);

  void fit_to_space(const dimension& width, const dimension& height, 
		    bool vgap_mode, float aspect);

  void startrow(const row& r) { rounds=r; }

private:
  vector<rule> rules_at_posn(int i) const;
  bool needrule(int i) const;
  void dorules(printrow& pr, int i) const;
  int find_pnextra();
  static int divd(int a, int b) { return (a - a % b) / b; }
  static int divu(int a, int b) 
    { return (a % b == 0) ? (a / b) : divd(a,b) + 1; }

  string call(size_t l) const;
  void init_call_vec() const;

  mutable vector<string> call_vec;
};

RINGING_END_NAMESPACE

#endif
