// -*- C++ -*- expression.cpp - classes to handle expressions
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "expression.h"
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
#include <istream>
#include <iomanip>
#endif
#include <ringing/streamutils.h> // Takes care of <sstream>

RINGING_USING_NAMESPACE
RINGING_USING_STD

// ---------------------------------------------------------------------

long expression::s_node::i_evaluate( const method_properties& m ) const
{
  return lexical_cast<long>( s_evaluate(m) );
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
// an integer.
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
  virtual long i_evaluate( const method_properties& m ) const {
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
  virtual long i_evaluate( const method_properties& m ) const {
    return op(arg1->i_evaluate(m), arg2->i_evaluate(m));
  }

  BinaryOperator op;
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
  virtual long i_evaluate( const method_properties& m ) const {
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
  virtual long i_evaluate( const method_properties& m ) const {
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
  virtual long i_evaluate( const method_properties& m ) const 
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
  explicit integer_node( const string& str ) : i( lexical_cast<long>(str) ) {}

private:
  virtual long i_evaluate( const method_properties& m ) const {
    return i;
  }

  long i;
};

class variable_node : public expression::s_node {
public:
  explicit variable_node( const string& str )
    : num_opt(0)
  {
    // Skip the leading dollar
    string::const_iterator begin( str.begin() ), end( str.end() );
    if ( begin != end && *begin == '$' ) ++begin;

    // Separate the num_opt out
    string::const_iterator iter(begin);
    while ( iter != end && isdigit(*iter) ) ++iter;
    if ( iter != begin )
      num_opt = atoi( string( begin, iter ).c_str() );

    // What's left must be the name
    name.assign( iter, end );
  }

private:
  virtual string s_evaluate( const method_properties& m ) const {
    if ( name == "*" )
      return expression_cache::evaluate( num_opt, m );
    else
      return m.get_property( num_opt, name );
  }

  int num_opt;
  string name;
};

class exception_node : public expression::node {
public:
  explicit exception_node( script_exception::type_t t ) : t(t) {}
private:
  virtual long i_evaluate( const method_properties& m ) const {
    throw script_exception(t); return 0;
  }
  virtual string s_evaluate( const method_properties& m ) const {
    throw script_exception(t); return ""; 
  }
  script_exception::type_t t;
};


RINGING_END_ANON_NAMESPACE

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

  enum { precedence_levels = 6 };
  vector<tok_types> prec[(int)precedence_levels];
};

expression::parser::parser()
{
  prec[0].push_back(logor);  
  
  prec[1].push_back(logand);  
  
  prec[2].push_back(iequals);  
  prec[2].push_back(inoteq);  
  prec[2].push_back(sequals);  
  prec[2].push_back(snoteq);  
  
  prec[3].push_back(iless);  
  prec[3].push_back(igreater);
  prec[3].push_back(ilesseq);  
  prec[3].push_back(igreatereq);
  prec[3].push_back(sless);  
  prec[3].push_back(sgreater);
  prec[3].push_back(slesseq);  
  prec[3].push_back(sgreatereq);
  
  prec[4].push_back(plus);  
  prec[4].push_back(minus);
  prec[4].push_back(concat);
  
  prec[5].push_back(times); 
  prec[5].push_back(divide);
  prec[5].push_back(modulo);
}

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
# define CASE( label, tmpl )  \
    case label:		      \
      return ptr_t	      \
	( new i_binary_i_node< RINGING_PREFIX_STD tmpl<long> >(arg1, arg2) )

      CASE( plus,       plus          );
      CASE( minus,      minus         );
      CASE( times,      multiplies    );
      CASE( divide,     divides       );
      CASE( modulo,     modulus       );
      CASE( iless,      less          );
      CASE( igreater,   greater       );
      CASE( ilesseq,    less_equal    );
      CASE( igreatereq, greater_equal );
      CASE( iequals,    equal_to      );
      CASE( inoteq,     not_equal_to  );

# undef CASE
# define CASE( label, tmpl ) \
    case label:		      \
      return ptr_t	      \
	( new s_binary_i_node< RINGING_PREFIX_STD tmpl<string> >(arg1, arg2) )

      CASE( sless,      less          );
      CASE( sgreater,   greater       );
      CASE( slesseq,    less_equal    );
      CASE( sgreatereq, greater_equal );
      CASE( sequals,    equal_to      );
      CASE( snoteq,     not_equal_to  );

# undef CASE
# define CASE( label, tmpl ) \
    case label:               \
      return ptr_t            \
	( new s_binary_s_node< RINGING_PREFIX_STD tmpl<string> >(arg1, arg2) )

      CASE( concat,     plus          );

#undef CASE

    case logand:
      return ptr_t( new logand_node(arg1, arg2) );

    case logor:
      return ptr_t( new logor_node(arg1, arg2) );

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
	  if ( i->type() == *pi )
	    return split_expr( first, i, last );
      
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

      if (ok)
	{
	  if (depth) 
	    throw argument_error( "Unmatched parentheses" );

	  if (first+1 == last-1 )
	    throw argument_error( "Empty parentheses" );

	  return make_node( first+1, last-1 );
	}

      // It's something like (1) + (2), so continue
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
      return ptr_t( new integer_node(*first) );

    case variable:
      return ptr_t( new variable_node(*first) );

    case string_lit:
      return ptr_t( new string_node(*first) );

    case kwd_abort:
      return ptr_t( new exception_node( script_exception::abort_search ) );

    case kwd_suppress:
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
      t11("&&", logand),  t12("||", logor),
      k1 ("suppress", kwd_suppress),  
      k2 ("abort",   kwd_abort)
  {
    // NB: Must add se before v
    add_qtype(&v);  add_qtype(&q);   add_qtype(&qq); 
    add_qtype(&t1); add_qtype(&t2);  add_qtype(&t3);  add_qtype(&t4);
    add_qtype(&t5); add_qtype(&t6);  add_qtype(&t7);  add_qtype(&t8);
    add_qtype(&t9); add_qtype(&t10); add_qtype(&t11); add_qtype(&t12);
    add_qtype(&k1); add_qtype(&k2);
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
      case logand: case logor: case string_lit: 
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
      if ( !isalpha(*j) && *j != '*' ) return failed; ++j;
      tok.assign(i,j); tok.type( variable ); i = j;
      return done;
    }
  };

  var_impl v;
  string_token q, qq;
  basic_token t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12; // operators
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
  vector<token> toks( parser::tokenise(str) );
  pimpl = p.make_node( toks.begin(), toks.end() ); 
}

bool expression::b_evaluate( const method_properties& m ) const
{
  try 
    {
      return pimpl->i_evaluate(m); 
    }
  catch ( const bad_lexical_cast& )
    {
      return false;
    }
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
  catch ( const bad_lexical_cast& )
    {
      return "<ERROR>";
    }
}

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

