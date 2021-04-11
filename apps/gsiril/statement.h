// -*- C++ -*- statement.h - Code to execute different types of statement
// Copyright (C) 2003, 2004, 2005, 2011, 2019, 2020, 2021
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

#ifndef GSIRIL_STATEMENT_INCLUDED
#define GSIRIL_STATEMENT_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include "expr_base.h"
#include <string>
#include <ringing/row.h>
#include <ringing/music.h>


// Defines a symbol
class definition_stmt : public statement::impl
{
public:
  explicit definition_stmt( const string& name, const expression& val )
    : defn(name, val) {}

private:
  virtual void execute( execution_context& );
  virtual bool is_definition() const { return true; }

  pair<const string, expression> defn;
};

// Default definition of a symbol 
// (defines only if not already defined)
class default_defn_stmt : public statement::impl
{
public:
  explicit default_defn_stmt( const string& name, const expression& val )
    : defn(name, val) {}

private:
  virtual void execute( execution_context& );
  virtual bool is_definition() const { return true; }

  pair<const string, expression> defn;
};

// Immediate definition of a symbol, evaluating its value
class immediate_defn_stmt : public statement::impl
{
public:
  explicit immediate_defn_stmt( const string& name, const expression& val )
    : name(name), val(val) {}

private:
  virtual void execute( execution_context& );
  virtual bool is_definition() const { return true; }

  const string name;
  expression val;
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
  virtual void execute( execution_context& );

  string diagnostic;
};

// Prove a touch
class prove_stmt : public statement::impl
{
public:
  explicit prove_stmt( const expression& expr )
    : expr(expr) {}

private:
  virtual void execute( execution_context& );

  expression expr;
};

// Set the number of extents
class extents_stmt : public statement::impl
{
public:
  explicit extents_stmt( int n )
    : n(n) {}

private:
  virtual void execute( execution_context& );

  int n;
};

// Set the default number of bells
class bells_stmt : public statement::impl
{
public:
  explicit bells_stmt( int bells )
    : bells(bells) {}

private:
  virtual void execute( execution_context& );

  int bells;
};

// Set the expected length
class rows_stmt : public statement::impl
{
public:
  explicit rows_stmt( size_t len ) : len(len, len) {}
  rows_stmt( size_t len1, size_t len2 ) : len(len1, len2) {}

private:
  virtual void execute( execution_context& );

  pair<size_t, size_t> len;
};

class rounds_stmt : public statement::impl
{
public:
  explicit rounds_stmt( const row& rounds )
    : rounds(rounds) {}

private:
  virtual void execute( execution_context& );

  row rounds;
};

class row_mask_stmt : public statement::impl
{
public:
  explicit row_mask_stmt( const music_details& mask )
    : mask(mask) {}

private:
  virtual void execute( execution_context& );

  music_details mask;
};

// Import a resource
class import_stmt : public statement::impl
{
public:
  explicit import_stmt( const string& name )
    : name(name) {}

private:
  virtual void execute( execution_context& );

  string name;
};

// Print a message
class echo_stmt : public statement::impl
{
public:
  explicit echo_stmt( const expression& expr, bool substitute )
    : expr(expr), substitute(substitute) {}

private:
  virtual void execute( execution_context& );

  expression expr;
  bool substitute;
};

class if_stmt : public statement::impl
{
public:
  enum type_t {
    push_if, chain_else_if, chain_else, pop_if
  };

  explicit if_stmt( type_t type, const expression& expr = expression() )
    : type(type), expr(expr) {}

private:
  virtual void execute( execution_context& );
  virtual void skip( execution_context& ex ) { execute(ex); }

  type_t type;
  expression expr;
};


#endif // GSIRIL_STATEMENT_INCLUDED
