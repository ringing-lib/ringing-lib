// -*- C++ -*- execution_context.h - Global environment
// Copyright (C) 2002, 2003, 2004, 2007, 2008, 2012, 2019
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
#include "symbol_table.h"
#include "expr_base.h" // MSVC 6 requires expression to be complete.

RINGING_USING_STD
RINGING_USING_NAMESPACE

class expression;

class execution_context
{
public:
  execution_context( ostream& os, const arguments& arg );
 ~execution_context();

  ostream& output() const { return *os; }

  bool defined( const string& sym ) const;

  // Returns true for a redefinition and false otherwise
  bool define_symbol( const pair< const string, expression > &defn );

  // Returns true if used and false otherwise
  bool default_define_symbol( const pair< const string, expression > &defn );

  void undefine_symbol( const string& sym );
  expression lookup_symbol( const string &sym ) const;
  void prove_symbol( const string& str ); 
 
  void extents( int n ) { args.num_extents = n; }
  int  extents() const { return args.num_extents; }

  int bells( int b );
  int bells() const  { return args.bells; }

  bool interactive( bool i ) { swap(i, args.interactive.get()); return i; }
  bool interactive() const   { return args.interactive; }

  bool verbose( bool v ) { swap(v, args.verbose.get()); return v; }
  bool verbose() const   { return args.verbose; }

  row rounds( row r ) { swap(r, args.rounds); return r; }
  row rounds() const { return args.rounds; }

  const arguments& get_args() const { return args; }

  void set_failure( bool f = true ) { failed = f; }
  bool failure() const { return failed; }

  void increment_node_count() const;
  bool done_one_proof() const { return done_proof; }
  void set_done_proof() { done_proof = true; }

  int expected_length() const { return args.expected_length; }
  int expected_length(int l);

private:
  arguments args;
  ostream* os;
  symbol_table sym_table;
  bool failed;
  mutable int node_count;
  bool done_proof;
};

#endif // GSIRIL_EXECUTION_CONTEXT_INCLUDED
