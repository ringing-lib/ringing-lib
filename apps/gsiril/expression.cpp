// expression.cpp - Nodes and factory function for expressions
// Copyright (C) 2002, 2003, 2004, 2005, 2008, 2009
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

#include <cstdlib>

#include <iterator>

#include "expression.h"
#include "proof_context.h"
#include <ringing/mathutils.h>
#include <ringing/streamutils.h>
#include <ringing/method.h>
#include <ringing/music.h>
#include <ringing/place_notation.h>


RINGING_USING_NAMESPACE


void list_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  os << "( ";
  car.debug_print( ctx, os );
  os << ", ";
  cdr.debug_print( ctx, os );
  os << " )";
}

void list_node::execute( proof_context &ctx )
{
  car.execute( ctx );
  cdr.execute( ctx );
}

bool list_node::evaluate( proof_context &ctx ) 
{
  car.execute( ctx );
  return cdr.evaluate( ctx );
}

expression::integer_t list_node::ievaluate( proof_context &ctx ) 
{
  car.execute( ctx );
  return cdr.ievaluate( ctx );
}

expression::type_t list_node::type( proof_context const& ctx ) const
{
  return cdr.type(ctx);
}

void nop_node::debug_print( proof_context const&, ostream &os ) const
{
  os << "{null}";
}

void nop_node::execute( proof_context & )
{
}

bool nop_node::isnop( proof_context const& ) const
{
  return true;
}

void repeated_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  if ( count.isnop(ctx) )
    cout << "repeat";
  else
    count.debug_print( ctx, os );
  os << ' ';
  child.debug_print( ctx, os );
}

void repeated_node::execute( proof_context &ctx )
{
  // Don't want to catch script exceptions from this
  expression::integer_t n;
  if ( !count.isnop(ctx) ) 
    n = count.ievaluate( ctx );

  // XXX Arguably we should only catch do_break in an arbitrarily-repeating
  // block, and not from a finitely-repeating one.
  try {
    if ( count.isnop(ctx) ) 
      while ( true )
	child.execute( ctx );
    else
      for (expression::integer_t i=0; i<n; ++i) 
	child.execute( ctx );
  } catch ( script_exception const& ex ) {
    if ( ex.t == script_exception::do_break ) 
      return;
    else
      throw;
  }
}

#define RINGING_GSIRIL_BINARY_INODE( name, symbol )                          \
                                                                             \
void name##_node::debug_print( proof_context const& ctx, ostream &os ) const \
{                                                                            \
  lhs.debug_print( ctx, os );                                                \
  cout << " " symbol " ";                                                    \
  rhs.debug_print( ctx, os );                                                \
}                                                                            \
                                                                             \
expression::integer_t name##_node::ievaluate( proof_context &ctx )

RINGING_GSIRIL_BINARY_INODE( ldivide, "\\" ) {
  return rhs.ievaluate(ctx) / lhs.ievaluate(ctx);
}

RINGING_GSIRIL_BINARY_INODE( exponent, "^" ) {
  return ipower( lhs.ievaluate(ctx), rhs.ievaluate(ctx) );
}

#define RINGING_GSIRIL_BINARY_INODE_STD( name, symbol )                      \
                                                                             \
RINGING_GSIRIL_BINARY_INODE( name, #symbol ) {                               \
  return lhs.ievaluate(ctx) symbol rhs.ievaluate(ctx);                       \
}
 
RINGING_GSIRIL_BINARY_INODE_STD( add,      + )
RINGING_GSIRIL_BINARY_INODE_STD( subtract, - )
RINGING_GSIRIL_BINARY_INODE_STD( multiply, * )
RINGING_GSIRIL_BINARY_INODE_STD( divide,   / )
RINGING_GSIRIL_BINARY_INODE_STD( modulus,  % )

#undef RINGING_GSIRIL_BINARY_INODE_STD
#undef RINGING_GSIRIL_BINARY_INODE

void factorial_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  arg.debug_print(ctx, os);
  os << "!";
}

expression::integer_t factorial_node::ievaluate( proof_context &ctx )
{
  return factorial( arg.ievaluate(ctx) );
}

expression 
  repeat_or_multiply_node::make_delegate( proof_context const& ctx ) const
{
  if ( rhs.type(ctx) == expression::no_type )
    return expression( new repeated_node( lhs, rhs ) );
  else
    return expression( new multiply_node( lhs, rhs ) );
}

void string_node::execute( proof_context &ctx )
{
  ctx.output_string(str);
}

void string_node::debug_print( proof_context const&, ostream &os ) const
{
  os << "\"" << str << "\"";
}

