// functions.cpp - Built-in functions
// Copyright (C) 2021 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/streamutils.h>
#include "expression.h"
#include "functions.h"
#include "execution_context.h"
#include "proof_context.h"

RINGING_USING_NAMESPACE

void fnnode::execute( proof_context &ctx, int dir ) const
{
  make_string os;
  os << "Unable to execute function '";
  debug_print( os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

class pos_fn_impl : public fnnode {
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size() != 1)
      throw runtime_error("The pos function takes one argument");
    int rv = ctx.current_row().find(args[0].int_evaluate(ctx) - 1) + 1;
    return expression( new integer_node(rv) );
  }

  virtual void debug_print( ostream &os ) const { os << "pos"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::integer; }
};

class swap_fn_impl : public fnnode {
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size() != 2)
      throw runtime_error("The pos function takes two arguments");
    row r( ctx.bells() );
    int i = args[0].int_evaluate(ctx) - 1;
    int j = args[1].int_evaluate(ctx) - 1;
    if (i >= r.bells() || j >= r.bells() || i < 0 ||  j < 0)
      throw runtime_error("Bell out of bounds in swap");
    else if (i == j)
      throw runtime_error("Bell swapping with self");
    r.swap(i, j);
    return expression( new transp_node(r) );
  }

  virtual void debug_print( ostream &os ) const { os << "swap"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::transp; }
};

void register_functions( execution_context& ectx )
{
  ectx.define_symbol( pair< const string, expression >
                        ( "pos", expression( new pos_fn_impl() ) ) );
  ectx.define_symbol( pair< const string, expression >
                        ( "swap", expression( new swap_fn_impl() ) ) );
}

