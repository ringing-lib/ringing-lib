// expression.cpp - Nodes and factory function for expressions
// Copyright (C) 2002, 2003, 2004, 2005, 2008, 2011, 2014
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <iterator>

#include "expression.h"
#include "proof_context.h"
#include <ringing/streamutils.h>
#include <ringing/method.h>
#include <ringing/music.h>
#include <ringing/place_notation.h>


RINGING_USING_NAMESPACE


void list_node::debug_print( ostream &os ) const
{
  os << "( ";
  car.debug_print( os );
  os << ", ";
  cdr.debug_print( os );
  os << " )";
}

void list_node::execute( proof_context &ctx, int dir ) const
{
  if (dir > 0) car.execute( ctx, dir );
  cdr.execute( ctx, dir );
  if (dir <= 0) car.execute( ctx, dir );
}

bool list_node::evaluate( proof_context &ctx ) const
{
  car.execute( ctx, +1 );
  return cdr.evaluate( ctx );
}

expression::type_t list_node::type() const
{
  return cdr.type();
}

void nop_node::debug_print( ostream &os ) const
{
  os << "{null}";
}

void nop_node::execute( proof_context &, int dir ) const
{
}

bool nop_node::isnop() const
{
  return true;
}

void repeated_node::debug_print( ostream &os ) const
{
  if (count != -1) os << count << " ";
  else os << "repeat ";
  child.debug_print( os );
}

void repeated_node::execute( proof_context &ctx, int dir ) const
{
  try {
    if ( count != -1 )
      for (int i=0; i<count; ++i) 
        child.execute( ctx, dir );
    else
      while ( true )
        child.execute( ctx, dir );
  } catch ( script_exception const& ex ) {
    if ( ex.t == script_exception::do_break ) 
      return;
    else
      throw;
  }
}

void reverse_node::debug_print( ostream &os ) const
{
  os << "~";
  child.debug_print( os );
}

void reverse_node::execute( proof_context &ctx, int dir ) const
{
  child.execute( ctx, -dir );
}

void string_node::execute( proof_context &ctx, int dir ) const
{
  ctx.output_string(str);
}

void string_node::debug_print( ostream &os ) const
{
  os << "\"" << str << "\"";
}

pn_node::pn_node( int bells, const string &pn )
{
  if ( bells <= 0 )
    throw runtime_error( "Must set number of bells before using "
			 "place notation" );
  
  interpret_pn( bells, pn.begin(), pn.end(), back_inserter( changes ) );
}

pn_node::pn_node( const change& ch )
  : changes( 1, ch )
{
}

pn_node::pn_node( const method& m )
  : changes( m )
{
}

void pn_node::debug_print( ostream &os ) const
{
  copy( changes.begin(), changes.end(),
	ostream_iterator< change >( os, "." ) );
}

void pn_node::execute( proof_context &ctx, int dir ) const
{
  if (dir > 0) 
    for_each( changes.begin(), changes.end(), ctx.permute_and_prove() );
  else
    for_each( changes.rbegin(), changes.rend(), ctx.permute_and_prove() );
}

transp_node::transp_node( int bells, const string &r )
  : transp(bells)
{
  if ( bells == -1 )
    throw runtime_error( "Must set number of bells before using "
			 "transpostions" );
  
  transp *= r;
  if ( transp.bells() != bells )
    throw runtime_error( "Transposition is on the wrong number of bells" );
}

void transp_node::debug_print( ostream &os ) const
{
  os << "'" << transp << "'";
}

void transp_node::execute( proof_context &ctx, int dir ) const
{
  ctx.permute_and_prove()( transp );
}

void symbol_node::debug_print( ostream &os ) const
{
  os << sym;
}

void symbol_node::execute( proof_context &ctx, int dir ) const
{
  ctx.execute_symbol(sym, dir);
}

void assign_node::debug_print( ostream &os ) const
{
  os << "(" << defn.first << " = ";
  defn.second.debug_print(os);
  os << ")";
}

static void throw_no_backwards_execution(expression::node const& x)
{
  make_string os;
  os << "Unable to execute expression backwards: '";
  x.debug_print( os.out_stream() );
  os << "'";

  throw runtime_error( os );
}

void assign_node::execute( proof_context& ctx, int dir ) const
{
  if (dir < 0) throw_no_backwards_execution(*this);
  ctx.define_symbol(defn);
}

