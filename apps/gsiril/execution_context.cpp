// execution_context.cpp - Environment to evaluate expressions
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
#pragma implementation
#endif

#include <ringing/streamutils.h>
#include "expression.h" // Must be before execution_context.h because 
                        // of bug in MSVC 6.0
#include "execution_context.h"
#include "common_expr.h"

RINGING_USING_NAMESPACE


expression symbol_table::lookup( const string& sym ) const
{
  sym_table_t::const_iterator i = sym_table.find(sym);
  if ( i == sym_table.end() )
    return expression( NULL );
  return i->second;
}

bool symbol_table::define( const pair<const string, expression>& defn )
{
  sym_table_t::iterator i = sym_table.find( defn.first );

  // Is it a redefinition?
  if ( i != sym_table.end() )
    {
      i->second = defn.second;
      return true;
    }
  else
    {
      sym_table.insert( defn );
      return false;
    }
}

void symbol_table::undefine( const string& sym )
{
  sym_table.erase(sym);
}


execution_context::execution_context( ostream& os, const arguments& args )
  : args(args), os(&os)
{
}

execution_context::~execution_context() 
{
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

void execution_context::prove_symbol( const string& sym )
{
  expression e( new symbol_node(sym) );
  statement s( new prove_stmt(e) );
  s.execute( *this );
}

proof_context::proof_context( const execution_context &ectx ) 
  : ectx(ectx), p( ectx.get_args().num_extents ), 
    silent( ectx.get_args().everyrow_only )
{
  if ( ectx.bells() == -1 )
    throw runtime_error( "Must set number of bells before proving" ); 
  r = row(ectx.bells());
}

proof_context::~proof_context()
{
  // MSVC 7.1 does not line-buffer std::cout (there is no requirement
  // for it to).
  ectx.output().flush();
}

void proof_context::output_string( const string& str )
{
  bool do_exit( false );
  std::string o( substitute_string(str, do_exit) );
  if ( !silent )
    ectx.output() << o;
  if (do_exit)
    throw script_exception( script_exception::do_abort );
}

void proof_context::execute_everyrow()
{
  // Temporarily disable silent flag if running with -E
  bool s = silent;
  if ( ectx.get_args().everyrow_only ) silent = false;
  execute_symbol("everyrow");
  silent = s;
}

bool proof_context::permute_and_prove_t::operator()( const change &c )
{
  bool rv = p.add_row( r *= c ); 
  pctx.execute_everyrow();
  if ( r.isrounds() ) pctx.execute_symbol("rounds");
  if ( !rv ) pctx.execute_symbol("conflict");
  return rv;
}

bool proof_context::permute_and_prove_t::operator()( const row &c )
{
  bool rv = p.add_row( r *= c ); 
  pctx.execute_symbol("everyrow");
  if ( r.isrounds() ) pctx.execute_symbol("rounds");
  if ( !rv ) pctx.execute_symbol("conflict");
  return rv;
}

proof_context::permute_and_prove_t::
permute_and_prove_t( row &r, prover &p, proof_context &pctx ) 
  : r(r), p(p), pctx(pctx)
{
}

proof_context::permute_and_prove_t 
proof_context::permute_and_prove()
{
  return permute_and_prove_t( r, p, *this );
}

void proof_context::execute_symbol( const string &sym ) 
{
  expression e( dsym_table.lookup(sym) );
  if ( e.isnull() ) e = ectx.lookup_symbol(sym);
  e.execute( *this );
}

void proof_context::define_symbol( const pair<const string, expression>& defn )
{
  dsym_table.define(defn);
}

proof_context::proof_state proof_context::state() const
{
  if ( p.truth() && r.isrounds() ) 
    return rounds;
  else if ( p.truth() )
    return notround;
  else
    return isfalse;
}

string proof_context::substitute_string( const string &str, bool &do_exit )
{
  make_string os;
  bool nl = true;

  for ( string::const_iterator i( str.begin() ), e( str.end() ); i != e; ++i )
    switch (*i)
      {
      case '@':
	os << r;
	break;
      case '$': 
	if ( i+1 == e || i[1] != '$' )
	  os << p.duplicates();
	else
	  ++i, do_exit = true;
	break;
      case '#':
	os << p.size();
	break;
      case '\\':
	if (i+1 == e) 
	  nl = false;
	else if (i[1] == 'n') 
	  ++i, os << '\n';
	else if (i[1] == 't') 
	  ++i, os << '\t';
	else if (i[1] == '\'' )
	  ++i, os << '"';
	else
	  os << *++i;
	break;
      default:
	os << *i;
	break;
      }

  if (nl) 
    os << '\n';
  return os;
}
