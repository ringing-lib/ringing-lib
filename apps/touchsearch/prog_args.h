// -*- C++ -*- prog_args.h - program arguments for touchsearch
// Copyright (C) 2007, 2009, 2010, 2025 Richard Smith <richard@ex-parrot.com>

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

#ifndef TOUCHSEARCH_PROG_ARGS_INCLUDED
#define TOUCHSEARCH_PROG_ARGS_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface "gsiril/prog_args.h"
#endif

#include <ringing/change.h>
#include <ringing/method.h>
#include <ringing/group.h>
#include "init_val.h"
#include <string>
#include <vector>      

class arg_parser;

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  init_val<int,0>      bells; // the default number of bells

  pair<size_t,size_t>  length;
  init_val<int,1>      extents;
  init_val<bool,false> ignore_rotations;
  init_val<bool,false> mutually_true_parts;
  
  init_val<int,-1>     search_limit;
  init_val<bool,false> filter_mode;
  init_val<bool,false> quiet;
  init_val<bool,false> count;
  init_val<bool,false> raw_count;
  init_val<bool,false> comma_separate;
  init_val<bool,false> use_plan;
  init_val<bool,true>  round_blocks;

  string               plain_name;
  string               meth_str;
  method               meth;

  vector<string>       call_strs;
  vector<change>       calls;

  vector<string>       pend_strs;
  group                pends;

  arguments( int argc, char** argv );

private:
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
  bool generate_calls( arg_parser& ap );
  bool generate_pends( arg_parser& ap );
};

// TODO:  This doesn't belong here!
pair<size_t, size_t> range_div( pair<size_t, size_t> const& r, size_t n );


#endif // TOUCHSEARCH_PROG_ARGS_INCLUDED
