// -*- C++ -*- row_calc.cpp - classes to implement a simple row calculator
// Copyright (C) 2009 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>


#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "row_calc.h"
#include "tokeniser.h"
#include "stringutils.h"
#include "exec.h"
#if RINGING_HAVE_OLD_IOSTREAMS
#include <fstream.h>
#include <istream.h>
#include <iostream.h>
#else
#include <fstream>
#include <istream>
#include <iostream>
#endif
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#include <vector.h>
#else
#include <stdexcept>
#include <vector>
#endif
#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#include <string.h>
#else
#include <cassert>
#include <cstring>
#endif
#include <ringing/row.h>
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

namespace tok_types
{
  enum enum_t {
    row_or_int  = '0',
    open_paren  = '(',
    close_paren = ')',
    times       = '*',
    divide      = '/',
    ldivide     = '\\', 
    power       = '^',
    minus       = '-',
    exec        = tokeniser::first_token,
    read
  };
}

class rowgen_impl : public tokeniser::basic_token
{
public:
  rowgen_impl( char const* init, tok_types::enum_t t ) 
    : basic_token( init, t ), ilen(strlen(init)) 
  {}

private:
  virtual parse_result parse( string::const_iterator& i,
                              string::const_iterator e,
                              token& tok ) const
  {
    string::const_iterator j = i + ilen;  // Skip $(
    while ( j < e && *j != ')' ) ++j;
    if ( j == e ) return more;
    tok = string(i + ilen, j); tok.type( type() ); 
    i = j; ++i; // Skip )
    assert( i <= e );
    return done;
  }

  size_t ilen;
};

class rc_tokeniser : public tokeniser 
{
public:
  explicit rc_tokeniser( istream& in )
    : tokeniser( in ), 
      exec_impl( "$(", tok_types::exec ),
      read_impl( "<(", tok_types::read )
  {
    add_qtype(&exec_impl); add_qtype(&read_impl);
  }

  virtual void validate( const token& t ) const
  {
    using namespace tok_types;

    switch ( t.type() ) {
      case row_or_int: case open_paren: case close_paren: 
      case times: case divide: case ldivide: case power: case minus:
        return;
    }
    throw runtime_error
      ( make_string() << "Unknown token -Fr expression: " << t );
  }

  rowgen_impl exec_impl, read_impl;
};

class mult_node : public row_calc::expr::node {
public:
  mult_node( row_calc::expr const& lhs, row_calc::expr const& rhs, 
             tok_types::enum_t op )
    : lhs(lhs), rhs(rhs), op(op) 
  {}

private:
  virtual row evaluate() {
    switch ( op ) {
      case tok_types::times:   
        return lhs.evaluate() * rhs.evaluate();
      case tok_types::divide:  
        return lhs.evaluate() * rhs.evaluate().inverse();
      case tok_types::ldivide: 
        return lhs.evaluate().inverse() * rhs.evaluate();
      default: abort();
    }
  }

  row_calc::expr lhs, rhs;
  tok_types::enum_t op;
};

class exp_node : public row_calc::expr::node {
public:
  exp_node( row_calc::expr const& lhs, int rhs )
    : lhs(lhs), rhs(rhs)
  {}

private:
  virtual row evaluate() { return lhs.evaluate().power(rhs); }

  row_calc::expr lhs;
  int rhs;
};

class row_node : public row_calc::expr::node {
public:
  row_node( row const& r ) : r(r)  {}

private:
  virtual row evaluate() { return r; }

  row r;
};

class vector_end {};

class exec_node : public row_calc::expr::node {
public:
  exec_node( string const& cmd ) : cmd(cmd), invoked(false) {}

private:
  virtual row evaluate() 
  {
    if (!invoked) {
      result = exec_command(cmd);
      invoked = true;
      idx = 0;
    }
 
    char const seps[] = " ,;:\t\r\n";

    string::size_type i = result.find_first_not_of( seps, idx );
    idx = result.find_first_of( seps, i );

    if ( i == string::npos ) 
      throw vector_end();
    return row(result.substr(i, idx == string::npos ? string::npos : idx-i));
  }

  string cmd;
  bool invoked;
  string result;
  string::size_type idx;
};

class read_node : public row_calc::expr::node {
public:
  read_node( string const& f ) 
    : file(f), in(NULL) 
  {
    trim_leading_whitespace(file);
    trim_trailing_whitespace(file);
  }

private:
  row evaluate()
  { 
    if (!in) {
      if (file == "-") in = &cin;
      else { 
        in_owner.reset( new ifstream(file.c_str()) ); 
        in = in_owner.get();
      }
      if (!*in) throw runtime_error( "Unable to open file: '" + file + "'" );
    }

    char const seps[] = " ,;:\t\r\n";
    while ( *in && strchr( seps, in->peek() ) ) in->get();
    if ( !*in || in->peek() == EOF ) throw vector_end();

    string r;
    while ( *in && !strchr( seps, in->peek() ) ) r += (char)in->get(); 
    return row(r);
  }

  string file;
  scoped_pointer<istream> in_owner;
  istream *in;
};


class rc_parser 
{
public:
  rc_parser( string const& str )
    : is(str), tok( is ), tokens( tok.begin(), tok.end() ), vec(false)
  {}

  row_calc::expr parse() { return parse_expr( tokens.begin(), tokens.end() ); }

  bool is_vector() const { return vec; }

private:
  typedef vector<token>::const_iterator iter_t;

  row_calc::expr parse_expr( iter_t first, iter_t last );
  int parse_int( iter_t first, iter_t last );

