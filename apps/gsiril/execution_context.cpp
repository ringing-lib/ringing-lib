// execution_context.cpp - Environment to evaluate expressions
// Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include <ringing/streamutils.h>
#include "expression.h" // Must be before execution_context.h because 
                        // of bug in MSVC 6.0
#include "execution_context.h"

RINGING_USING_NAMESPACE


execution_context::execution_context( ostream& os, bool interactive )
  : intrv( interactive ), b(-1), os(&os)
{
}

execution_context::~execution_context() 
{
}

expression execution_context::lookup_symbol( const string& sym ) const
{
  sym_table_t::const_iterator i = sym_table.find(sym);
  if ( i == sym_table.end() )
    throw runtime_error( make_string() << "Unknown symbol: " << sym );
  return i->second;
}

bool execution_context::define_symbol( const pair< const string, expression > &defn )
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

  return sym_table.insert( defn ).second;
}

proof_context::proof_context( const execution_context &ectx ) 
  : ectx(ectx)
{
  if ( ectx.bells() == -1 )
    throw runtime_error( "Must set number of bells before proving" ); 
  r = row(ectx.bells());
}

bool proof_context::permute_and_prove_t::operator()( const change &c )
{
  bool rv = p.add_row( r *= c ); 
  ex.execute_symbol("everyrow");
  if ( r.isrounds() ) ex.execute_symbol("rounds");
  if ( !rv ) ex.execute_symbol("conflict");
  return rv;
}

bool proof_context::permute_and_prove_t::operator()( const row &c )
{
  bool rv = p.add_row( r *= c ); 
  ex.execute_symbol("everyrow");
  if ( r.isrounds() ) ex.execute_symbol("rounds");
  if ( !rv ) ex.execute_symbol("conflict");
  return rv;
}

proof_context::permute_and_prove_t::
permute_and_prove_t( row &r, prover &p, proof_context &ex ) 
  : r(r), p(p), ex(ex)
{
}

proof_context::permute_and_prove_t 
proof_context::permute_and_prove()
{
  return permute_and_prove_t( r, p, *this );
}

void proof_context::execute_symbol( const string &sym ) 
{
  ectx.lookup_symbol( sym ).execute( *this );
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
	else
	  os << '\\';
	break;
      default:
	os << *i;
	break;
      }

  if (nl) 
    os << '\n';
  return os;
}
