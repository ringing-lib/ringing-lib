// main.cpp - Entry point for gsiril
// Copyright (C) 2002, 2003, 2004, 2007, 2008
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

#include <ringing/pointers.h>
#include <ringing/streamutils.h>
#include <ringing/library.h>
#include <ringing/litelib.h>
#include <ringing/method.h>
#include "console_stream.h"
#include "parser.h"
#include "execution_context.h"
#include "args.h"
#include "stringutils.h"
#include "expr_base.h"
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
    RINGING_ISTRINGSTREAM in(init_string);

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
      RINGING_ISTRINGSTREAM in(*i);
      
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
      // We want to suppress a warning about the prove symbol not being
      // defined only when it is __first__: the logic being that if it
      // has been explicitly set, it should be there; but if it is 
      // implicitly set (e.g. by invoking as 'msiril'), we shouldn't give
      // an error.
      if ( args.prove_symbol != "__first__" || e.defined("__first__") )
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

bool prove_stream( execution_context& e, istream& in, const arguments& args )
{
  e.set_failure(false); // unset failure state

  bool read_anything 
    = parse_all( e, make_default_parser(in, args), 
                 "Error", !args.interactive );

  if ( read_anything && args.prove_symbol.size() )
    // Return directly from here:  this means that with -P and additional
    // prove statements are allowed to fail.
    return prove_final_symbol( e, args );
      
  // Failure will be set if any prove statement has failed 
  return !e.failure();
}

bool run( execution_context& e, const arguments& args )
{
  if (args.interactive) welcome();

  scoped_pointer<istream> in;

  if ( !args.expression.empty() ) {
    in.reset( new RINGING_ISTRINGSTREAM( args.expression ) );
  }
  else if ( !args.filename.empty() ) {
    in.reset( new fstream( args.filename.c_str() ) );
    if ( !*in ) { 
      cerr << "Error opening file: " << args.filename << "\n";
      exit(1);
    }
  }
  else {
    in.reset( new console_istream( args.interactive ) );
    static_cast<console_istream*>( in.get() )->set_prompt( "> " );
  }

  return prove_stream( e, *in, args );
}

void filter( execution_context& e, const arguments& args )
{
  litelib in( args.bells, cin );
  for ( library::const_iterator i=in.begin(), ei=in.end(); i!=ei; ++i )
    {
      method m;
      try {
        m = i->meth();
      }
      catch ( exception const& ex ) {
        cerr << "Error reading method from input stream: "
             << ex.what() << "\n";
        string pn;  try { pn = i->pn(); } catch (...) {}
        if ( pn.size() ) cerr << "Place notation: '" << pn << "'\n";
        cerr << flush;

        continue;
      }

      if ( m.size() == 0 ) {
        cerr << "Error: empty method found\n";
        continue;
      }

      // Define the lead head and lead
      if ( args.lh_symbol.size() )
        e.define_symbol
          ( make_pair( args.lh_symbol, 
                       expression( new pn_node( m.back() ) ) ) );

      m.pop_back();
      if ( args.lead_symbol.size() )
        e.define_symbol
          ( make_pair( args.lead_symbol, 
                       expression( new pn_node( m ) ) ) );

      if ( run( e, args ) ) 
        cout << i->meth().format( method::M_DASH | method::M_SYMMETRY )
             << endl;
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

      if ( args.filter )
        filter( e, args );
      else
        if ( !run( e, args ) )
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

