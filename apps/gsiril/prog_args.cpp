// prog_args.cpp - handle program arguments
// Copyright (C) 2002, 2003, 2004, 2007, 2008, 2010, 2011, 2012, 2014
// Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation "gsiril/prog_args.h"
#endif

#include <ringing/streamutils.h>
#include "args.h"
#include "stringutils.h"
#include "prog_args.h"

RINGING_USING_NAMESPACE


arguments::arguments( int argc, char** argv ) 
  : lead_symbol( "m" ),
    lh_symbol( "lh" )
{
  arg_parser ap( argv[0], "gsiril -- proves touches.",
		 "OPTIONS" );

  // If the program is called msiril or microsiril, go into microsiril
  // compatibility mode.  Do this first so that command line arguments
  // can override them.
  if ( ap.program_name() ==  "msiril" || ap.program_name() ==  "microsiril" ||
       ap.program_name() == "gmsiril" || ap.program_name() == "gmicrosiril" )
    set_msiril_compatible();
  else if ( ap.program_name() == "sirilic" || ap.program_name() == "gsirilic" )
    set_sirilic_compatible();
  
  bind(ap);
  if ( !ap.parse(argc, argv) ) {
    ap.usage();
    exit(3);
  }

  if ( !validate(ap) )
    exit(3);
}

void arguments::set_msiril_compatible()
{
  msiril_syntax = true;
  case_insensitive = true;
  prove_symbol = "__first__";
}

void arguments::set_sirilic_compatible()
{
  set_msiril_compatible();
  sirilic_syntax = true;
}

class function_opt : public option {
public:
  function_opt( char c, const string &l, const string &d,
	        arguments& args, void (arguments::*fn)() ) 
    : option(c, l, d), args(args), fn(fn)
  {}
  
  virtual bool process( const string&, const arg_parser& ) const {
    (args.*fn)();
    return true;
  }

private:
  arguments& args;
  void (arguments::*fn)();
};

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt
	 ( 'b', "bells",
	   "The default number of bells.",  "BELLS",
	   bells ) );

  p.add( new integer_opt
	 ( 'n', "extents",
	   "The number of extents required.",  "NUM",
	   num_extents ) );

  p.add( new row_opt
	 ( 'r', "rounds",
	   "The starting 'rounds'.",  "ROW",
	   rounds ) );

  p.add( new boolean_opt
	 ( 'i', "interactive",
	   "Run in interactive mode", 
	   interactive ) );

  p.add( new boolean_opt
	 ( 'v', "verbose",
	   "Run in verbose mode", 
	   verbose ) );

  p.add( new repeated_boolean_opt
	 ( 'q', "quiet",
	   "Do not give truth output; or if given twice, no output at all", 
	   quiet ) );

  p.add( new integer_opt
         ( '\0', "length", 
           "Require the touch to be of the specified length", "NUM",
           expected_length ) ); 

  p.add( new boolean_opt
	 ( 'E', "everyrow-only",
	   "Only print output from the everyrow symbol",
	   everyrow_only ) );

  p.add( new boolean_opt
	 ( 'I', "case-insensitive",
	   "Run case insensitively ", 
	   case_insensitive ) );

  // NB __first__ is an alias for the first symbol
  p.add( new string_opt
	 ( 'P', "prove",
	   "Proves a particular symbol (or the first if none specified)",
	   "SYMBOL",
	   prove_symbol, "__first__" ) );

  p.add( new strings_opt
	 ( 'D', "define",
	   "Define a particular symbol",
	   "NAME=VALUE",
	   definitions ) );
 
  p.add( new strings_opt
	 ( 'm', "module",
	   "Import the given module",
	   "MODULE",
	   import_modules ) );

  p.add( new function_opt
	 ( '\0', "msiril",
	   "Run in microsiril compatibile mode",
	   *this, &arguments::set_msiril_compatible ) );

  p.add( new function_opt
         ( '\0', "sirilic",
           "Run in sirilic compatibile mode",
           *this, &arguments::set_sirilic_compatible ) );

  p.add( new string_opt
         ( 'e', "expression",
           "Execute EXPR", "EXPR", 
           expression ) );

  p.add( new string_opt
         ( 'f', "script-file", 
           "Execute file FILENAME", "FILENAME",
           filename ) );

  // This is used in conjunction with -D and -P when the whole work
  // is done at initialistion time, and no -e or -f is given.
  p.add( new boolean_opt
         ( 'N', "no-read",
           "Don't read from standard input", 
           no_read ) );

  p.add( new boolean_opt
         ( '\0', "disable-import", 
           "Disable the import directive",
           disable_import ) );

  p.add( new integer_opt
         ( '\0', "node-limit", 
           "Limit prover to some number of nodes.", "NUM",
           node_limit ) );

  p.add( new boolean_opt
         ( '\0', "determine-bells", 
           "Determine the number of bells for each proof without proving.",
           determine_bells ) );

  p.add( new boolean_opt
         ( '\0', "prove-one", 
           "Prove one composition only.", 
           prove_one ) );

  p.add( new boolean_opt
         ( '\0', "filter",
           "Run as a filter on a method library or stream",
           filter ) );

  p.add( new string_opt
         ( '\0', "lead-symbol",
           "Assign lead place-notation (excluding l.h.) to SYM; default 'm'",
           "SYM", lead_symbol ) );

  p.add( new boolean_opt
         ( '\0', "lead-includes-lh", 
           "Assign the whole lead (including l.h.) to the lead symbol",
           lead_includes_lh ) );

  p.add( new string_opt
         ( '\0', "lh-symbol", 
           "Assign lead end change to SYM; default 'lh'",
           "SYM", lh_symbol ) );

  p.add( new boolean_opt
         ( '\0', "show-lead-heads", 
           "List the lead heads for the specified methods",
           show_lead_heads ) );

  p.add( new strings_opt
	 ( '\0', "methods",
	   "Mark the specified symbols as methods",
	   "SYM,SYM,...",
	   methods ) );
 }

bool arguments::validate( arg_parser& ap )
{
  if ( bells != 0 && ( bells < 3 || bells > int(bell::MAX_BELLS) ) )
    {
      ap.error( make_string() << "The number of bells must be between 3 and " 
		<< bell::MAX_BELLS << " (inclusive)" );
      return false;
    }

  if ( num_extents < 1 )
    {
      ap.error( "The number of extents must be at least one" );
      return false;
    }

  if ( bells == 0 && rounds.bells() )
    {
      ap.error( "Must specify the number of bells if rounds is used" );
      return false;
    }

  if ( rounds.bells() && rounds.bells() != bells )
    {
      ap.error( "Rounds is on the wrong number of bells" );
      return false;
    }

  if ( !expression.empty() && !filename.empty() )
    {
      ap.error( "Only one of -e and -f may be used" );
      return false;
    }

  if ( filter && expression.empty() && filename.empty() )
    {
      ap.error( "When running in filter mode, either -e or -f is required");
      return false;
    }

  if ( filter && bells == 0 )
    {
      ap.error( "When running in filter mode, "
                "the number of bells is required" );
      return false;
    }

  return true;
}


