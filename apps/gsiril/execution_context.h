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

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
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

RINGING_USING_STD
RINGING_USING_NAMESPACE

class expression;

class execution_context
{
public:
  execution_context( ostream& os, bool interactive = false );
 ~execution_context();

  ostream& output() const { return *os; }

  // Returns true for a redefinition and false otherwise
  bool define_symbol( const pair< const string, expression > &defn );
  expression lookup_symbol( const string &sym ) const;

  int bells( int newb ) { int oldb (b); b=newb; return oldb; }
  int bells() const  { return b; }

  bool interactive( bool newi ) { bool oldi(intrv); intrv=newi; return oldi; }
  bool interactive() const       { return intrv; }

private:
  bool intrv;
  int b;

  ostream* os;

  typedef map< string, expression > sym_table_t;
  sym_table_t sym_table;
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
    permute_and_prove_t( row &r, prover &p, proof_context &ex );
  
    row &r;
    prover &p;
    proof_context &ex;
  };

  explicit proof_context( const execution_context & );
  
  ostream &output() { return ectx.output(); }
  permute_and_prove_t permute_and_prove();

  void   execute_symbol( const string &sym );

  enum proof_state { rounds, notround, isfalse } state() const;
  string substitute_string( const string &str, bool &do_exit );

private:
  const execution_context &ectx;
  row r;
  prover p;
};

#endif // GSIRIL_EXECUTION_CONTEXT_INCLUDED
