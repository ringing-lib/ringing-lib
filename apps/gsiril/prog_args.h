// -*- C++ -*- prog_args.h - program arguments for gsiril
// Copyright (C) 2003, 2004, 2007, 2008, 2010, 2011, 2012, 2014, 2019, 2020,
// 2021 Richard Smith <richard@ex-parrot.com>

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


#ifndef GSIRIL_ARGS_INCLUDED
#define GSIRIL_ARGS_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface "gsiril/prog_args.h"
#endif

#include <ringing/row.h>
#include <ringing/methodset.h>
#include "init_val.h"
#include "bell_fmt.h"
#include <string>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif

class arg_parser;

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  init_val<int,0>      bells; // the default number of bells
  init_val<int,1>      num_extents;
  
  init_val<bool,false> interactive;
  init_val<bool,false> verbose;
  init_val<int, 0>     quiet;
  init_val<bool,false> case_insensitive;
  init_val<bool,false> msiril_syntax;
  init_val<bool,false> sirilic_syntax;

  pair<size_t, size_t> expected_length;
  init_val<bool,false> everyrow_only;
  string               row_mask;

  init_val<bool,false> filter;

  vector<string>       import_modules;
  vector<string>       definitions;

  string               filename;
  string               expression;
  init_val<bool,false> no_read;
  init_val<bool,false> disable_import;
  init_val<bool,false> no_init_file;
  init_val<int,0>      node_limit;
  init_val<bool,false> determine_bells;

  string               prove_symbol;
  init_val<bool,false> prove_one;
  string               lead_symbol;
  string               lh_symbol;
  string               payload_symbol;
  init_val<bool,false> lead_includes_lh;

  init_val<bool,false> trace_all_symbols;
  vector<string>       trace_symbols;

  row                  rounds;

  string               rstr, gstr, bstr;
  bell_fmt             bellfmt;

  vector<string>       libnames;

  arguments( int argc, char** argv );

  methodset const&     methset() const;

private:
  mutable shared_pointer<methodset> the_methset;

  void set_msiril_compatible();
  void set_sirilic_compatible();

  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

#endif // GSIRIL_ARGS_INCLUDED
