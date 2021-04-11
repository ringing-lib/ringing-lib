// execution_context.cpp - Global environment
// Copyright (C) 2002, 2003, 2004, 2007, 2011, 2012, 2019, 2020, 2021
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

  // Define __line__ to be a line of hyphens, one per bell, with no 
  // terminating line break.  The expectation is that users will do
  // something like:   line = __line__, "";
  sym_table.define
    ( pair<const string, expression>( "__line__", 
        expression( new string_node( string(b, '-') + '\\' ) ) ) );

  swap( b, args.bells.get() );
  return b;
}
  
pair<size_t,size_t> 
execution_context::expected_length( pair<size_t, size_t> l )
{ 
  // Define min_length and max_length so they can be referenced by 
  // tooshort and toolong, respectively.
  if (l.first && l.second) {
    sym_table.define
      ( pair<const string, expression>( "min_length", 
          expression( new string_node(make_string() << l.first) ) ) );
    sym_table.define
      ( pair<const string, expression>( "max_length", 
          expression( new string_node(make_string() << l.second) ) ) );
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
  if ( !args.rounds.bells() )
    args.rounds = row(args.bells);
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
  if (args.node_limit && ++node_count == args.node_limit)
    throw runtime_error("Node count exceeded");
}

bool execution_context::evaluate_bool_const( expression const& cond ) const {
  proof_context p(*this);
  p.set_silent(true);
  return cond.bool_evaluate(p);
}

void execution_context::push_if(expression const& cond) {
  // The if_stack contains 1 if we're executing the current branch,
  // 0 if we've not executed any branch yet, and 2 if we've executed a previous
  // branch.
  if_stack.push_back( evaluate_bool_const(cond) ? 1 : 0 );
}

void execution_context::chain_else_if(expression const& cond) {
  if (if_stack.empty())
    throw runtime_error("Unexpected elseif statement");
  if (if_stack.back() == 0)
    if_stack.back() = evaluate_bool_const(cond) ? 1 : 0;
  else 
    if_stack.back() = 2;
}

void execution_context::chain_else() {
  if (if_stack.empty())
    throw runtime_error("Unexpected else statement");
  if (if_stack.back() == 0)
    if_stack.back() = 1;
  else 
    if_stack.back() = 2;
}

void execution_context::pop_if() {
  if (if_stack.empty())
    throw runtime_error("Unexpected endif statement");
  if_stack.pop_back();
}

bool execution_context::is_executing() const {
  return if_stack.empty() || if_stack.back() == 1;
}
