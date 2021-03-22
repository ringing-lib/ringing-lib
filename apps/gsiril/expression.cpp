// expression.cpp - Nodes and factory function for expressions
// Copyright (C) 2002, 2003, 2004, 2005, 2008, 2011, 2014, 2019, 2020, 2021
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
#include <ringing/lexical_cast.h>


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

expression list_node::evaluate( proof_context &ctx ) const
{
  car.execute( ctx, +1 );
  return cdr.evaluate( ctx );
}

bool list_node::bool_evaluate( proof_context &ctx ) const
{
  car.execute( ctx, +1 );
  return cdr.bool_evaluate( ctx );
}

RINGING_LLONG list_node::int_evaluate( proof_context &ctx ) const
{
  car.execute( ctx, +1 );
  return cdr.int_evaluate( ctx );
}

string list_node::string_evaluate( proof_context &ctx ) const
{
  car.execute( ctx, +1 );
  return cdr.string_evaluate( ctx );
}


expression::type_t list_node::type( proof_context &ctx ) const
{
  return cdr.type(ctx);
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
  ctx.output_string(str, echo);
}

expression string_node::evaluate( proof_context &ctx ) const
{
  return expression( new string_node(str) );
}

string string_node::string_evaluate( proof_context &ctx ) const
{
  return str;
}

void string_node::debug_print( ostream &os ) const
{
  os << "\"" << str << "\"";
}
 
static int parse_int( string const& str ) {
  try {
    return lexical_cast<int>( str ); 
  } 
  catch (bad_lexical_cast) {
    throw runtime_error( make_string() << "Unable to parse \"" << str
                           << "\" as a bell expression" );
  }
}

static int parse_bell_expr( int bells, string const& expr ) {
  // At the moment, the grammar for a bell expression is as follows:
  //   BellExpr ::= "N" ("-" INT)? | INT

  string::const_iterator i(expr.begin()), e(expr.end());
  while (i != e && isspace(*i)) ++i;

  if (*i == 'N') {
    ++i; while (i != e && isspace(*i)) ++i;
    if ( i == e ) return bells;

    if ( *i != '-' ) 
      throw runtime_error( make_string() << "Invalid bell expression: "
                             "expected '-' after 'N' in " << expr );
    ++i; while (i != e && isspace(*i)) ++i;
    return bells - parse_int( string(i,e) ); 
  } 
  else return parse_int( string(i,e) ); 
}

static string expand_bell_exprs( int bells, string const& pn ) {
  make_string ret;
  for ( string::const_iterator i=pn.begin(), e=pn.end(); i!=e; ) {
    if (*i == '{') {
      string::const_iterator j = read_bell_expr(i, e);
      ret << bell( parse_bell_expr( bells, string(i+1,j-1) ) - 1 );
      i = j;
    }
    else ret << *i++;
  }
  return ret;
}

