// -*- C++ -*- execution_context.h - Environment to evaluate expressions
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

#ifndef GSIRIL_EXECUTION_CONTEXT_INCLUDED
#define GSIRIL_EXECUTION_CONTEXT_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <map.h>
#else
#include <map>
#endif
#if RINGING_HAVE_OLD_IOSTREAM
#include <ostream.h>
#else
#include <iosfwd>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/proof.h>
#include <ringing/pointers.h>
#include "prog_args.h"
#include "expression.h" // MSVC 6 requires expression to be complete.

RINGING_USING_STD
RINGING_USING_NAMESPACE

class expression;

class symbol_table
{
public:
  expression lookup( const string& sym ) const;

  // Returns true for a redefinition and false otherwise
  bool define( const pair< const string, expression >& defn );

  void undefine( const string& sym );

private:
  typedef map< string, expression > sym_table_t;
  sym_table_t sym_table;
};

class execution_context
{
public:
  execution_context( ostream& os, const arguments& arg );
 ~execution_context();

  ostream& output() const { return *os; }

  // Returns true for a redefinition and false otherwise
  bool define_symbol( const pair< const string, expression > &defn );

  // Returns true if used and false otherwise
  bool default_define_symbol( const pair< const string, expression > &defn );

  void undefine_symbol( const string& sym );
  expression lookup_symbol( const string &sym ) const;
  void prove_symbol( const string& str ); 
 
  void extents( int n ) { args.num_extents = n; }

  int bells( int b ) { swap(b, args.bells.get()); return b; }
  int bells() const  { return args.bells; }

  bool interactive( bool i ) { swap(i, args.interactive.get()); return i; }
  bool interactive() const   { return args.interactive; }

  bool verbose( bool v ) { swap(v, args.verbose.get()); return v; }
  bool verbose() const   { return args.verbose; }

  const arguments& get_args() const { return args; }

private:
  arguments args;

  ostream* os;

  symbol_table sym_table;
};

class proof_context
{
public:
  struct permute_and_prove_t
  {
  public:
    typedef change argument_type;
    typedef bool result_type;
  
    bool operator()( const change &c );
    bool operator()( const row    &c );

  private:
    friend class proof_context;
    permute_and_prove_t( row &r, prover &p, proof_context &pctx );
  
    row &r;
    prover &p;
    proof_context &pctx;
  };

  explicit proof_context( const execution_context & );
 ~proof_context();
  
  permute_and_prove_t permute_and_prove();

  row current_row() const { return r; }

  void execute_symbol( const string &sym );
  void define_symbol( const pair< const string, expression > &defn );

  enum proof_state { rounds, notround, isfalse };
  proof_state state() const;
  string substitute_string( const string &str, bool &do_exit );

  void execute_everyrow();
  void output_string( const string& str );

private:
  const execution_context &ectx;
  symbol_table dsym_table; // dynamic symbol table
  row r;
  prover p;
  bool silent;
};

#endif // GSIRIL_EXECUTION_CONTEXT_INCLUDED
