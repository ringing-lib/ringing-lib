// -*- C++ -*- proof_context.h - Environment to evaluate expressions
// Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2010, 2011, 2012, 2014,
// 2019, 2020, 2021 Richard Smith <richard@ex-parrot.com>

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

#ifndef GSIRIL_PROOF_CONTEXT_INCLUDED
#define GSIRIL_PROOF_CONTEXT_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_HAVE_OLD_IOSTREAM
#include <ostream.h>
#else
#include <iosfwd>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/proof.h>
#include <ringing/music.h>
#include "symbol_table.h"

RINGING_START_NAMESPACE
class method;
RINGING_END_NAMESPACE

RINGING_USING_STD
RINGING_USING_NAMESPACE

class execution_context;

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

    bool prove();
  
    row &r;
    prover &p;
    proof_context &pctx;
  };

  explicit proof_context( const execution_context & );
 ~proof_context();
  
  permute_and_prove_t permute_and_prove();
  void disable_proving();
  bool is_proving() { return proving; }

  row current_row() const { return r; }
  bool isrounds() const;
  size_t length() const { return p->size(); }
  int bells() const;

  bool set_silent( bool s ) { bool rv = silent; silent = s; return rv; }

  void execute_symbol( const string& sym, int dir = +1 );
  void execute_final_symbol( const string& sym );
  void define_symbol( const pair< const string, expression > &defn );
  expression lookup_symbol( const string& sym ) const;
  bool defined( const string& sym ) const;

  enum proof_state { rounds, notround, isfalse };
  proof_state state() const;

  string string_escapes( const string& str );
  string substitute_string( const string &str, bool *do_exit = 0,
                            bool *do_nl = 0 ) const;

  void execute_everyrow();
  void output_string( const string& str, bool to_parent = false ) const;
  void output_newline( bool to_parent = false ) const;

  method load_method( const string& title );

  proof_context silent_clone() const;

  void increment_node_count() const;

private:
  void termination_sequence( ostream& os ) const;
  void do_output( string const& str ) const;

  const execution_context &ectx;
  symbol_table dsym_table; // dynamic symbol table
  mutable music row_mask;
  row r, last_row;
  size_t max_length;
  shared_pointer<prover> p;
  bool proving;
  proof_context const* parent;

  ostream* output;
  bool silent;
  mutable bool underline;
};

#endif // GSIRIL_PROOF_CONTEXT_INCLUDED

