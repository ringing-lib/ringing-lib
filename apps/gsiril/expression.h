// -*- C++ -*- expression.h - Code to execute different types of expression
// Copyright (C) 2003, 2004, 2005, 2008, 2009
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

#ifndef GSIRIL_EXPRESSION_INCLUDED
#define GSIRIL_EXPRESSION_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include "expr_base.h"
#if RINGING_OLD_INCLUDES
#include <utility.h>
#else
#include <utility>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/music.h>
#include <ringing/method.h>

// Forward declare ringing::method and ringing::change
RINGING_START_NAMESPACE
class method;
class change;
RINGING_END_NAMESPACE

RINGING_USING_NAMESPACE


class list_node : public expression::node
{
public:
  list_node( const expression& car, const expression& cdr )
    : car(car), cdr(cdr) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );
  virtual bool evaluate( proof_context &ctx );
  virtual expression::integer_t ievaluate( proof_context &ctx );
  virtual expression::type_t type( proof_context const& ctx ) const;

private:  
  expression car, cdr;
};

class nop_node : public expression::node
{
protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context & );
  virtual bool isnop( proof_context const& ) const;
};

class repeated_node : public expression::node
{
public:
  repeated_node( expression const& count, expression const& child )
    : count(count), child(child) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  expression count, child;
};

#define RINGING_GSIRIL_BINARY_INODE( name )                                  \
                                                                             \
class name##_node : public expression::inode                                 \
{                                                                            \
public:                                                                      \
  name##_node( expression const& lhs, expression const& rhs )                \
    : lhs(lhs), rhs(rhs) {}                                                  \
                                                                             \
protected:                                                                   \
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;   \
  virtual expression::integer_t ievaluate( proof_context &ctx );             \
                                                                             \
private:                                                                     \
  expression lhs, rhs;                                                       \
}

RINGING_GSIRIL_BINARY_INODE( add      );
RINGING_GSIRIL_BINARY_INODE( subtract );
RINGING_GSIRIL_BINARY_INODE( multiply );
RINGING_GSIRIL_BINARY_INODE( divide   );
RINGING_GSIRIL_BINARY_INODE( ldivide  );
RINGING_GSIRIL_BINARY_INODE( modulus  );
RINGING_GSIRIL_BINARY_INODE( exponent );

#undef RINGING_GSIRIL_BINARY_INODE

class factorial_node : public expression::inode
{
public:
  explicit factorial_node( expression const& arg )
    : arg(arg) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual expression::integer_t ievaluate( proof_context &ctx );

private:
  expression arg;
};

class repeat_or_multiply_node : public expression::node 
{
public:
  repeat_or_multiply_node( expression const& lhs, expression const& rhs )
    : lhs(lhs), rhs(rhs) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const 
    { make_delegate(ctx).debug_print(ctx, os); }
  virtual void execute( proof_context &ctx ) 
    { make_delegate(ctx).execute(ctx); }
  virtual bool evaluate( proof_context &ctx )
    { return make_delegate(ctx).evaluate(ctx); }
  virtual expression::integer_t ievaluate( proof_context &ctx )
    { return make_delegate(ctx).ievaluate(ctx); }
  virtual bool isnop( proof_context const& ctx ) const 
    { return make_delegate(ctx).isnop(ctx); }
  virtual expression::type_t type( proof_context const& ctx ) const 
    { return make_delegate(ctx).type(ctx); }

private:
  expression make_delegate( proof_context const& ) const;

  expression lhs, rhs;
};

class string_node : public expression::node
{
public:
  explicit string_node( const string &str ) 
    : str(str) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  string str;
};

class bool_node : public expression::bnode
{
public:
  explicit bool_node( bool val ) : val(val) {}

  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual bool evaluate( proof_context &ctx );

private:
  bool val;
};

