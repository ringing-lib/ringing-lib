// functions.cpp - Built-in functions
// Copyright (C) 2021, 2022 Richard Smith <richard@ex-parrot.com>

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

class at_fn_impl : public fnnode {
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size() != 1)
      throw runtime_error("The at function takes one argument");
    int idx = args[0].int_evaluate(ctx);
    if (idx < 1 || idx >= ctx.bells())
      throw runtime_error("Argument out of range in at function");
    row const& r = ctx.current_row();
    int rv = idx > r.bells() ? idx : r[idx-1]+1;
    return expression( new integer_node(rv) );
  }

  virtual void debug_print( ostream &os ) const { os << "pos"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::integer; }
};

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

class rowno_fn_impl : public fnnode {
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size())
      throw runtime_error("The rowno function takes no arguments");
    return expression( new integer_node(ctx.length()) );
  }

  virtual void debug_print( ostream &os ) const { os << "rowno"; }
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

class stagename_impl : public fnnode {
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size() != 1)
      throw runtime_error("The stagename function takes one argument");
    string rv( method::stagename( args[0].int_evaluate(ctx) ) );
    return expression( new string_node(rv) );
  }

  virtual void debug_print( ostream &os ) const { os << "stagename"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::no_type; }
};

class music_impl : public fnnode {
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size())
      throw runtime_error("The music function takes no arguments");
    return expression( new integer_node( ctx.music_score() ) );
  }

  virtual void debug_print( ostream &os ) const { os << "music"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::integer; }
};

class runs_impl : public fnnode {
public:
  explicit runs_impl( music::match_pos pos = music::anywhere ) : pos(pos) {}

private:
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size() > 2)
      throw runtime_error("The runs function takes up to two arguments");
    int len( args.size() > 0 ? args[0].int_evaluate(ctx) : 4 );
    int dir( args.size() > 1 ? args[1].int_evaluate(ctx) : 0 );
    return expression( new opaque_music_node( 
      music::make_runs_match( ctx.bells(), len, pos, dir, 1, 1 ) ) );
  }

  virtual void debug_print( ostream &os ) const { os << "runs"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::no_type; }

  music::match_pos pos;
};

class crus_impl : public fnnode {
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size())
      throw runtime_error("The crus function takes no arguments");
    return expression( new opaque_music_node( 
      music::make_cru_match( ctx.bells(), 1,1 ) ) );
  }

  virtual void debug_print( ostream &os ) const { os << "crus"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::no_type; }

  music::match_pos pos;
};

class methname_impl : public fnnode {
  virtual expression call( proof_context& ctx, 
                           vector<expression> const& args ) const {
    if (args.size() != 1)
      throw runtime_error("The crus function takes one argument");
    string rv( args[0].name(ctx) );
    return expression( new string_node(rv) );
  }

  virtual void debug_print( ostream &os ) const { os << "methname"; }
  virtual expression::type_t type( proof_context &ctx ) const 
    { return expression::no_type; }

  music::match_pos pos;
};

void register_functions( execution_context& ectx )
{
  ectx.define_symbol( pair< const string, expression >
    ( "at", expression( new at_fn_impl() ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "pos", expression( new pos_fn_impl() ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "rowno", expression( new rowno_fn_impl() ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "swap", expression( new swap_fn_impl() ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "load", expression( new load_fn_impl( load_fn_impl::full_method ) ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "loadm", expression( new load_fn_impl( load_fn_impl::lead_only ) ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "loadlh", expression( new load_fn_impl( load_fn_impl::lead_head ) ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "stagename", expression( new stagename_impl ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "music", expression( new music_impl ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "runs", expression( new runs_impl ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "frontruns", expression( new runs_impl( music::at_front ) ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "backruns", expression( new runs_impl( music::at_back ) ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "crus", expression( new runs_impl ) ) );
  ectx.define_symbol( pair< const string, expression >
    ( "methname", expression( new methname_impl ) ) );
}

