// -*- C++ -*- expression.cpp - classes to handle expressions
// Copyright (C) 2002, 2003, 2004, 2005, 2008, 2009, 2010, 2011, 2021
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

// Turn this on for parser debugging:
#define RINGING_DEBUG_FILE 0

#include "expression.h"
#include "exec.h"   // for exec_command
#include "output.h"
#include "format.h" // for argument_error
#include "tokeniser.h"
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <typeinfo.h> // for bad_cast
#include <stdexcept.h>
#include <functional.h>
#else
#include <vector>
#include <typeinfo> // for bad_cast
#include <stdexcept>
#include <functional>
#endif
#include <string>
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#include <iomanip.h>
#else
#if RINGING_DEBUG_FILE
#include <iostream> // for cout
#endif
#include <istream>
#include <iomanip>
#endif
#include <ringing/music.h>
#include <ringing/row.h>
#include <ringing/streamutils.h> // Takes care of <sstream>

#if RINGING_DEBUG_FILE
#define DEBUG( expr ) (void)((cout << expr) << endl)
#else
#define DEBUG( expr ) (void)(false)
#endif


RINGING_USING_NAMESPACE
RINGING_USING_STD

// A helper class to dump the whole of a range (e.g. of tokens)
RINGING_START_ANON_NAMESPACE

class division_by_zero : public argument_error {
public:
  division_by_zero() : argument_error("Division by zero") {}
};

#if RINGING_DEBUG_FILE
template <class It>
class output_range_t {
public:
  output_range_t(It first, It last) : first(first), last(last) {}
  
  friend ostream& operator<<( ostream& os, output_range_t const& r ) {
    copy( r.first, r.last, 
          ostream_iterator< typename iterator_traits<It>::value_type >
            ( os, " " ) );
    return os;
  }

private:
  It first, last;
};

template <class It>
output_range_t<It> output_range(It first, It last) {
  return output_range_t<It>( first, last );
}
#endif

RINGING_END_ANON_NAMESPACE

// ---------------------------------------------------------------------

expression::integer_type 
  expression::s_node::i_evaluate( const method_properties& m ) const
{
  return lexical_cast<expression::integer_type>( s_evaluate(m) );
}

string expression::i_node::s_evaluate( const method_properties& m ) const
{
  return lexical_cast<string>( i_evaluate(m) );
}

// ---------------------------------------------------------------------
//
// Nodes corresponding to each expression type
//
RINGING_START_ANON_NAMESPACE

// Class for a binary operator taking two strings and returning
// a string.
template <class BinaryOperator>
class s_binary_s_node : public expression::s_node {
public:
  s_binary_s_node( const shared_pointer<expression::node>& arg1,
		   const shared_pointer<expression::node>& arg2,
		   const BinaryOperator& op = BinaryOperator() )
    : op(op), arg1(arg1), arg2(arg2)
  {}

private:
  virtual string s_evaluate( const method_properties& m ) const {
    return op(arg1->s_evaluate(m), arg2->s_evaluate(m));
  }

  BinaryOperator op;
  shared_pointer<expression::node> arg1, arg2;
};

// Class for a binary operator taking two strings and returning
// an integer.
template <class BinaryOperator>
class s_binary_i_node : public expression::i_node {
public:
  s_binary_i_node( const shared_pointer<expression::node>& arg1,
		   const shared_pointer<expression::node>& arg2,
		   const BinaryOperator& op = BinaryOperator() )
    : op(op), arg1(arg1), arg2(arg2)
  {}

private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    return op(arg1->s_evaluate(m), arg2->s_evaluate(m));
  }

  BinaryOperator op;
  shared_pointer<expression::node> arg1, arg2;
};

// Class for a binary operator taking two integers and returning
// an integer.
template <class BinaryOperator>
class i_binary_i_node : public expression::i_node {
public:
  i_binary_i_node( const shared_pointer<expression::node>& arg1,
		   const shared_pointer<expression::node>& arg2,
		   const BinaryOperator& op = BinaryOperator() )
    : op(op), arg1(arg1), arg2(arg2)
  {}

private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    return op(arg1->i_evaluate(m), arg2->i_evaluate(m));
  }

  BinaryOperator op;
  shared_pointer<expression::node> arg1, arg2;
};

