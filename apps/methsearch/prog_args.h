// -*- C++ -*- prog_args.h - program arguments
// Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

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

#ifndef METHSEARCH_ARGS_INCLUDED
#define METHSEARCH_ARGS_INCLUDED

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include "format.h"
#include <string>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif
#include <ringing/row.h>

class arg_parser;

RINGING_USING_NAMESPACE
RINGING_USING_STD

template <class Type, Type Init>
class init_val
{
public:
  init_val() : val( Init ) {}
  init_val( const Type &val ) : val( val ) {}

  operator Type() const { return val; }
  init_val &operator=( const Type &x ) { val = x; return *this; }
  
  const Type &get() const { return val; }
  Type &get() { return val; }

private:
  Type val;
};

struct arguments
{
  init_val<int,-1> search_limit;

  init_val<int,0>  bells;
  init_val<int,0>  max_consec_blows;
  init_val<int,0>  max_places_per_change;
  init_val<int,0>  treble_dodges;
  init_val<int,1>  hunt_bells;
  init_val<int,0>  lead_len;
  init_val<int,0>  max_consec_places; 

  init_val<bool,true>  require_single_place_lh_le; 
  init_val<bool,false> require_limited_le; 
  init_val<bool,false> require_pbles;
  init_val<bool,false> require_cyclic_les;
  init_val<bool,false> require_cyclic_hlh;
  init_val<bool,false> require_cyclic_hle;
  init_val<bool,false> require_rev_cyclic_hlh;
  init_val<bool,false> require_rev_cyclic_hle;
  init_val<bool,false> require_offset_cyclic;
  init_val<bool,false> require_reg_hls;
  init_val<bool,false> right_place;
  init_val<bool,false> show_all_meths;

  init_val<bool,false> quiet;
  init_val<bool,false> histogram;
  init_val<bool,false> status;
  init_val<bool,false> count;

  init_val<bool,false> no_78_pns;

  init_val<bool,false> skewsym;
  init_val<bool,false> sym;
  init_val<bool,false> doubsym;
  init_val<bool,false> surprise;
  init_val<bool,false> treble_bob;

  init_val<bool,false> same_place_parity;
  init_val<bool,true>  true_trivial;
  init_val<bool,true>  true_half_lead;
  init_val<bool,true>  true_lead;
  init_val<bool,false> true_course;
  init_val<bool,false> true_extent;

  init_val<bool,false> require_CPS;

  string startmeth;

  string pn;

  string mask;

  vector< vector<change> > allowed_changes;
  vector<row>    required_rows;

  format_string H_fmt, R_fmt;

  string require_str;
  expression require_expr;

  arguments();

  void bind( arg_parser &p );
  bool validate( arg_parser &p );
};

#endif // METHSEARCH_ARGS_INCLUDED