void bool_node::debug_print( proof_context const&, ostream& os ) const
{
  os << boolalpha << val;
}

bool bool_node::evaluate( proof_context &ctx )
{
  return val;
}

int_node::int_node( string const& str )
{
  try {
    val = lexical_cast<integer_t>(str);
  }
  catch ( bad_lexical_cast const& ) {
    throw runtime_error( make_string() << "Malformed-integer: " << str );
  }
}

void int_node::debug_print( proof_context const&, ostream &os ) const
{
  os << val;
}

expression::integer_t int_node::ievaluate( proof_context &ctx )
{
  return val;
}

void int_node::execute( proof_context& ctx )
{
  throw runtime_error( make_string() 
    << "Unexpected integer found: '" << val << "'.\n"
    << "Use +" << val << " to write a change as a piece of place notation." );
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

void pn_node::debug_print( proof_context const&, ostream &os ) const
{
  copy( changes.begin(), changes.end(),
	ostream_iterator< change >( os, "." ) );
}

void pn_node::execute( proof_context &ctx )
{
  for_each( changes.begin(), changes.end(), ctx.permute_and_prove() );
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

void transp_node::debug_print( proof_context const&, ostream &os ) const
{
  os << "'" << transp << "'";
}

void transp_node::execute( proof_context &ctx )
{
  ctx.permute_and_prove()( transp );
}

void symbol_node::debug_print( proof_context const&, ostream &os ) const
{
  os << sym;
}

void symbol_node::execute( proof_context &ctx )
{
  ctx.lookup_symbol(sym).execute(ctx);
}

bool symbol_node::evaluate( proof_context &ctx )
{
  return ctx.lookup_symbol(sym).evaluate(ctx);
}

expression::integer_t symbol_node::ievaluate( proof_context &ctx )
{
  return ctx.lookup_symbol(sym).ievaluate(ctx);
}

string symbol_node::lvalue( proof_context const& ctx ) const
{
  return sym;
}

expression::type_t symbol_node::type( proof_context const& ctx ) const
{
  return ctx.lookup_symbol(sym).type(ctx);
}

void assign_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  os << "(" << defn.first << " = ";
  defn.second.debug_print(ctx, os);
  os << ")";
}

void assign_node::execute( proof_context& ctx )
{
  // This is a little subtle.  If the right has a type, then
  // we want to evaluate it immediately and define the symbol with
  // the result of the evaluation.  This means that n=n+1 will do the 
  // expected thing and increment n.  However, if I do lh=+2, I want to 
  // defer execution of +2 until lh is executed.

  switch (type(ctx)) {
    case expression::no_type:
      ctx.define_symbol(defn); 
      break;

    case expression::boolean:
      ctx.define_symbol( make_pair( defn.first, 
        expression( new bool_node( defn.second.evaluate(ctx) ) ) ) );
      break;
 
    case expression::integer:
      ctx.define_symbol( make_pair( defn.first,
        expression( new int_node( defn.second.ievaluate(ctx) ) ) ) );
      break;

    default:
      abort();
  }
}

bool assign_node::evaluate( proof_context &ctx )
{
  execute(ctx);
  return defn.second.evaluate(ctx);
}

expression::integer_t assign_node::ievaluate( proof_context &ctx )
{
  execute(ctx);
  return defn.second.ievaluate(ctx);
}

expression::type_t assign_node::type( proof_context const& ctx ) const
{
  return defn.second.type(ctx);
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

bool isrounds_node::evaluate( proof_context& ctx )
{
  return ctx.isrounds();
}

void isrounds_node::debug_print( proof_context const&, ostream &os ) const
{
  os << "rounds";
}

pattern_node::pattern_node( int bells, const string& regex )
  : mus( bells )
{
  music_details md;
  md.set( regex );
  validate_regex( md, bells );
  mus.push_back( md );
}

bool pattern_node::evaluate( proof_context &ctx ) 
{
  // Pre-multiply by the inverse of the reference row
  row r(ctx.current_row());
  mus.process_rows( &r, &r + 1 );  // Ugh!
  return mus.get_score();
}

void pattern_node::debug_print( proof_context const&, ostream &os ) const
{
  os << mus.begin()->get();
}

bool and_node::evaluate( proof_context &ctx )
{
  // short-circuits
  return left.evaluate(ctx) && right.evaluate(ctx);
}

void and_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  left.debug_print(ctx, os);
  os << " && ";
  right.debug_print(ctx, os);
}

bool or_node::evaluate( proof_context &ctx )
{
  // short-circuits
  return left.evaluate(ctx) || right.evaluate(ctx);
}

void or_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  left.debug_print(ctx, os);
  os << " || ";
  right.debug_print(ctx, os);
}