class match_node : public expression::i_node {
public:
  match_node( const shared_pointer<expression::node>& arg1,
              const shared_pointer<expression::node>& arg2 )
    : arg1(arg1), arg2(arg2) 
  {}

private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    row r( arg1->s_evaluate(m) );
    music mus( m.bells() );
    mus.push_back( music_details( arg2->s_evaluate(m) ) );
    return mus.process_row(r);
  }

  shared_pointer<expression::node> arg1, arg2;
};

// Not using i_binary_i_node< std::logical_and<bool> >
// as that will not short-circuit.
class logand_node : public expression::i_node {
public:
  logand_node( const shared_pointer<expression::node>& arg1,
	       const shared_pointer<expression::node>& arg2 )
    : arg1(arg1), arg2(arg2) 
  {}

private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    return arg1->i_evaluate(m) && arg2->i_evaluate(m);
  }

  shared_pointer<expression::node> arg1, arg2;
};

class logor_node : public expression::i_node {
public:
  logor_node( const shared_pointer<expression::node>& arg1,
	      const shared_pointer<expression::node>& arg2 )
    : arg1(arg1), arg2(arg2) 
  {}

private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    return arg1->i_evaluate(m) || arg2->i_evaluate(m);
  }

  shared_pointer<expression::node> arg1, arg2;
};

class ifelse_node : public expression::node {
public:
  ifelse_node( const shared_pointer<expression::node>& arg1,
	       const shared_pointer<expression::node>& arg2,
	       const shared_pointer<expression::node>& arg3 )
    : arg1( arg1 ), arg2( arg2 ), arg3( arg3 )
  {}

private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    return arg1->i_evaluate(m)
      ? arg2->i_evaluate(m) : arg3->i_evaluate(m);
  }

  virtual string s_evaluate( const method_properties& m ) const 
  {
    return arg1->i_evaluate(m)
      ? arg2->s_evaluate(m) : arg3->s_evaluate(m);
  }

  shared_pointer<expression::node> arg1, arg2, arg3;
};

class comma_node : public expression::node {
public:
  comma_node( const shared_pointer<expression::node>& arg1,
              const shared_pointer<expression::node>& arg2 )
    : arg1( arg1 ), arg2( arg2 )
  {}

private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    // Use s_evaluate instead of i_evaluate on arg1 as anything can be
    // converted into a string, but not necessarily into an integer.
    arg1->s_evaluate(m);
    return arg2->i_evaluate(m);
  }

  virtual string s_evaluate( const method_properties& m ) const 
  {
    arg1->s_evaluate(m);
    return arg2->s_evaluate(m);
  }

  shared_pointer<expression::node> arg1, arg2;
};

class string_node : public expression::s_node {
public:
  explicit string_node( const string& str ) : s(str) {}

private:
  virtual string s_evaluate( const method_properties& m ) const {
    return s;
  }

  string s;
};

class integer_node : public expression::i_node {
public:
  explicit integer_node( const string& str ) 
    : i( lexical_cast<expression::integer_type>(str) ) 
  {}

private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    return i;
  }

  expression::integer_type i;
};

class variable_node : public expression::s_node {
public:
  explicit variable_node( const string& str )
    : num_opt(0), num2_opt(0)
  {
    // Skip the leading dollar
    string::const_iterator begin( str.begin() ), end( str.end() );
    if ( begin != end && *begin == '$' ) ++begin;

    // Separate the num_opt out
    string::const_iterator iter(begin);
    while ( iter != end && isdigit(*iter) ) ++iter;
    if ( iter != begin )
      num_opt = atoi( string( begin, iter ).c_str() );

    if ( *iter == ',' ) {
      string::const_iterator iter2(iter);
      while ( iter != end && isdigit(*iter) ) ++iter;
      if ( iter != iter2 )
        num2_opt = atoi( string( iter2, iter ).c_str() );
    }

    // What's left must be the name
    name.assign( iter, end );
  }

private:
  virtual string s_evaluate( const method_properties& m ) const {
    if ( name == "*" )
      return expression_cache::evaluate( num_opt, m );
    else
      return m.get_property( make_pair(num_opt, num2_opt), name );
  }

  int num_opt, num2_opt;
  string name;
};

class exception_node : public expression::node {
public:
  explicit exception_node( script_exception::type_t t ) : t(t) {}
private:
  virtual expression::integer_type 
    i_evaluate( const method_properties& m ) const 
  {
    throw script_exception(t); return 0;
  }
  virtual string s_evaluate( const method_properties& m ) const 
  {
    throw script_exception(t); return ""; 
  }
  script_exception::type_t t;
};