static void validate_regex( const music_details& desc, int bells )
{
  static string allowed;
  if ( allowed.empty() ) {
    allowed.append( row(bells).print() );
    allowed.append("*?[]");
  }

  string tok( desc.get() );

  if ( tok.find_first_not_of( allowed ) != string::npos )
    throw runtime_error( make_string() << "Illegal regular expression: " 
			 << tok );

  bool inbrack(false);
  for ( string::const_iterator i(tok.begin()), e(tok.end()); i!=e; ++i ) 
    switch (*i) {
    case '[':
      if ( inbrack ) 
	throw runtime_error( "Unexpected '[' in regular expressions" );
      inbrack = true;
      break;
    case ']':
      if ( !inbrack )
	throw runtime_error( "Unexpected ']' in regular expressions" );
      inbrack = false;
      break;
    case '*': case '?':
      if ( inbrack )
	throw runtime_error( "Cannot use '*' or '?' in a [] block "
			     "of a regular expression" );
      break;
    }

  // TODO: Check for multiple occurances of the same bell
}

bool isrounds_node::evaluate( proof_context& ctx ) const
{
  return ctx.isrounds();
}

void isrounds_node::debug_print( ostream &os ) const
{
  os << "rounds";
}

pattern_node::pattern_node( int bells, const string& regex )
  : bells(bells), mus( regex )
{
  validate_regex( mus, bells );
}

bool pattern_node::evaluate( proof_context &ctx ) const
{
  music m( bells, mus );
  m.process_row( ctx.current_row() );
  return m.get_score();
}

void pattern_node::debug_print( ostream &os ) const
{
  os << mus.get();
}

bool and_node::evaluate( proof_context &ctx ) const
{
  // short-circuits
  return left.evaluate(ctx) && right.evaluate(ctx);
}

void and_node::debug_print( ostream &os ) const
{
  left.debug_print(os);
  os << " && ";
  right.debug_print(os);
}

bool or_node::evaluate( proof_context &ctx ) const
{
  // short-circuits
  return left.evaluate(ctx) || right.evaluate(ctx);
}

void or_node::debug_print( ostream &os ) const
{
  left.debug_print(os);
  os << " || ";
  right.debug_print(os);
}


void if_match_node::debug_print( ostream &os ) const
{
  os << "{ ";
  test.debug_print(os);
  os << ": ";
  iftrue.debug_print(os);
  if ( !iffalse.isnull() ) {
    os << ";";
    iffalse.debug_print(os);
  }
  os << " }";
}

void if_match_node::execute( proof_context& ctx, int dir ) const
{
  if (dir < 0) throw_no_backwards_execution(*this);

  proof_context ctx2( ctx.silent_clone() );
  bool result = false;

  // We don't want exceptions from the test to terminate
  // the search. 
  // 
  // TODO  What about falseness?  Should we ignore
  // falseness during the test expresion?  
  //
  // For example, if I have
  //
  //   W = repeat { b, /1*8?/: b, break; p }
  //   H = repeat { b, /1*8/:  b, break; p }
  //   prove W,4H  // false: 3H comes round
  //
  // does this mean that the fourth H should be suppressed?
  //
  // Currently it does, and I think this is wrong.

  try {
    result = test.evaluate( ctx2 );
  } 
  catch ( script_exception const& ) {}

  if ( result )
    iftrue.execute( ctx, dir );
  else if ( !iffalse.isnull() )
    iffalse.execute( ctx, dir );
}

void exception_node::debug_print( ostream &os ) const
{
  os << "break";
}

void exception_node::execute( proof_context& ctx, int dir ) const
{
  if (dir < 0) throw_no_backwards_execution(*this);
  throw script_exception( t );
}

void load_method_node::debug_print( ostream& os ) const
{
  os << "load(\"" << name << "\")";
}

void load_method_node::execute( proof_context& ctx, int dir ) const
{
  if (!read) {
//    meth = load_method(name);
//    read = true;
  }

  if (meth.empty())
    throw runtime_error
      ( make_string() << "Unable to load method \"" << name << "\"" );

  if (dir > 0) 
    for_each( meth.begin(), meth.end(), ctx.permute_and_prove() );
  else
    for_each( meth.rbegin(), meth.rend(), ctx.permute_and_prove() );
}
