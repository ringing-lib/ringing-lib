// execution_context.cpp - Global environment
// Copyright (C) 2002, 2003, 2004, 2007 Richard Smith <richard@ex-parrot.com>

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
#include "expr_base.h" // Must be before execution_context.h because 
                       // of bug in MSVC 6.0
#include "expression.h"
#include "statement.h"
#include "execution_context.h"

#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE


int execution_context::bells( int b ) 
{
  if ( b > args.rounds.bells() )
    args.rounds *= row(b);
  else 
    {
      if ( !args.rounds.isrounds() )
	output() << "Warning: Rounds reset";
      args.rounds = row(b);
    }

  swap( b, args.bells.get() );
  return b;
}

execution_context::execution_context( ostream& os, const arguments& args )
  : args(args), os(&os)
{
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

void execution_context::prove_symbol( const string& sym )
{
  expression e( new symbol_node(sym) );
  statement s( new prove_stmt(e) );
  s.execute( *this );
}
