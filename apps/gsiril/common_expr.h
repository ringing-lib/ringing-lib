// -*- C++ -*- common_expr.h - Some reusable expression & statement classes
// Copyright (C) 2003 Richard Smith <richard@ex-parrot.com>

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

#ifndef GSIRIL_COMMON_EXPR_INCLUDED
#define GSIRIL_COMMON_EXPR_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface "gsiril/common_expr.h"
#endif

#include "expression.h"
#if RINGING_OLD_INCLUDES
#include <utility.h>
#else
#include <utility>
#endif
#include <string>
#include <ringing/row.h>

RINGING_USING_NAMESPACE

// Defines a symbol
class definition_stmt : public statement::impl
{
public:
  explicit definition_stmt( const string& name, const expression& val )
    : defn(name, val) {}

private:
  virtual void execute( execution_context& ) const;

  pair<const string, expression> defn;
};

// These can be emited by directives that are handled
// entirely within the parser.  The diagnostic allows the parser
// to notify the user anything necessary.  It is only used if interactive.
class null_stmt : public statement::impl
{
public:
  explicit null_stmt( const string& diagnostic = string() )
    : diagnostic(diagnostic) {}

private:
  virtual void execute( execution_context& ) const;

  string diagnostic;
};

// Prove a touch
class prove_stmt : public statement::impl
{
public:
  explicit prove_stmt( const expression& expr )
    : expr(expr) {}

private:
  virtual void execute( execution_context& ) const;

  expression expr;
};

// Set the default number of bells
class bells_stmt : public statement::impl
{
public:
  explicit bells_stmt( int bells )
    : bells(bells) {}

private:
  virtual void execute( execution_context& ) const;

  int bells;
};

// Import a resource
class import_stmt : public statement::impl
{
public:
  explicit import_stmt( const string& name )
    : name(name) {}

private:
  virtual void execute( execution_context& ) const;

  string name;
};

// 
// 
//

class list_node : public expression::node
{
public:
  list_node( const expression& car, const expression& cdr )
    : car(car), cdr(cdr) {}

protected:
  virtual void debug_print( ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:  
  expression car, cdr;
};

class nop_node : public expression::node
{
protected:
  virtual void debug_print( ostream &os ) const;
  virtual void execute( proof_context & );
};

class repeated_node : public expression::node
{
public:
  repeated_node( int count,
		 const expression &child )
    : count(count), child(child) {}

protected:
  virtual void debug_print( ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:  
  int count;
  expression child;
};

class string_node : public expression::node
{
public:
  string_node( const string &str ) 
    : str(str) {}

protected:
  virtual void debug_print( ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  string str;
};

class pn_node : public expression::node
{
public:
  pn_node( int bells, const string &pn );

protected:
  virtual void debug_print( ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  vector< change > changes;
};

class transp_node : public expression::node
{
public:
  transp_node( int bells, const string &r );

protected:
  virtual void debug_print( ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  row transp;
};

class symbol_node : public expression::node
{
public:
  symbol_node( const string &sym )
    : sym(sym) {}

protected:
  virtual void debug_print( ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  string sym;
};

class assign_node : public expression::node
{
public:
  assign_node( const string& sym, const expression& val )
    : defn( make_pair(sym, val) ) {}

protected:
  virtual void debug_print( ostream &os ) const;
  virtual void execute( proof_context &ctx );

private:
  pair< const string, expression > defn;
};

#endif // GSIRIL_EXPRESSION_INCLUDED