class exec_expr_node : public expression::s_node {
public:
  exec_expr_node( const string& expr ) 
    : fs(expr, format_string::preparsed_type) 
  {}

  static int get_last_status() { return status; }
  static void clear_last_status() { status = 0; }

private:
  virtual string s_evaluate( const method_properties& m ) const {
    make_string ms;
    fs.print_method( m, ms.out_stream() );
    return exec_command( ms, &status );
  }

  format_string fs;
  static int status;
};

int exec_expr_node::status = 0;

RINGING_END_ANON_NAMESPACE

int get_last_exec_status()
{
  return exec_expr_node::get_last_status();
}

void clear_last_exec_status()
{
  return exec_expr_node::clear_last_status();
}

size_t store_exec_expression( const string& expr ) 
{
  return expression_cache::store( expression( new exec_expr_node( expr ) ) );
}

// ---------------------------------------------------------------------
//
// The expression parser
// 
class expression::parser
{
public:
  enum tok_types 
  { 
    name        = 'A',
    num_lit     = '0',
    open_paren  = '(',
    close_paren = ')',
    plus        = '+',
    minus       = '-',
    times       = '*',
    divide      = '/',
    modulo      = '%',
    question    = '?',
    colon       = ':',
    variable    = '$',
    concat      = '.',
    iless       = '<',
    igreater    = '>',
    comma       = ',',
    ilesseq    /* '<=' */ = tokeniser::first_token,
    igreatereq /* '>=' */,
    iequals    /* '==' */,
    inoteq     /* '!=' */,
    sless      /* 'lt' */,
    sgreater   /* 'gt' */,
    slesseq    /* 'le' */,
    sgreatereq /* 'ge' */,
    sequals    /* 'eq' */,
    snoteq     /* 'ne' */,
    logand     /* '&&' */,
    logor      /* '||' */,
    match      /* '~~' */,
    string_lit /* "..." */,
    kwd_suppress/* suppress */, 
    kwd_abort  /* abort */,
    subexpr    /* $[ ] */
  };

  parser();

  static vector<token> tokenise( string str );

  shared_pointer<expression::node> 
  make_node( vector<token>::const_iterator first,
	     vector<token>::const_iterator last );

private:
  shared_pointer<expression::node> 
  split_expr( vector<token>::const_iterator first,
	      vector<token>::const_iterator middle,
	      vector<token>::const_iterator last );

  shared_pointer<expression::node> 
  handle_left_infix( vector<token>::const_iterator first,
		     vector<token>::const_iterator last,
		     const vector<tok_types>& toks );

  class etokeniser;

  enum { precedence_levels = 7 };
  vector<tok_types> prec[(int)precedence_levels];
};

expression::parser::parser()
{
  prec[0].push_back(logor);  
  
  prec[1].push_back(logand);  
  
  prec[2].push_back(match);

  prec[3].push_back(iequals);  
  prec[3].push_back(inoteq);  
  prec[3].push_back(sequals);  
  prec[3].push_back(snoteq);  
  
  prec[4].push_back(iless);  
  prec[4].push_back(igreater);
  prec[4].push_back(ilesseq);  
  prec[4].push_back(igreatereq);
  prec[4].push_back(sless);  
  prec[4].push_back(sgreater);
  prec[4].push_back(slesseq);  
  prec[4].push_back(sgreatereq);
  
  prec[5].push_back(plus);  
  prec[5].push_back(minus);
  prec[5].push_back(concat);
  
  prec[6].push_back(times); 
  prec[6].push_back(divide);
  prec[6].push_back(modulo);
}

RINGING_START_ANON_NAMESPACE

template <class T> 
struct safe_divides : binary_function<T,T,T> {
  T operator() (const T& x, const T& y) const {
    if (y == T()) throw division_by_zero();
    return x/y;
  }
};

RINGING_END_ANON_NAMESPACE