  template <size_t N>
  bool find_first( iter_t first, iter_t last,
                   tok_types::enum_t (&tt_list)[N],
                   iter_t &result ) const;

  bool find_close_paren( iter_t const first, iter_t const last,
                         tok_types::enum_t open, tok_types::enum_t close,
                         string const& name, iter_t& result ) const;

  bool is_enclosed( iter_t first, iter_t last,
                    tok_types::enum_t open, tok_types::enum_t close,
                    string const& name ) const;

  void check_binary_expr( iter_t first, iter_t mid, iter_t last,
                          string const& opname ) const;

  RINGING_ISTRINGSTREAM is;
  rc_tokeniser tok;
  vector<token> tokens;
  bool vec;
};

void
rc_parser::check_binary_expr( iter_t first, iter_t mid, iter_t last,
                              string const& opname ) const
{
  if ( first == mid )
    throw runtime_error
      ( make_string() << "Binary operator \"" << opname
                      << "\" needs first argument" );

  if ( mid+1 == last)
    throw runtime_error
      ( make_string() << "Binary operator \"" << opname
                      << "\" needs second argument" );
}

template <size_t N>
bool rc_parser::find_first( iter_t first, iter_t const last,
                            tok_types::enum_t (&tt_list)[N],
                            iter_t &result ) const
{
  int depth( 0 );
  for ( ; first != last; ++first )
    if      ( first->type() == tok_types::open_paren )
      ++depth;
    else if ( first->type() == tok_types::close_paren )
      --depth;
    else if ( !depth )
      for ( size_t i = 0u; i<N; ++i )
        if ( first->type() == tt_list[i] ) {
          result = first;
          return true;
        }

  if (depth)
    throw runtime_error( "Unmatched brackets" );

  return false;
}
 
bool
rc_parser::find_close_paren( iter_t const first, iter_t const last,
                             tok_types::enum_t open,  
                             tok_types::enum_t close,
                             string const& name, iter_t& result ) const
{
  if ( first->type() != open )
    return false;

  int depth = 0;
  for ( result = first; result != last; ++result )
    {
      if      ( result->type() == open  ) ++depth;
      else if ( result->type() == close ) --depth;

      if ( depth == 0 )
        {
          if (first+1 == result)
            throw runtime_error( make_string() << "Empty " << name );
          return true;
        }
    }

  assert(depth);
  throw runtime_error( make_string() << "Unmatched " << name );
}

bool rc_parser::is_enclosed( iter_t first, iter_t last,
                             tok_types::enum_t open, 
                             tok_types::enum_t close,
                             string const& name ) const
{
  if ( !find_close_paren(first, last, open, close, name, first) )
    return false;

  if ( first != last-1 )
    return false;

  return true;
}

int rc_parser::parse_int( iter_t first, iter_t last )
{
  if ( first == last )
    throw runtime_error( "Expression expected" );

  // Parentheses first
  if ( is_enclosed( first, last, tok_types::open_paren,
                    tok_types::close_paren, "parentheses" ) )
    return parse_int( first+1, last-1 );

  if ( first+1 == last  && first->type() == tok_types::row_or_int ) 
    return lexical_cast<int>( string(*first) );

  if ( first != last && first->type() == tok_types::minus )
    return - parse_int( first+1, last );

  throw runtime_error( "Unable to parse expression as integer" );
}

row_calc::expr rc_parser::parse_expr( iter_t first, iter_t last )
{
  if ( first == last )
    throw runtime_error( "Expression expected" );

  // Parentheses first
  if ( is_enclosed( first, last, tok_types::open_paren,
                    tok_types::close_paren, "parentheses" ) )
    return parse_expr( first+1, last-1 );

  iter_t split;
  tok_types::enum_t multops[3] 
    = { tok_types::times, tok_types::divide, tok_types::ldivide };
  if ( find_first( first, last, multops, split ) ) 
    {
      check_binary_expr( first, split, last, string(1, (char)split->type()) );
     
      return row_calc::expr
        ( new mult_node( parse_expr( first, split ),
                         parse_expr( split+1, last ), 
                         (tok_types::enum_t)split->type() ) );
    }

  tok_types::enum_t expop[1] = { tok_types::power };
  if ( find_first( first, last, expop, split ) ) 
    {
      check_binary_expr( first, split, last, string(1, (char)split->type()) );

      return row_calc::expr( new exp_node( parse_expr( first, split ),
                                           parse_int( split+1, last ) ) ); 
    } 

  if ( first+1 == last ) {
    if ( first->type() == tok_types::row_or_int )
      return row_calc::expr( new row_node( row( *first ) ) );

    if ( first->type() == tok_types::exec || 
         first->type() == tok_types::read ) 
      {
        if (vec) 
          throw runtime_error( "Only one row generator $() or <() allowed" ); 

        vec = true;

        if ( first->type() == tok_types::exec )
          return row_calc::expr( new exec_node(*first) );
        else
          return row_calc::expr( new read_node(*first) );
      }
  }

  throw runtime_error( "Unable to parse expression" );
}

RINGING_END_ANON_NAMESPACE

row_calc::row_calc( string const& str )
{
  rc_parser p(str);
  e = p.parse();
  v = p.is_vector();
}


void row_calc::const_iterator::increment() 
{ 
  if ( !rc->v && val.bells() )
    rc = 0;
  else try {
    val = rc->e.evaluate(); 
  } catch ( vector_end ) {
    rc = 0;
  }
}

