// expression.cpp - Nodes and factory function for expressions
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation "gsiril/expression.h"
#pragma implementation "gsiril/common_expr.h"
#endif

#include "expression.h"
#include "common_expr.h"
#include "parser.h"
#include "execution_context.h"
#if RINGING_OLD_IOSTREAMS
#include <fstream.h>
#else
#include <fstream>
#endif
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE


void definition_stmt::execute( execution_context& e ) const
{
  if ( e.define_symbol( defn ) )
    {
      if ( e.interactive() )
	e.output() << "Redefinition of '" << defn.first << "'." << endl;
    }
  else
    {
      if ( e.interactive() )
	e.output() << "Definition of '" << defn.first << "' added." << endl;
    }
}

void null_stmt::execute( execution_context& e ) const
{
  if ( e.interactive() )
    e.output() << diagnostic;
}

void prove_stmt::execute( execution_context& e ) const
{
  try 
    {
      proof_context p(e);
      expr.execute(p);
      switch ( p.state() )
	{
	case proof_context::rounds: 
	  p.execute_symbol( "true" ); 
	  break;
	case proof_context::notround:
	  p.execute_symbol( "notround" ); 
	  break;
	case proof_context::isfalse:
	  p.execute_symbol( "false" ); 
	  break;
	}
    }
  catch( const script_exception& ) 
    {}
}

void bells_stmt::execute( execution_context& e ) const
{
  e.bells( bells );

  if ( e.interactive() )
    e.output() << "Set bells to " << bells << endl;
}

void import_stmt::execute( execution_context& e ) const
{
  bool i( e.interactive() );
  int b( e.bells() );

  try
    {
      ifstream ifs( name.c_str() );
      if ( !ifs )
	throw runtime_error
	  ( make_string() << "Unable to load resource: " << name );
      
      shared_pointer<parser> p( make_default_parser(ifs, e.get_args() ) );
      e.interactive(false);
      while (true)
	{
	  statement s( p->parse() );
	  if ( s.eof() ) break;
	  s.execute(e);
	}
    }
  catch (...)
    {
      // Restore bells, interactive flags
      if (b > 0) e.bells(b);
      e.interactive(i);
      throw;
    }

  // Restore bells, interactive flags
  if (b > 0) e.bells(b);
  e.interactive(i);

  if ( e.interactive() )
    e.output() << "Resource \"" << name << "\" loaded" << endl;
}




void list_node::debug_print( ostream &os ) const
{
  os << "( ";
  car.debug_print( os );
  os << ", ";
  cdr.debug_print( os );
  os << " )";
}

void list_node::execute( proof_context &ctx )
{
  car.execute( ctx );
  cdr.execute( ctx );
}

void nop_node::debug_print( ostream &os ) const
{
  os << "{null}";
}

void nop_node::execute( proof_context & )
{
}

void repeated_node::debug_print( ostream &os ) const
{
  os << count << " ";
  child.debug_print( os );
}

void repeated_node::execute( proof_context &ctx )
{
  for (int i=0; i<count; ++i) 
    child.execute( ctx );
}

void string_node::execute( proof_context &ctx )
{
  bool do_exit = false;
  ctx.output() << ctx.substitute_string(str, do_exit);
  if (do_exit)
    throw script_exception();
}

void string_node::debug_print( ostream &os ) const
{
  os << "\"" << str << "\"";
}

pn_node::pn_node( int bells, const string &pn )
{
  if ( bells <= 0 )
    throw runtime_error( "Must set number of bells before using "
			 "place notation" );
  
  interpret_pn( bells, pn.begin(), pn.end(), back_inserter( changes ) );
}

void pn_node::debug_print( ostream &os ) const
{
  copy( changes.begin(), changes.end(),
	ostream_iterator< change >( os, "." ) );
}

void pn_node::execute( proof_context &ctx )
{
  for_each( changes.begin(), changes.end(), 
	    ctx.permute_and_prove() );
}

transp_node::transp_node( int bells, const string &r )
  : transp(bells)
{
  if ( bells == -1 )
    throw runtime_error( "Must set number of bells before using "
			 "transpostions" );
  
  transp *= r;
  if ( transp.bells() != bells )
    throw runtime_error( "Transposition is on the wrong number of bells" );
}

void transp_node::debug_print( ostream &os ) const
{
  os << "'" << transp << "'";
}

void transp_node::execute( proof_context &ctx )
{
  ctx.permute_and_prove()( transp );
}

void symbol_node::debug_print( ostream &os ) const
{
  os << sym;
}

void symbol_node::execute( proof_context &ctx )
{
  ctx.execute_symbol(sym);
}

