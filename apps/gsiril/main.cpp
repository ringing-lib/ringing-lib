// main.cpp - Entry point for gsiril
// Copyright (C) 2002, 2003, 2004 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/pointers.h>
#include <ringing/streamutils.h>
#include "console_stream.h"
#include "parser.h"
#include "execution_context.h"
#include "args.h"
#include "util.h"
#include "expression.h"
#include "prog_args.h"
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif
#if RINGING_OLD_IOSTREAMS
#include <iostream.h>
#include <fstream.h>
#else
#include <iostream>
#include <fstream>
#endif
#if RINGING_USE_STRINGSTREAM
#if RINGING_OLD_INCLUDES
#include <sstream.h>
#else
#include <sstream>
#endif
#else // RINGING_USE_STRINGSTREAM
#if RINGING_OLD_INCLUDES
#include <strstream.h>
#else
#include <strstream>
#endif
#endif // RINGING_USE_STRINGSTREAM

RINGING_USING_NAMESPACE


arguments::arguments( int argc, char** argv ) 
{
  arg_parser ap( argv[0], "gsiril -- proves touches.",
		 "OPTIONS" );

  // If the program is called msiril or microsiril, go into microsiril
  // compatibility mode.
  if ( ap.program_name() == "msiril" || ap.program_name() == "microsiril" )
    set_msiril_compatible();
  
  bind(ap);
  if ( !ap.parse(argc, argv) ) {
    ap.usage();
    exit(1);
  }

  if ( !validate(ap) )
    exit(1);
}

void arguments::set_msiril_compatible()
{
  msiril_comments = true;
  case_insensitive = true;
  prove_symbol = "__first__";
}

class row_opt : public option {
public:
  row_opt( char c, const string &l, const string &d, const string& a,
	   row& opt )
    : option(c, l, d, a), opt(opt)
  {}

private:
  virtual bool process( const string& arg, const arg_parser& ap ) const {
    try {
      opt = arg;
    } catch ( exception const& ex ) {
      ap.error(make_string() << "Invalid row '" << arg << "': " << ex.what());
      return false;
    }
    return true;
  }
  row &opt;
};

class msiril_opt : public option {
public:
  msiril_opt( char c, const string &l, const string &d,
	      arguments& args ) 
    : option(c, l, d), args(args)
  {}
  
  virtual bool process( const string&, const arg_parser& ) const {
    args.set_msiril_compatible();
    return true;
  }

private:
  arguments& args;
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

  p.add( new msiril_opt
	 ( '\0', "msiril",
	   "Run in microsiril compatibile mode",
	   *this ) );
}

bool arguments::validate( arg_parser& ap )
{
  if ( bells != 0 && ( bells < 3 || bells >= int(bell::MAX_BELLS) ) )
    {
      ap.error( make_string() << "The number of bells must be between 3 and " 
		<< bell::MAX_BELLS-1 << " (inclusive)" );
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

  if ( rounds.bells() != bells )
    {
      ap.error( "Rounds is on the wrong number of bells" );
      return false;
    }

  return true;
}

int parse_all( execution_context& e,
	       const shared_pointer<parser>& p,
	       const string& error_prefix,
	       bool errors_are_fatal )
{
  int count(0);

  while (true)
    {
      try 
	{
	  statement s( p->parse() );
	  if ( s.eof() ) break;
	  s.execute(e);
	  ++count;
	}
      catch (const exception& ex )
	{
	  cerr << error_prefix << ": " << ex.what() << endl;
	  if (errors_are_fatal) exit(1);
	}
    }

  return count;
}

void welcome()
{
  cout << "Ringing Class Library / gsiril " RINGING_VERSION ".\n"
"Gsiril is free software covered by the GNU General Public License, and you\n"
"are welcome to change it and/or distribute copies of it under certain\n"
"conditions."  << endl;
}

shared_pointer<istream> load_file( string const& name )
{
  shared_pointer<istream> in( new ifstream(name.c_str()) );
  if ( !*in ) {
    string filename( name ); filename += ".gsir";
    in.reset( new ifstream(filename.c_str()) );
  }
  if ( !*in ) {
    string filename( name ); filename += ".sir";
    in.reset( new ifstream(filename.c_str()) );
  }
  if ( !*in )
    in.reset();
  return in;
}

void initialise( execution_context& ex, const arguments& args )
{
  string init_string = 
"true     = \"# rows ending in @\", \"Touch is true\"\n"
"notround = \"# rows ending in @\", \"Is this OK?\"\n"
"false    = \"# rows ending in @\", \"Touch is false in $ rows\"\n"
"conflict = \"# rows ending in @\", \"Touch not completed due to false row$$\"\n"
"rounds   = \n"
"start    = \n"
"finish   = \n"
"abort    = \n";

  // The 'everyrow' symbol is defined to "@" if -E is specified.
  if ( args.everyrow_only )
    init_string += "everyrow = \"@\"\n";
  else
    init_string += "everyrow = \n";

  // Turn off interactivity whilst it prepopulates the symbol table
  bool interactive = ex.interactive(false);
  bool verbose     = ex.verbose(false);

  // Prepopulate symbol table
  {
#if RINGING_USE_STRINGSTREAM
    istringstream in(init_string);
#else
    istrstream in(init_string);
#endif

    parse_all(ex, make_default_parser(in, args),
	      "Error initialising", true);
  }

  // Import any required modules
  for ( vector< string >::const_iterator 
	  i(args.import_modules.begin()), e( args.import_modules.end());
	i != e; ++i ) 
    {
      shared_pointer<istream> in( load_file(*i) );

      if ( !in )
	throw runtime_error
	  ( make_string() << "Unable to find module: " << *i );

      parse_all(ex, make_default_parser(*in, args),
		make_string() << "Error loading module '" << *i << "'", true);
    }
    
  for ( vector< string >::const_iterator 
	  i(args.definitions.begin()), e( args.definitions.end());
	i != e; ++i ) 
    {
#if RINGING_USE_STRINGSTREAM
      istringstream in(*i);
#else
      istrstream in(*i);
#endif
      
      parse_all(ex, make_default_parser(in, args),
		make_string() << "Error parsing definition '" << *i << "'", 
		true);
    }

  ex.interactive(interactive);
  ex.verbose(verbose);

  // Don't allow any of the predefined symbols to polute 'first'.
  ex.undefine_symbol( "__first__" );
}

bool prove_final_symbol( execution_context& e, const arguments& args )
{
  try 
    {
      e.prove_symbol( args.prove_symbol );
      return true;
    } 
  catch (const exception& ex ) 
    {
      cerr << "Error proving " << args.prove_symbol << ": "
	   << ex.what() << endl;
      return false;
    }
}

int main( int argc, char *argv[] )
{
  try
    {
      arguments args( argc, argv );

      if ( args.case_insensitive ) 
	args.prove_symbol = lower_case( args.prove_symbol );

      execution_context e( cout, args );
      initialise(e, args);

      if (args.interactive) welcome();

      console_istream cin( args.interactive );
      cin.set_prompt( "> " );

      bool read_anything 
	= parse_all(e, make_default_parser(cin, args), 
		    "Error", !args.interactive);

      if ( read_anything && args.prove_symbol.size() )  
	if ( !prove_final_symbol( e, args ) )
	  exit(1);
    }
  catch ( const exception &ex )
    {
      cerr << "Unexpected error: " << ex.what() << endl;
      exit(1);
    }
  catch ( ... )
    {
      cerr << "An unknown error occured" << endl;
      exit(1);
    }

  return 0;
}

