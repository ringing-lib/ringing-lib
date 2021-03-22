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

#include <ringing/method.h>
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
    if (args.size() < 1 || args.size() > 2)
      throw runtime_error("The pos function takes one or two arguments");
    row r( ctx.bells() );
    int i = args[0].int_evaluate(ctx) - 1;
    int j = args.size() == 2 ?  args[1].int_evaluate(ctx) - 1 : i + 1;
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

class load_fn_impl : public fnnode {
public:
  enum type_t { full_method, lead_only, lead_head };
  explicit load_fn_impl( type_t t ) : t(t) {}

private:
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if ( args.size() != 1 )
      throw runtime_error("The load function takes one argument");
    method m( ctx.load_method( args[0].string_evaluate(ctx) ) );
    if ( t == lead_head )
      return expression( new pn_node( m.back() ) );
    if ( t == lead_only )
      m.pop_back();
    return expression( new pn_node(m) );
  }

  virtual void debug_print( ostream &os ) const { os << "load"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::no_type; }

  type_t t;
};

void register_functions( execution_context& ectx )
{
  ectx.define_symbol( pair< const string, expression >
    ( "pos", expression( new pos_fn_impl() ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "swap", expression( new swap_fn_impl() ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "load", expression( new load_fn_impl( load_fn_impl::full_method ) ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "loadm", expression( new load_fn_impl( load_fn_impl::lead_only ) ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "loadlh", expression( new load_fn_impl( load_fn_impl::lead_head ) ) ) );
}