bool not_node::evaluate( proof_context &ctx )
{
  return !arg.evaluate(ctx);
}

void not_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  os << "!";  
  arg.debug_print(ctx, os);
}

void if_match_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  os << "{ ";
  test.debug_print(ctx, os);
  os << ": ";
  iftrue.debug_print(ctx, os);
  if ( !iffalse.isnull() ) {
    os << ";";
    iffalse.debug_print(ctx, os);
  }
  os << " }";
}

void if_match_node::execute( proof_context& ctx )
{
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
    iftrue.execute( ctx );
  else if ( !iffalse.isnull() )
    iffalse.execute( ctx );
}

void exception_node::debug_print( proof_context const&, ostream &os ) const
{
  os << "break";
}

void exception_node::execute( proof_context& ctx )
{
  throw script_exception( t );
}

void load_method_node::debug_print( proof_context const&, ostream& os ) const
{
  os << "load(\"" << name << "\")";
}

void load_method_node::execute( proof_context& ctx )
{
  if (!read) {
//    meth = load_method(name);
//    read = true;
  }

  if (meth.empty())
    throw runtime_error
      ( make_string() << "Unable to load method \"" << name << "\"" );

  for_each( meth.begin(), meth.end(), ctx.permute_and_prove() );
}

void cmp_node_base::debug_print( proof_context const& ctx, ostream& os ) const
{
  lhs.debug_print(ctx, os);
  switch (op) {
    case eq: os << "=="; break;
    case ne: os << "!="; break;
    case lt: os << "<";  break;
    case le: os << "<="; break;
    case gt: os << ">";  break;
    case ge: os << ">="; break;
    default: abort();
  }
  rhs.debug_print(ctx, os);
}

RINGING_START_ANON_NAMESPACE

template <typename T>
static bool do_compare( cmp_node_base::operation op, T lhs, T rhs )
{
  switch (op) {
    case cmp_node_base::eq: return lhs == rhs;
    case cmp_node_base::ne: return lhs != rhs;
    case cmp_node_base::lt: return lhs <  rhs;
    case cmp_node_base::le: return lhs <= rhs;
    case cmp_node_base::gt: return lhs >  rhs;
    case cmp_node_base::ge: return lhs >= rhs;
    default: abort();
  }
}

RINGING_END_ANON_NAMESPACE

bool cmp_node_base::evaluate( proof_context& ctx )
{
  if ( lhs.type(ctx) == expression::no_type ) {
    make_string os;
    os << "Cannot compare void expression: '";
    lhs.debug_print( ctx, os.out_stream() );
    os << "'";
    throw runtime_error(os);
  }

  else if ( rhs.type(ctx) == expression::no_type ) {
    make_string os;
    os << "Cannot compare void expression: '";
    rhs.debug_print( ctx, os.out_stream() );
    os << "'";
    throw runtime_error(os);
  }
  
  // We don't want any int -> bool conversions here.  
  else if ( lhs.type(ctx) != rhs.type(ctx) )  {
    make_string os; 
    os << "Type mismatch while comparing '";
    lhs.debug_print( ctx, os.out_stream() );
    os << "' with '";
    rhs.debug_print( ctx, os.out_stream() );
    os << "'";
    throw runtime_error(os);
  }
  
  else switch ( lhs.type(ctx) ) {
    case expression::boolean:
      return do_compare( op, lhs.evaluate(ctx), rhs.evaluate(ctx) );

    case expression::integer:
      return do_compare( op, lhs.ievaluate(ctx), rhs.ievaluate(ctx) );

    default: abort();
  }
}

void incr_node::debug_print( proof_context const& ctx, ostream &os ) const
{
  if ( mode == prefix ) {
    if ( by == +1 ) os << "++"; else if ( by == -1 ) os << "--";
  }
  arg.debug_print(ctx, os);
  if ( mode == postfix ) {
    if ( by == +1 ) os << "++"; else if ( by == -1 ) os << "--";
  }
  if ( by != +1 && by != -1 ) 
    os << " += " << by;  // XXX This is misleading if we somehow allow
                         // creation of a postfix incr other than by +/- 1
}

expression::integer_t incr_node::ievaluate( proof_context &ctx )
{
  string const sym = arg.lvalue(ctx);
  integer_t val = arg.ievaluate(ctx);

  ctx.define_symbol( make_pair( sym, expression( new int_node(val+by) ) ) );

  return val + (mode == prefix ? by : 0);
}

// Allow the result of this to be discarded because it has an effect
void incr_node::execute( proof_context &ctx )
{
  ievaluate(ctx);
}
