// -*- C++ -*- proof_context.h - Environment to evaluate expressions
// Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007
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
#include "symbol_table.h"

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
  
    row &r;
    prover &p;
    proof_context &pctx;
  };

  explicit proof_context( const execution_context & );
 ~proof_context();
  
  permute_and_prove_t permute_and_prove();

  row current_row() const { return r; }
  bool isrounds() const;

  void execute_symbol( const string &sym );
  void define_symbol( const pair< const string, expression > &defn );

  enum proof_state { rounds, notround, isfalse };
  proof_state state() const;
  string substitute_string( const string &str, bool &do_exit );

  void execute_everyrow();
  void output_string( const string& str );

  proof_context silent_clone() const;

private:
  void termination_sequence( ostream& os );

  const execution_context &ectx;
  symbol_table dsym_table; // dynamic symbol table
  row r;
  shared_pointer<prover> p;

  ostream* output;
  bool silent;
  bool underline;
};

#endif // GSIRIL_PROOF_CONTEXT_INCLUDED

