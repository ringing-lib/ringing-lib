// -*- C++ -*- parser.h - Tokenise and parse lines of input
// Copyright (C) 2002 Richard Smith <richard@ex-parrot.com>

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

#ifndef GSIRIL_PARSER_INCLUDED
#define GSIRIL_PARSER_INCLUDED

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <utility.h>
#include <map.h>
#else
#include <utility>
#include <map>
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <istream.h>
#include <ostream.h>
#else
#include <iosfwd>
#endif
#include <string>
#include <ringing/pointers.h>
#include "expression.h"

RINGING_USING_NAMESPACE

class expression;

class parser
{
public:
  parser();
 ~parser();

  void init_with( const string &str );
  void read_file( istream &in, ostream &out );
  bool run_final_proof( ostream &out ) const;

  int bells() const { return b; } 
  void set_interactive( bool i ) { interactive = i; }

  expression lookup_symbol( const string &sym ) const;

private:
  bool run_proof( ostream &out, const expression &node ) const;

  bool maybe_handle_bells_command( const string &cmd, ostream &out );
  bool maybe_handle_prove_command( const string &cmd, ostream &out );
  bool maybe_handle_definition( const string &cmd, ostream &out );
  pair< const string, expression > parse_definition( const string &cmd ) const;

  bool interactive;
  int b;

  // Make it uncopyable
  parser &operator=( const parser & );
  parser( const parser & );

  typedef map< string, expression > sym_table_t;

  sym_table_t sym_table;
  string entry_sym;
};

#endif // GSIRIL_PARSER_INCLUDED
