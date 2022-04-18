// -*- C++ -*- prog_args.h - program arguments
// Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009, 2010, 2011, 2020, 2021,
// 2022 Richard Smith <richard@ex-parrot.com>

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

#ifndef METHSEARCH_ARGS_INCLUDED
#define METHSEARCH_ARGS_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include "init_val.h"
#include <string>
#if RINGING_OLD_INCLUDES
#include <set.h>
#include <vector.h>
#else
#include <set>
#include <vector>
#endif
#if RINGING_OLD_INCLUDES
#include <iosfwd.h>
#else
#include <iosfwd>
#endif
#include <ringing/row.h>
#include <ringing/pointers.h>
#include <ringing/libout.h>
#include <ringing/method.h>
#include <ringing/group.h>
#include <ringing/music.h>

class arg_parser;

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  init_val<int,-1>     search_limit;
  init_val<bool,false> random_order;
  init_val<int,0>      random_count; // 0 disables, -1 is infinite
  init_val<int,-1>     random_seed;

  init_val<int,0>  bells;
  init_val<int,0>  max_consec_blows;
  init_val<bool,false> long_le_place;
  init_val<int,0>  max_places_per_change;
  init_val<int,0>  treble_dodges;
  init_val<int,1>  hunt_bells;
  init_val<int,0>  orig_lead_len, lead_len;
  init_val<int,0>  max_consec_places;

  init_val<int,-1> treble_front;
  init_val<int,-1> treble_back;
  string           treble_path;

  init_val<bool,false> require_limited_le;
  init_val<bool,false> prefer_limited_le;
  init_val<bool,false> require_pbles;
  init_val<bool,false> any_regular_le;
  init_val<bool,false> require_cyclic_les;
  init_val<bool,false> require_cyclic_hlh;
  init_val<bool,false> require_cyclic_hle;
  init_val<bool,false> require_rev_cyclic_hlh;
  init_val<bool,false> require_rev_cyclic_hle;
  init_val<bool,false> require_offset_cyclic;
  init_val<bool,false> require_reg_hls;
  init_val<bool,false> right_place;
  init_val<bool,false> no_points;
  init_val<bool,false> has_points;
  init_val<bool,false> no_unpaired_points;
  init_val<bool,false> has_unpaired_points;
  init_val<bool,false> show_all_meths;

  init_val<bool,false> quiet;
  init_val<bool,false> histogram;
  init_val<bool,false> status;
  init_val<int, 0>     status_freq;
  init_val<bool,false> count;
  init_val<bool,false> raw_count;
  init_val<bool,false> node_count;
  init_val<bool,false> filter_mode;
  init_val<bool,false> filter_lib_mode;
  init_val<bool,false> invert_filter;
  init_val<int, 0>     timeout;
  init_val<bool,false> only_named;
  init_val<bool,false> only_unnamed;

  init_val<bool,false> no_78_pns;
  init_val<bool,false> sym_sects;

  init_val<bool,false> skewsym;
  init_val<bool,false> sym;
  init_val<bool,false> doubsym;
  init_val<bool,false> mirrorsym;
  init_val<bool,false> floating_sym;
  init_val<bool,false> surprise;
  init_val<bool,false> treble_bob;
  init_val<bool,false> delight;
  init_val<bool,false> delight3;
  init_val<bool,false> delight4;
  init_val<bool,false> strict_delight;
  init_val<bool,false> exercise;
  init_val<bool,false> strict_exercise;
  init_val<bool,false> pas_alla_tria;
  init_val<bool,false> pas_alla_tessera;

  init_val<bool,false> same_place_parity;
  init_val<bool,true>  true_trivial;
  init_val<bool,false> basic_falseness_opt;
  init_val<bool,true>  true_half_lead;
  init_val<bool,true>  true_lead;
  init_val<bool,false> true_course;
  init_val<bool,false> true_extent;
  init_val<bool,false> true_positive_extent;
  init_val<int,1>      n_extents;
  string               allowed_falseness; 

  row                  start_row;
  vector<row>          pends_generators;
  group                pends;

  init_val<bool,false> require_CPS;

  string startmethstr;
  method startmeth;

  string prefixstr;
  method prefix;

  string pn;

  string mask;

  string              changes_str;
  init_val<bool,true> include_changes;
  set<change>         changes;

  vector<string>      row_matches_str;
  vector<music>       row_matches;

  vector< vector<change> > allowed_changes;

  string overwork_map_file, underwork_map_file;

  string pn_fmt;
  string H_fmt_str, R_fmt_str;
  string outfile;
  string outfmt;

  // TODO:  Isn't really part of this struct
  mutable multilibout outputs;

  vector<string> require_strs;
  vector<size_t> require_expr_idxs;

  set<row> orig_avoid_rows;
  set<row> avoid_rows;

  arguments( int argc, char* argv[] );

  bool set_bells( int b = 0 );

private:
  void bind( arg_parser &p );
  bool validate( arg_parser &p );
  bool validate_path( arg_parser *p = 0 );
  bool validate_bells( arg_parser *p = 0 );
};

#endif // METHSEARCH_ARGS_INCLUDED
