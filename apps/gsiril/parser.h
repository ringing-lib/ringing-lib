// -*- C++ -*-

#ifndef GSIRIL_PARSER_INCLUDED
#define GSIRIL_PARSER_INCLUDED

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
  bool maybe_handle_defintion( const string &cmd, ostream &out );
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
