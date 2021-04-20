// expr_base.cpp - Base classes, nodes and factory function for expressions
// Copyright (C) 2005, 2011, 2012, 2019, 2020, 2021
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

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif
#include <ringing/streamutils.h>
#include "expr_base.h"
#include "proof_context.h"
#include "execution_context.h"
#include "expression.h"

RINGING_USING_NAMESPACE

bool expression::node::bool_evaluate( proof_context& ctx ) const
{
  make_string os;
  os << "Unable to evaluate expression as a boolean: '";
  debug_print( os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

RINGING_LLONG expression::node::int_evaluate( proof_context& ctx ) const
{
  make_string os;
  os << "Unable to evaluate expression as an integer: '";
  debug_print( os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

string expression::node::string_evaluate( proof_context& ctx ) const
{
  make_string os;
  os << "Unable to evaluate expression as a string: '";
  debug_print( os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

string 
expression::node::string_evaluate( proof_context &ctx, bool *no_nl ) const 
{
  return string_evaluate(ctx);
}

expression expression::node::evaluate( proof_context& ctx ) const
{
  make_string os;
  os << "Unable to evaluate expression: '";
  debug_print( os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

expression expression::node::call( proof_context& ctx,
                                   vector<expression> const& args ) const
{
  make_string os;
  os << "Unable to call expression: '";
  debug_print( os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

void expression::bnode::execute( proof_context& ctx, int dir ) const
{
  if ( dir < 0 ) {
    make_string os;
    os << "Unable to execute expression backwards: '";
    debug_print( os.out_stream() );
    os << "'";

    throw runtime_error( os );
  }

  bool_evaluate(ctx);
}

expression expression::bnode::evaluate( proof_context& ctx ) const
{
  return expression( new boolean_node( bool_evaluate(ctx) ) );
}

void expression::inode::execute( proof_context& ctx, int dir ) const
{
  if ( dir < 0 ) {
    make_string os;
    os << "Unable to execute expression backwards: '";
    debug_print( os.out_stream() );
    os << "'";

    throw runtime_error( os );
  }

  int_evaluate(ctx);
}

expression expression::inode::evaluate( proof_context& ctx ) const
{
  return expression( new integer_node( int_evaluate(ctx) ) );
}


// This deals with casting ints as strings
string expression::inode::string_evaluate( proof_context& ctx ) const 
{
  return make_string() << int_evaluate(ctx);
}

void expression::snode::execute( proof_context &ctx, int dir ) const
{
  ctx.output_string( string_evaluate(ctx) );
}

expression expression::snode::evaluate( proof_context& ctx ) const
{
  return expression( new string_node( string_evaluate(ctx) ) );
}


void expression::execute( proof_context &ctx, int dir ) const 
{ 
  ctx.increment_node_count();
  impl->execute(ctx, dir); 
}

expression expression::evaluate( proof_context& ctx ) const 
{ 
  ctx.increment_node_count();
  return impl->evaluate(ctx); 
}

bool expression::bool_evaluate( proof_context& ctx ) const 
{ 
  ctx.increment_node_count();
  return impl->bool_evaluate(ctx); 
}

RINGING_LLONG expression::int_evaluate( proof_context& ctx ) const 
{ 
  ctx.increment_node_count();
  return impl->int_evaluate(ctx); 
}

string expression::string_evaluate( proof_context& ctx ) const
{
  ctx.increment_node_count();
  return impl->string_evaluate(ctx); 
}

string expression::string_evaluate( proof_context& ctx, bool* no_nl ) const
{
  ctx.increment_node_count();
  return impl->string_evaluate(ctx, no_nl); 
}

expression expression::call( proof_context& ctx, 
                             vector<expression> const& args ) const
{
  ctx.increment_node_count();
  return impl->call(ctx, args); 
}

void statement::execute( execution_context& e ) {
  if (e.is_executing())
    pimpl->execute(e);
  else 
    pimpl->skip(e);
}

