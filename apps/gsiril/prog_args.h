// -*- C++ -*- prog_args.h - program arguments for gsiril
// Copyright (C) 2003, 2004, 2007, 2008 Richard Smith <richard@ex-parrot.com>

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


#ifndef GSIRIL_ARGS_INCLUDED
#define GSIRIL_ARGS_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface "gsiril/prog_args.h"
#endif

#include <ringing/row.h>
#include "init_val.h"
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
  init_val<bool,false> case_insensitive;
  init_val<bool,false> msiril_syntax;
  init_val<bool,false> sirilic_syntax;

  init_val<bool,false> everyrow_only;

  init_val<bool,false> filter;

  vector<string>       import_modules;
  vector<string>       definitions;

  string               filename;
  string               expression;

  string               prove_symbol;
  string               lead_symbol;
  string               lh_symbol;

  row                  rounds;

  arguments( int argc, char** argv );

private:
  void set_msiril_compatible();
  void set_sirilic_compatible();

  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

#endif // GSIRIL_ARGS_INCLUDED
