// execution_context.cpp - Global environment
// Copyright (C) 2002, 2003, 2004, 2007, 2011, 2012, 2019, 2020, 2021, 2022
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif
#include "expr_base.h" // Must be before execution_context.h because 
                       // of bug in MSVC 6.0
#include "expression.h"
#include "execution_context.h"
#include "proof_context.h"

#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE


int execution_context::bells( int b ) 
{
  if ( b > args.rounds.bells() )
    args.rounds *= row(b);
  else if ( b < args.rounds.bells() )
    {
      if ( !args.rounds.isrounds() )
	output() << "Warning: Rounds reset";
      args.rounds = row(b);
    }

  swap( b, args.bells.get() );
  define_line();
  return b;
}

void execution_context::row_mask(music_details const& m) { 
  rmask = m; 
  define_line();
}
  
void execution_context::define_line() {
  size_t len = args.bells.get();
  music row_mask( bells(), rmask );
  if ( row_mask.process_row( row( bells() ) ) )
    len = row_mask.begin()->last_wildcard_matches().size();

  // Define __line__ to be a line of hyphens, one per displayed bell.
  // The expectation is that users will do something like: line = __line__
  sym_table.define
    ( pair<const string, expression>( "__line__", 
        expression( new string_node( string(len, '-') ) ) ) );
}
  
pair<size_t,size_t> 
execution_context::expected_length( pair<size_t, size_t> l )
{ 
  // Define min_length and max_length so they can be referenced by 
  // tooshort and toolong, respectively.
  if (l.first && l.second) {
    sym_table.define
      ( pair<const string, expression>( "min_length", 
          expression( new integer_node(l.first) ) ) );
    sym_table.define
      ( pair<const string, expression>( "max_length", 
          expression( new integer_node(l.second) ) ) );
  }
  else {
    sym_table.undefine("min_length");
    sym_table.undefine("max_length");
  }

  swap( l, args.expected_length ); 
  return l;
}

execution_context::execution_context( ostream& os, const arguments& a )
  : args(a), rmask(a.row_mask.length() ? a.row_mask : "*"), os(&os), 
    failed(false), node_count(0), done_proof(false)
{
  if ( args.bells ) {
    if ( !args.rounds.bells() )
      args.rounds = row(args.bells);
    define_line();
  }
  expected_length(args.expected_length);
}

execution_context::~execution_context() 
{
}

bool execution_context::defined( const string& sym ) const
{
  return !sym_table.lookup(sym).isnull();
}

expression execution_context::lookup_symbol( const string& sym ) const
{
  expression e( sym_table.lookup(sym) );
  if ( e.isnull() )
    throw runtime_error( make_string() << "Unknown symbol: " << sym );
  return e;
}

void execution_context::undefine_symbol( const string& sym )
{
  sym_table.undefine(sym);
}

bool execution_context::default_define_symbol( const pair< const string, expression > &defn )
{
  if ( ! sym_table.lookup(defn.first).isnop() ) 
    return false;

  sym_table.define(defn);

  if ( sym_table.lookup( "__first__" ).isnull() )
    sym_table.define
      ( pair<const string, expression>( "__first__", defn.second ) );
  return true;
}

bool execution_context::define_symbol( const pair< const string, expression > &defn )
{
  if ( sym_table.define(defn) )
    return true;  // redefinition can't be first

  if ( sym_table.lookup( "__first__" ).isnull() )
    sym_table.define
      ( pair<const string, expression>( "__first__", defn.second ) );
  return false;
}

void execution_context::increment_node_count() const
{
  ++node_count;
  if (++node_count == args.node_limit)
    throw runtime_error("Node count exceeded");
}

bool execution_context::evaluate_bool_const( expression const& cond ) const {
  proof_context p(*this);
  p.set_silent(true);
  return cond.bool_evaluate(p);
}

string execution_context::evaluate_string_var( string const& name ) const {
  proof_context p(*this);
  p.set_silent(true);
  return lookup_symbol(name).string_evaluate(p);
}