shared_pointer<expression::node> 
expression::parser::split_expr( vector<token>::const_iterator first,
				vector<token>::const_iterator middle,
				vector<token>::const_iterator last )
{
  typedef shared_pointer<expression::node> ptr_t;

  if ( middle == last )
    throw logic_error( "Can't split at end of string" );

  if ( first == middle )
    throw argument_error
      ( make_string() << "Binary operator \"" << *middle 
	              << "\" needs first argument" );

  if ( middle+1 == last )
    throw argument_error
      ( make_string() << "Binary operator \"" << *middle 
	              << "\" needs second argument" );

  ptr_t arg1( make_node( first, middle ) ); // .....
  ptr_t arg2( make_node( middle+1, last ) );

  switch (middle->type())
    {
# define CASE( label, tmpl )  			\
    case label:		     			\
      DEBUG( "Splitting node at " #tmpl );	\
      return ptr_t	      			\
	( new i_binary_i_node< tmpl<expression::integer_type> >(arg1, arg2) )

      CASE( plus,       RINGING_PREFIX_STD plus          );
      CASE( minus,      RINGING_PREFIX_STD minus         );
      CASE( times,      RINGING_PREFIX_STD multiplies    );
      CASE( divide,                        safe_divides  );
      CASE( modulo,     RINGING_PREFIX_STD modulus       );
      CASE( iless,      RINGING_PREFIX_STD less          );
      CASE( igreater,   RINGING_PREFIX_STD greater       );
      CASE( ilesseq,    RINGING_PREFIX_STD less_equal    );
      CASE( igreatereq, RINGING_PREFIX_STD greater_equal );
      CASE( iequals,    RINGING_PREFIX_STD equal_to      );
      CASE( inoteq,     RINGING_PREFIX_STD not_equal_to  );

# undef CASE
# define CASE( label, tmpl )			\
    case label:		      			\
      DEBUG( "Splitting node at " #tmpl );	\
      return ptr_t	      			\
	( new s_binary_i_node< RINGING_PREFIX_STD tmpl<string> >(arg1, arg2) )

      CASE( sless,      less          );
      CASE( sgreater,   greater       );
      CASE( slesseq,    less_equal    );
      CASE( sgreatereq, greater_equal );
      CASE( sequals,    equal_to      );
      CASE( snoteq,     not_equal_to  );

# undef CASE
# define CASE( label, tmpl ) 			\
    case label:               			\
      DEBUG( "Splitting node at " #tmpl );	\
      return ptr_t            			\
	( new s_binary_s_node< RINGING_PREFIX_STD tmpl<string> >(arg1, arg2) )

      CASE( concat,     plus          );

#undef CASE

    case match:
      DEBUG( "Splitting node at ~~" );
      return ptr_t( new match_node(arg1, arg2) );

    case logand:
      DEBUG( "Splitting node at &&" );
      return ptr_t( new logand_node(arg1, arg2) );

    case logor:
      DEBUG( "Splitting node at ||" );
      return ptr_t( new logor_node(arg1, arg2) );

    case comma:
      DEBUG( "Spltting node at ," );
      return ptr_t( new comma_node(arg1, arg2) );
    }

  throw logic_error( "Attempted to split expression at an illegal token" );
  return ptr_t(); // Keep MSVC5 happy
}


shared_pointer<expression::node> 
expression::parser::handle_left_infix
  ( vector<token>::const_iterator first,
    vector<token>::const_iterator last,
    const vector<expression::parser::tok_types>& toks )
{
  // left associative -- so start from the right

  int depth = 0;
  vector< token >::const_iterator i(last-1);
  for (;;)
    {
      if ( i->type() == close_paren )
	++depth;
      else if ( i->type() == open_paren )
	--depth;
      else if ( depth == 0 )
	for ( vector< tok_types >::const_iterator
		pi(toks.begin()), pe(toks.end()); pi != pe; ++pi )
	  if ( i->type() == *pi ) {
            DEBUG( "Lowest precedent operator is tok_type " << (int)*pi );
	    return split_expr( first, i, last );
          }
      
      if (i == first) break; --i;
    }
  return shared_pointer<expression::node>(NULL);
}


shared_pointer<expression::node> 
expression::parser::make_node( vector<token>::const_iterator first,
			       vector<token>::const_iterator last )
{
  typedef vector< token >::const_iterator iter_t;
  typedef shared_pointer<expression::node> ptr_t;

  DEBUG( "Parsing token sequence: " << output_range(first, last) );

  if ( first == last ) 
    throw argument_error( "Expression expected" );

  // Parentheses first:
  if ( first->type() == open_paren && (last-1)->type() == close_paren )
    {
      int depth = 0;
      bool ok = true;
      for ( iter_t i(first); ok && i != last; ++i )
	{
	  if ( i->type() == open_paren )
	    ++depth;
	  else if ( i->type() == close_paren )
	    --depth;

	  if ( depth == 0 && i != last-1 ) 
	    ok = false;
	}

      // If the open_paren at the start of the sequence is not paired
      // with the close_paren at the end of the sequence, e.g. if we have
      //   ( $a == 0 ) && ( $b == 0 )
      // then ok will be false.
      if (ok)
	{
	  if (depth) 
	    throw argument_error( "Unmatched parentheses" );

	  if (first+1 == last-1 )
	    throw argument_error( "Empty parentheses" );

          DEBUG( "Striping outer parentheses" );
	  return make_node( first+1, last-1 );
	}

      // It's something like (1) + (2), so continue
    }

  // , is the lowest precedence operator and isn't in the table of binary
  // operators because it is lower precedence than ?: and so needs special
  // handling
  {
    vector<tok_types> toks( 1, comma );
    ptr_t result = handle_left_infix( first, last, toks );
    if ( result ) return result;
  }

  // ?: is the lowest precedence operator
  // It is right associative -- so start from the left looking for ?
  {
    int depth = 0;
    iter_t i(first), qmark;
    for (;;)
      {
	if ( i->type() == open_paren )
	  ++depth;
	else if ( i->type() == close_paren )
	  --depth;
	else if ( i->type() == question && depth == 0 )
	  { 
	    if ( i == first )
	      throw argument_error
		( "Ternary operator \"?:\" needs first argument" );

	    qmark = i; ++i;
	    break;
	  }

	if (++i == last) break;
      }

    // We have a ?, now look for a :
    if ( i != last )
      {
	for (;;)
	  {
	    if ( i->type() == open_paren )
	      ++depth;
	    else if ( i->type() == close_paren )
	      --depth;
	    else if ( i->type() == colon && depth == 0 )
	      {
		if ( i == qmark+1 )
		  throw argument_error
		    ( "Ternary operator \"?:\" needs second argument" );

		if ( i+1 == last )
		  throw argument_error
		    ( "Ternary operator \"?:\" needs third argument" );

                DEBUG( "Lowest precedence operator is ?:" );
		return ptr_t( new ifelse_node( make_node( first,   qmark ),
					       make_node( qmark+1, i ),
					       make_node( i+1,     last ) ) );
	      }

	    if (++i == last) break;
	  }

	throw argument_error( "Found \"?\" without \":\" in expression" );
      }

    if (depth) 
      throw argument_error( "Unmatched parentheses" );
  }
  
  // Handle infix binary operators
  for ( int lev=0; lev<precedence_levels; ++lev )
    {
      ptr_t result = handle_left_infix( first, last, prec[lev] );
      if ( result ) return result;
    }
  
  // Everything left is a literal of some sort
  if ( last - first != 1 )
    throw argument_error( "Parse error" );

  switch ( first->type() )
    {
    case num_lit:
      DEBUG( "Literal integer" );
      try {
        return ptr_t( new integer_node(*first) );
      } catch ( bad_lexical_cast const& ) { 
        throw argument_error( make_string() << "Overflow parsing '" 
                              << *first << "' as an integer" );
      }

    case variable:
      DEBUG( "Variable reference" );
      return ptr_t( new variable_node(*first) );

    case string_lit:
      DEBUG( "String literal" );
      return ptr_t( new string_node(*first) );

    case kwd_abort:
      DEBUG( "Keyword: abort" );
      return ptr_t( new exception_node( script_exception::abort_search ) );

    case kwd_suppress:
      DEBUG( "Keyword: suppress" );
      return ptr_t( new exception_node( script_exception::suppress_output ) );

    default:
      throw argument_error( "Unknown token in input" );
      return ptr_t(); // To keep MSVC 5 happy
    }
}

// ---------------------------------------------------------------------
//
// The expression tokeniser class
// 

class expression::parser::etokeniser : public tokeniser 
{
public:
  etokeniser( istream& in )
    : tokeniser(in), 
      q("\'", string_lit), qq("\"", string_lit),
      t1 ("<=", ilesseq), t2 (">=", igreatereq),
      t3 ("==", iequals), t4 ("!=", inoteq),
      t5 ("lt", sless),   t6 ("gt", sgreater),
      t7 ("le", slesseq), t8 ("ge", sgreatereq),
      t9 ("eq", sequals), t10("ne", snoteq),
      t11("&&", logand),  t12("||", logor),  t13("~~", match),
      k1 ("suppress", kwd_suppress),  
      k2 ("abort",   kwd_abort)
  {
    // NB: Must add se before v
    add_qtype(&v);   add_qtype(&q);   add_qtype(&qq); 
    add_qtype(&t1);  add_qtype(&t2);  add_qtype(&t3);  add_qtype(&t4);
    add_qtype(&t5);  add_qtype(&t6);  add_qtype(&t7);  add_qtype(&t8);
    add_qtype(&t9);  add_qtype(&t10); add_qtype(&t11); add_qtype(&t12);
    add_qtype(&t13); add_qtype(&k1);  add_qtype(&k2);
  }

  virtual void validate( const token& t ) const
  {
    switch ( t.type() )
      case name: case num_lit: case open_paren: case close_paren:
      case plus: case minus: case times: case divide: case modulo:
      case question: case colon: case variable: case concat:
      case iless: case igreater: case ilesseq: case igreatereq: 
      case sless: case sgreater: case slesseq: case sgreatereq: 
      case iequals: case inoteq: case sequals: case snoteq: 
      case logand: case logor: case match: case comma: case string_lit: 
      case kwd_suppress: case kwd_abort: case subexpr:
	return;
    
    throw argument_error( make_string() << "Unknown token in input: " << t );
  }

private:
  class var_impl : public tokeniser::basic_token
  {
  public:
    var_impl() : basic_token("$", variable) {}

  private:
    virtual parse_result parse( string::const_iterator& i, 
				string::const_iterator e, 
				token& tok ) const
    {
      string::const_iterator j = i;
      if ( *j != '$' ) return failed; ++j;
      while ( j < e && isdigit(*j) ) ++j;
      if ( j == e ) return more; // Or fail?
      // $N* is magic:
      if ( !isalpha(*j) && *j != '*' && *j != '#' && *j != '?' ) 
        return failed; ++j;
      tok.assign(i,j); tok.type( variable ); i = j;
      return done;
    }
  };

  var_impl v;
  string_token q, qq;
  basic_token t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13; // ops
  basic_token k1, k2; // keywords
};

vector<token> 
expression::parser::tokenise( string str ) // Not by reference
{
  str += '\n';

#if RINGING_USE_STRINGSTREAM
  istringstream src(str);
#else
  istrstream src(str.c_str());
#endif
  etokeniser tok(src);

  vector<token> toks;  
  for ( etokeniser::const_iterator i( tok.begin() ), e( tok.end() );
	i != e; ++i )
    {
      toks.push_back(*i);
      tok.validate( toks.back() );
    }

  return toks;
}


// ---------------------------------------------------------------------
//
// The expression class
// 

expression::expression( const string& str ) 
{
  parser p;
  DEBUG( "Parsing string: " << str );
  vector<token> toks( parser::tokenise(str) );
  pimpl = p.make_node( toks.begin(), toks.end() ); 
}

#define RINGING_CATCH_EXPRESSION_ERROR( retval )           \
  catch ( division_by_zero const& ) { return (retval); }   \
  catch ( bad_lexical_cast const& ) { return (retval); }   \
  catch ( bell::invalid    const& ) { return (retval); }   \
  catch ( row::invalid     const& ) { return (retval); }   \
  catch ( row_wildcard::invalid_pattern const& ) { return (retval); }

bool expression::b_evaluate( const method_properties& m ) const
{
  try 
    {
      return pimpl->i_evaluate(m); 
    }
  RINGING_CATCH_EXPRESSION_ERROR( false )
  
  catch ( const script_exception& sc )
    {
      switch ( sc.type() )
	{
	case script_exception::suppress_output:
	  return false;

	case script_exception::abort_search:
	  throw exit_exception();

	default:
	  throw;
	}
    }
}

string expression::evaluate( const method_properties& m ) const
{
  try 
    {
      return pimpl->s_evaluate(m); 
    }
  RINGING_CATCH_EXPRESSION_ERROR( expr_error_string )
}

#undef RINGING_CATCH_EXPRESSION_ERROR

vector<expression>& expression_cache::exprs() 
{
  static vector<expression> instance;
  return instance;
}

size_t expression_cache::store( const expression& expr )
{
  exprs().push_back( expr );
  return exprs().size() - 1;
}

string expression_cache::evaluate( size_t idx, 
				   const method_properties& props ) 
{
  return exprs().at(idx).evaluate(props);
}

bool expression_cache::b_evaluate( size_t idx, 
				   const method_properties& props ) 
{
  return exprs().at(idx).b_evaluate(props);
}
