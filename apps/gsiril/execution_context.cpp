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
#include "execution_context.h"
#include "expression.h"
#include "parser.h"

RINGING_USING_NAMESPACE

execution_context::execution_context( const parser &pa, ostream &os ) 
  : pa(pa), os(os)
{
  if ( pa.bells() == -1 )
    throw runtime_error( "Must set number of bells before proving" ); 
  r = row(pa.bells());
}

bool execution_context::permute_and_prove_t::operator()( const change &c )
{
  bool rv = p.add_row( r *= c ); 
  ex.execute_symbol("everyrow");
  if ( r.isrounds() ) ex.execute_symbol("rounds");
  if ( !rv ) ex.execute_symbol("conflict");
  return rv;
}

bool execution_context::permute_and_prove_t::operator()( const row &c )
{
  bool rv = p.add_row( r *= c ); 
  ex.execute_symbol("everyrow");
  if ( r.isrounds() ) ex.execute_symbol("rounds");
  if ( !rv ) ex.execute_symbol("conflict");
  return rv;
}

execution_context::permute_and_prove_t::
permute_and_prove_t( row &r, prover &p, execution_context &ex ) 
  : r(r), p(p), ex(ex)
{
}

execution_context::permute_and_prove_t 
execution_context::permute_and_prove()
{
  return permute_and_prove_t( r, p, *this );
}

void execution_context::execute_symbol( const string &sym ) 
{
  pa.lookup_symbol( sym ).execute( *this );
}

string execution_context::final_symbol() const
{
  if ( p.truth() && r.isrounds() ) 
    return "true";
  else if ( p.truth() )
    return "notround";
  else
    return "false";
}

string execution_context::substitute_string( const string &str, bool &do_exit )
{
  make_string os;

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
      case '#' : os << p.size();        break;
      default:   os << *i;              break;
      }

  return os;
}
