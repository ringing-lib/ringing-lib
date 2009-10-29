// -*- C++ -*- statement.h - Code to execute different types of statement
// Copyright (C) 2003, 2004, 2005, 2009 Richard Smith <richard@ex-parrot.com>

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

#ifndef GSIRIL_STATEMENT_INCLUDED
#define GSIRIL_STATEMENT_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include "expr_base.h"
#include <string>
#include <ringing/row.h>


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

// Default definition of a symbol 
// (defines only if not already defined)
class default_defn_stmt : public statement::impl
{
public:
  explicit default_defn_stmt( const string& name, const expression& val )
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

// Print the value of an expression
class print_stmt : public statement::impl
{
public:
  explicit print_stmt( const expression& expr )
    : expr(expr) {}

private:
  virtual void execute( execution_context& ) const;

  expression expr;
};

// Set the number of extents
class extents_stmt : public statement::impl
{
public:
  explicit extents_stmt( int n )
    : n(n) {}

private:
  virtual void execute( execution_context& ) const;

  int n;
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

class rounds_stmt : public statement::impl
{
public:
  explicit rounds_stmt( const row& rounds )
    : rounds(rounds) {}

private:
  virtual void execute( execution_context& ) const;

  row rounds;
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


#endif // GSIRIL_STATEMENT_INCLUDED