pn_node::pn_node( int bells, const string &raw_pn )
{
  if ( bells <= 0 )
    throw runtime_error( "Must set number of bells before using "
			 "place notation" );

  string pn( expand_bell_exprs(bells, raw_pn) );
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
  if ( bells <= 0 )
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

expression symbol_node::evaluate( proof_context &ctx ) const
{
   expression e( ctx.lookup_symbol(sym) );
   return e.evaluate(ctx);
}

bool symbol_node::bool_evaluate( proof_context &ctx ) const
{
   expression e( ctx.lookup_symbol(sym) );
   return e.bool_evaluate(ctx);
}

RINGING_LLONG symbol_node::int_evaluate( proof_context &ctx ) const
{
   expression e( ctx.lookup_symbol(sym) );
   return e.int_evaluate(ctx);
}

string symbol_node::string_evaluate( proof_context &ctx ) const
{
   expression e( ctx.lookup_symbol(sym) );
   return e.string_evaluate(ctx);
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

void append_assign_node::debug_print( ostream &os ) const
{
  os << "(" << sym << " .= ";
  val.debug_print(os);
  os << ")";
}

void append_assign_node::execute( proof_context& ctx, int dir ) const
{
  if (dir < 0) throw_no_backwards_execution(*this);
  ctx.define_symbol( make_pair(sym, apply(ctx.lookup_symbol(sym), val, ctx)) );
}

expression 
append_assign_node::apply( expression const& lhs, expression const& rhs,
                           proof_context& ctx ) const {
  string res( lhs.string_evaluate(ctx) + rhs.string_evaluate(ctx) );
  return expression( new string_node(res) );
}

void immediate_assign_node::debug_print( ostream &os ) const
{
  os << "(" << sym << " := ";
  val.debug_print(os);
  os << ")";
}

void immediate_assign_node::execute( proof_context& ctx, int dir ) const
{
  if (dir < 0) throw_no_backwards_execution(*this);
  ctx.define_symbol( make_pair(sym, val.evaluate(ctx)) );
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

bool isrounds_node::bool_evaluate( proof_context& ctx ) const
{
  return ctx.isrounds();
}

void isrounds_node::debug_print( ostream &os ) const
{
  os << "rounds";
}

pattern_node::pattern_node( int bells, const string& regex )
  : bells(bells), mus( expand_bell_exprs(bells, regex) )
{
  validate_regex( mus, bells );
}

bool pattern_node::bool_evaluate( proof_context &ctx ) const
{
  music m( bells, mus );
  m.process_row( ctx.current_row() );
  return m.get_score();
}

void pattern_node::debug_print( ostream &os ) const
{
  os << mus.get();
}

void boolean_node::debug_print( ostream &os ) const
{
  os << ( value ? "true" : "false" );
}


bool and_node::bool_evaluate( proof_context &ctx ) const
{
  // short-circuits
  return left.bool_evaluate(ctx) && right.bool_evaluate(ctx);
}

void and_node::debug_print( ostream &os ) const
{
  left.debug_print(os);
  os << " && ";
  right.debug_print(os);
}

bool or_node::bool_evaluate( proof_context &ctx ) const
{
  // short-circuits
  return left.bool_evaluate(ctx) || right.bool_evaluate(ctx);
}

void or_node::debug_print( ostream &os ) const
{
  left.debug_print(os);
  os << " || ";
  right.debug_print(os);
}

bool not_node::bool_evaluate( proof_context &ctx ) const
{
  return !arg.bool_evaluate(ctx);
}

void not_node::debug_print( ostream &os ) const
{
  os << "!";
  arg.debug_print(os);
}

bool cmp_node::bool_evaluate( proof_context &ctx ) const
{
  expression::type_t lt = left.type(ctx), rt = right.type(ctx);

  if (lt == expression::boolean && rt == expression::boolean) {
    bool l = left.bool_evaluate(ctx), r = right.bool_evaluate(ctx);
    switch (cmp) {
      case equals:     return l == r;
      case not_equals: return l != r;
      default: throw runtime_error("Cannot compare booleans");
    }
  }
  else if (lt == expression::boolean || rt == expression::boolean) {
    throw runtime_error("Cannot compare booleans and non-booleans");
  }
  else if (lt == expression::integer && rt == expression::integer) {
    RINGING_LLONG l = left.int_evaluate(ctx), r = right.int_evaluate(ctx);
    switch (cmp) {
      case equals:     return l == r;
      case not_equals: return l != r;
      case greater:    return l > r;
      case less:       return l < r;
      case greater_eq: return l >= r;
      case less_eq:    return l <= r;
      default:         abort();
    }
  }
  else {
    string l = left.string_evaluate(ctx), r = right.string_evaluate(ctx);
    switch (cmp) {
      case equals:     return l == r;
      case not_equals: return l != r;
      case greater:    return l > r;
      case less:       return l < r;
      case greater_eq: return l >= r;
      case less_eq:    return l <= r;
      default:         abort();
    }
  }
}

char const* cmp_node::symbol( cmp_t cmp ) {
  switch (cmp) {
    case equals:     return "==";
    case not_equals: return "!=";
    case greater:    return ">";
    case less:       return "<";
    case greater_eq: return ">=";
    case less_eq:    return "<=";
    default:         abort();
  }
}

void cmp_node::debug_print( ostream &os ) const
{
  left.debug_print(os);
  os << symbol(cmp);
  right.debug_print(os);
}

void bells_node::debug_print( ostream &os ) const
{
  os << "bells";
}

RINGING_LLONG bells_node::int_evaluate( proof_context& ctx ) const
{
  return ctx.bells();
}

void integer_node::debug_print( ostream &os ) const
{
  os << value;
}

RINGING_LLONG increment_node::int_evaluate( proof_context& ctx ) const
{
  RINGING_LLONG res( ctx.lookup_symbol(sym).int_evaluate(ctx) + val );
  ctx.define_symbol( make_pair( sym, expression( new integer_node(res) ) ) );
  return res;
}

void increment_node::debug_print( ostream &os ) const
{
  os << "(" << sym << " += " << val << ")";
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
    result = test.bool_evaluate( ctx2 );
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

void call_node::debug_print( ostream& os ) const
{
  os << name << '(';
  bool first = true;
  for (vector<expression>::const_iterator i=args.begin(), e=args.end();
       i != e; ++i) {
    if (!first) os << ", ";
    i->debug_print(os);
    first = false;
  }
  os << ')';
}

void call_node::execute( proof_context& ctx, int dir ) const
{
  if (dir < 0) throw_no_backwards_execution(*this);
  evaluate(ctx).execute(ctx, dir);
}

expression call_node::evaluate( proof_context &ctx ) const
{
  expression e( ctx.lookup_symbol(name) );
  return e.call(ctx, args);
}

bool call_node::bool_evaluate( proof_context &ctx ) const
{
  return evaluate(ctx).bool_evaluate(ctx);
}

RINGING_LLONG call_node::int_evaluate( proof_context &ctx ) const
{
  return evaluate(ctx).int_evaluate(ctx);
}

string call_node::string_evaluate( proof_context &ctx ) const
{
  return evaluate(ctx).string_evaluate(ctx);
}

expression::type_t call_node::type( proof_context &ctx ) const
{
  return ctx.lookup_symbol(name).type(ctx);
}

