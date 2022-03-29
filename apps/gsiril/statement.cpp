// statement.cpp - Code to execute different types of statement
// Copyright (C) 2002, 2003, 2004, 2005, 2010, 2011, 2012, 2019, 2020, 2021,
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "statement.h"
#include "parser.h"
#include "expression.h"
#include "execution_context.h"
#include "proof_context.h"
#if RINGING_OLD_IOSTREAMS
#include <fstream.h>
#else
#include <fstream>
#endif
#include <ringing/streamutils.h>

void definition_stmt::execute( execution_context& e )
{
  if ( e.define_symbol( defn ) )
    {
      if ( e.verbose() ) {
	if ( defn.second.isnop() )
	  e.output() << "Definition of '" << defn.first << "' cleared." << endl;
	else
	  e.output() << "Redefinition of '" << defn.first << "'." << endl;
      }
    }
  else
    {
      if ( e.verbose() ) {
	if ( ! defn.second.isnop() )
	  e.output() << "Definition of '" << defn.first << "' added." << endl;
      }
    }
}

void default_defn_stmt::execute( execution_context& e )
{
  if ( e.default_define_symbol( defn ) )
    {
      if ( e.verbose() ) {
	if ( ! defn.second.isnop() )
	  e.output() << "Definition of '" << defn.first << "' added." << endl;
      }
    }
}

void immediate_defn_stmt::execute( execution_context& e )
{
  proof_context p(e);
  if ( e.define_symbol( make_pair( name, val.evaluate(p) ) ) )
    {
      if ( e.verbose() )
        e.output() << "Redefinition of '" << name << "'." << endl;
    }
  else
    {
      if ( e.verbose() )
        e.output() << "Definition of '" << name << "' added." << endl;
    }
}


void null_stmt::execute( execution_context& e )
{
  if ( e.verbose() )
    e.output() << diagnostic;
}

void prove_stmt::execute( execution_context& e )
{
  if (e.get_args().prove_one && e.done_one_proof())
    throw runtime_error( "Already done proof" );

  e.set_done_proof();
  
  if (e.get_args().determine_bells)
    cout << e.bells() << " bells" << endl;

  proof_context p(e);

  try {
    p.execute_symbol( "start" );
    expr.execute(p, +1);
    p.execute_symbol( "finish" );
  } 
  catch( const script_exception& ex ) {
    if ( ex.t == script_exception::do_abort ) {
      p.execute_final_symbol( "abort" );
      e.set_failure();
    }
    return;
  }
   
  switch ( p.state() ) {
  case proof_context::rounds: 
    if ( e.expected_length().first && 
         p.length() < e.expected_length().first ) {
      p.execute_final_symbol("tooshort");
      e.set_failure();
    }
    else {
      if ( e.get_args().quiet ) p.set_silent(true);
      p.execute_final_symbol("true"); 
    }
    break;
  case proof_context::notround:
    p.execute_final_symbol("notround");
    e.set_failure();
    break;
  case proof_context::isfalse:
    p.execute_final_symbol("false"); 
    e.set_failure();
    break;
  }
}

void extents_stmt::execute( execution_context& e )
{
  e.extents( n );

  if ( e.verbose() )
    e.output() << "Set extents to " << n << endl;
}

void bells_stmt::execute( execution_context& e )
{
  if ( bells > (int) bell::MAX_BELLS )
    throw runtime_error( make_string() 
			 << "Number of bells must be less than "
			 << bell::MAX_BELLS + 1 );
  else if ( bells <= 1 )
    throw runtime_error( "Number of bells must be greater than 1" );

  e.bells( bells );

  if ( e.verbose() )
    e.output() << "Set bells to " << bells << endl;
}

void rows_stmt::execute( execution_context& e )
{
  e.expected_length(len);

  if ( e.verbose() ) {
    e.output() << "Set expected length to ";
    if (len.first == len.second) e.output() << len.first;
    else e.output() << len.first << '-' << len.second; 
    e.output() << endl;
  }
}

void rounds_stmt::execute( execution_context& e )
{
  e.rounds( rounds );

  if ( e.verbose() )
    e.output() << "Set rounds to " << rounds << endl;
}

void row_mask_stmt::execute( execution_context& e )
{
  if (!e.bells())
    throw runtime_error
      ( "Must specify the number of bells before setting a row mask" );

  if (!e.get_args().everyrow_only) {
    pattern_node pattern(e.bells(), mask);
    e.row_mask( pattern.get_music_details() );

    if ( e.verbose() )
      e.output() << "Set row mask" << endl;
  }
}

void import_stmt::execute( execution_context& e )
{
  class restore_values {
    execution_context& e;
    bool i, v;
    int b;
    expression first;

  public:
    restore_values( execution_context& e )
      : e(e), i( e.interactive() ), v( e.verbose() ), b( e.bells() ),
        first(NULL)
    {
      if (e.defined("__first__"))
       first = e.lookup_symbol("__first__");
    }

   ~restore_values() {
      if (b > 0) e.bells(b);
      e.interactive(i);
      e.verbose(v);

      if (first.isnull())
        e.undefine_symbol("__first__");
      else
        e.define_symbol(pair<const string, expression>("__first__", first));
    }
  };

  { // Use RAII to revert the number of bells, etc. in case we throw.
    restore_values restore(e);
  
    shared_pointer<istream> in(( load_file(name) )); 
    if ( !in )
      throw runtime_error
        ( make_string() << "Unable to load resource: " << name );
        
    shared_pointer<parser> p( make_default_parser(*in, e ) );
    e.interactive(false);
    e.verbose(false);
    p->run(e, name, parser::propagate);
  }

  if ( e.verbose() )
    e.output() << "Resource \"" << name << "\" loaded" << endl;
}

echo_stmt::mode_t echo_stmt::get_mode( string const& keyword ) {
  if (keyword == "error") return error;
  else if (keyword == "warn") return warn;
  else return echo;
}

void echo_stmt::execute( execution_context& e )
{
  proof_context p(e);
  // The proof_context sets silent mode with -E or --filter.
  // We only want that with -qq.  Warnings and errors should never be silenced.
  if ( e.get_args().quiet < 2 || mode != echo ) p.set_silent( false );

  bool no_nl = false;
  string str( expr.string_evaluate(p, &no_nl) );

  ostream& os = mode == echo ? e.output() : cerr;
  os << str;
  if (!no_nl) os << '\n';

  if ( mode == error ) {
    if ( e.interactive() ) e.set_failure();
    else exit(1);
  }
}

void compound_stmt::execute( execution_context& ex ) {
  for ( vector<statement>::iterator i=stmts.begin(), e=stmts.end(); 
          i != e; ++i )
    i->execute(ex);
}

void if_stmt::push( expression const& cond, statement const& stmt ) {
  alts.push_back( make_pair(cond, stmt) );
}

void if_stmt::execute( execution_context& ex ) {
  for ( vector< pair< expression, statement > >::iterator 
          i = alts.begin(), e = alts. end(); i != e; ++i )
    if ( ex.evaluate_bool_const(i->first) ) {
      i->second.execute(ex);
      return;
    }
}