class int_node : public expression::inode
{
public:
  explicit int_node( integer_t val ) : val(val) {}
  explicit int_node( string const& str );

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual integer_t ievaluate( proof_context &ctx );
  virtual void execute( proof_context& ctx );

private:
  integer_t val;
};

class pn_node : public expression::node
{
public:
  pn_node( int bells, const string &pn );

  explicit pn_node( method const& m );
  explicit pn_node( change const& ch );

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  vector< change > changes;
};

class transp_node : public expression::node
{
public:
  transp_node( int bells, const string &r );

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  row transp;
};

class symbol_node : public expression::node
{
public:
  explicit symbol_node( const string &sym )
    : sym(sym) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );
  virtual bool evaluate( proof_context &ctx );
  virtual expression::integer_t ievaluate( proof_context &ctx );
  virtual string lvalue( proof_context const& ctx ) const;
  virtual expression::type_t type( proof_context const& ctx ) const;

private:
  string sym;
};

class assign_node : public expression::node
{
public:
  assign_node( const string& sym, const expression& val )
    : defn( make_pair(sym, val) ) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );
  virtual bool evaluate( proof_context &ctx );
  virtual expression::integer_t ievaluate( proof_context &ctx );
  virtual expression::type_t type( proof_context const& ctx ) const;

private:
  pair< const string, expression > defn;
};

class isrounds_node : public expression::bnode
{
protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual bool evaluate( proof_context &ctx );
};

class pattern_node : public expression::bnode
{
public:
  pattern_node( int bells, const string& regex );

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual bool evaluate( proof_context &ctx );

private:
  music mus;
};

class and_node : public expression::bnode
{
public:
  and_node( expression const& left, expression const& right )
    : left(left), right(right) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual bool evaluate( proof_context &ctx );

private:
  expression left, right;
};

class or_node : public expression::bnode
{
public:
  or_node( expression const& left, expression const& right )
    : left(left), right(right) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual bool evaluate( proof_context &ctx );

private:
  expression left, right;
};

class not_node : public expression::bnode 
{
public:
  explicit not_node( expression const& arg )
    : arg(arg) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual bool evaluate( proof_context &ctx );

private:
  expression arg;
};

class if_match_node : public expression::node
{
public:
  if_match_node( const expression& test,
		 const expression& iftrue, const expression& iffalse )
    : test(test), iftrue(iftrue), iffalse(iffalse) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  expression test, iftrue, iffalse;
};

class exception_node : public expression::node
{
public:
  explicit exception_node( script_exception::type t ) : t(t) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  script_exception::type t;
};

class load_method_node : public expression::node
{
public:
  explicit load_method_node( string const& name )
    : name(name), read(false) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  string name;

  // Reading the method is expensive; we don't want to re-read it every
  // time the node is evaluated.  Set read=true when it has been read.
  // If meth is sill empty, we'll throw an exception.
  bool read;
  method meth;
};

class cmp_node_base : public expression::bnode
{
public:
  enum operation {
    eq, ne, lt, le, gt, ge
  };

  cmp_node_base( operation op, expression const& lhs, expression const& rhs )
    : op(op), lhs(lhs), rhs(rhs) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual bool evaluate( proof_context &ctx );

private:
  operation op;
  expression lhs, rhs;
};

template <cmp_node_base::operation Op> 
class cmp_node : public cmp_node_base 
{
public:
  cmp_node( expression const& lhs, expression const& rhs )
    : cmp_node_base( Op, lhs, rhs ) {}
};

class incr_node : public expression::inode
{
public:
  enum mode_t { postfix, prefix };  

  explicit incr_node( expression const& arg, int by, mode_t mode )
    : arg(arg), by(by), mode(mode) {}

protected:
  virtual void debug_print( proof_context const& ctx, ostream &os ) const;
  virtual void execute( proof_context &ctx );
  virtual expression::integer_t ievaluate( proof_context &ctx );

private:
  expression arg;
  int by;
  mode_t mode;
};


#endif // GSIRIL_EXPRESSION_INCLUDED
