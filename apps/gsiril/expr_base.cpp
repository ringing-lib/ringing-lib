// expr_base.cpp - Base classes, nodes and factory function for expressions
// Copyright (C) 2005, 2009 Richard Smith <richard@ex-parrot.com>

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

RINGING_USING_NAMESPACE

bool expression::node::evaluate( proof_context& ctx )
{
  make_string os;
  os << "Unable to evaluate expression as a boolean: '";
  debug_print( ctx, os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

expression::integer_t expression::node::ievaluate( proof_context& ctx )
{
  make_string os;
  os << "Unable to evaluate expression as an integer: '";
  debug_print( ctx, os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

void expression::bnode::execute( proof_context& ctx )
{
  make_string os;
  if ( type(ctx) == boolean ) os << "Boolean expression ";
  else if ( type(ctx) == integer ) os << "Integer expression ";
  else os << "Expression ";
  os << "used in void context: '";
  debug_print( ctx, os.out_stream() );
  os << "'";
    
  throw runtime_error( os );  
}

string expression::node::lvalue( proof_context const& ctx ) const
{
  make_string os;
  os << "Attempt to modify the value of an expression: '";
  debug_print( ctx, os.out_stream() );
  os << "'";

  throw runtime_error( os );  
}
