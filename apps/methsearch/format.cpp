// -*- C++ -*- format.cpp - classes to handle format specifiers
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

// Parts of this file are taken from the boost lexical_cast library [see
// http://www.boost.org for details], and are under the following copyright:

//  Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
//  Permission to use, copy, modify, and distribute this software for any
//  purpose is hereby granted without fee, provided that this copyright and
//  permissions notice appear in all copies and derivatives.
//
//  This software is provided "as is" without express or implied warranty.

// $Id$


#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include "format.h"
#include "libraries.h"
#include "methodutils.h"
#include "falseness.h"
#include "music.h"
#include "tokeniser.h"
#if RINGING_OLD_INCLUDES
#include <iterator.h>
#include <algo.h>
#include <map.h>
#include <stdexcept.h>
#include <typeinfo.h> // for bad_cast
#include <functional.h>
#else
#include <iterator>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <typeinfo> // for bad_cast
#include <functional>
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#include <iomanip.h>
#else
#include <ostream>
#include <iomanip>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <assert.h>
#else
#include <cctype>
#include <cassert>
#endif
#include <ringing/streamutils.h>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/pointers.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD


// ---------------------------------------------------------------------
//
// Lexical cast stuff.  Should this be moved to streamutils.h?
//
RINGING_START_ANON_NAMESPACE

// exception used to indicate runtime lexical_cast failure
class bad_lexical_cast : public bad_cast
{
public:
  virtual const char* what() const throw()
  {
    return "bad cast: "
      "source type value could not be interpreted as target";
  }
};

// Last function argument is to make MSVC happy.
// This is important as omitting it does not give a compile time error
template<typename Target, typename Source>
Target lexical_cast(Source arg, Target* = 0)
{
#if RINGING_USE_STRINGSTREAM
  stringstream interpreter;
#else
  strstream interpreter; // for out-of-the-box g++ 2.95.2
# endif
  Target result;
  
  if(!(interpreter << arg) || !(interpreter >> result) ||
     !(interpreter >> ws).eof())
    throw bad_lexical_cast();
  
  return result;
}


RINGING_END_ANON_NAMESPACE

// ---------------------------------------------------------------------
//
// Nodes corresponding to each expression type
//
RINGING_START_ANON_NAMESPACE

class s_node : public expression::node {
  virtual long i_evaluate( const method& m ) const {
    return lexical_cast<long>( s_evaluate(m) );
  }
};

class i_node : public expression::node {
  virtual string s_evaluate( const method& m ) const {
    return lexical_cast<string>( i_evaluate(m) );
  }
};

template <class BinaryOperator>
class s_binary_i_node : public i_node {
public:
  s_binary_i_node( const shared_pointer<expression::node>& arg1,
		   const shared_pointer<expression::node>& arg2,
		   const BinaryOperator& op = BinaryOperator() )
    : op(op), arg1(arg1), arg2(arg2)
  {}

private:
  virtual long i_evaluate( const method& m ) const {
    return op(arg1->s_evaluate(m), arg2->s_evaluate(m));
  }

  BinaryOperator op;
  shared_pointer<expression::node> arg1, arg2;
};

template <class BinaryOperator>
class i_binary_i_node : public i_node {
public:
  i_binary_i_node( const shared_pointer<expression::node>& arg1,
		   const shared_pointer<expression::node>& arg2,
		   const BinaryOperator& op = BinaryOperator() )
    : op(op), arg1(arg1), arg2(arg2)
  {}

private:
  virtual long i_evaluate( const method& m ) const {
    return op(arg1->i_evaluate(m), arg2->i_evaluate(m));
  }

  BinaryOperator op;
  shared_pointer<expression::node> arg1, arg2;
};

// Not using i_binary_i_node< std::logical_and<bool> >
// as that will not short-circuit.
class logand_node : public i_node {
public:
  logand_node( const shared_pointer<expression::node>& arg1,
	       const shared_pointer<expression::node>& arg2 )
    : arg1(arg1), arg2(arg2) 
  {}

private:
  virtual long i_evaluate( const method& m ) const {
    return arg1->i_evaluate(m) && arg2->i_evaluate(m);
  }

  shared_pointer<expression::node> arg1, arg2;
};

class logor_node : public i_node {
public:
  logor_node( const shared_pointer<expression::node>& arg1,
	      const shared_pointer<expression::node>& arg2 )
    : arg1(arg1), arg2(arg2) 
  {}

private:
  virtual long i_evaluate( const method& m ) const {
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
  virtual long i_evaluate( const method& m ) const {
    return arg1->i_evaluate(m) ? arg2->i_evaluate(m) : arg3->i_evaluate(m);
  }

  virtual string s_evaluate( const method& m ) const {
    return arg1->i_evaluate(m) ? arg2->s_evaluate(m) : arg3->s_evaluate(m);
  }

  shared_pointer<expression::node> arg1, arg2, arg3;
};

class string_node : public s_node {
public:
  explicit string_node( const string& str ) : s(str) {}

private:
  virtual string s_evaluate( const method& m ) const {
    return s;
  }

  string s;
};

class integer_node : public i_node {
public:
  explicit integer_node( const string& str ) : i( lexical_cast<long>(str) ) {}

private:
  virtual long i_evaluate( const method& m ) const {
    return i;
  }

  long i;
};

class variable_node : public s_node {
public:
  explicit variable_node( const string& var ) 
    : fs(make_string() << var << "\\", format_string::normal_type) {}

private:
  virtual string s_evaluate( const method& m ) const; // Implemented below

  format_string fs;
};

class script_exception 
{
public:
  enum type_t { suppress_output, abort_search };
  explicit script_exception( type_t t ) : t(t) {}
  type_t type() const { return t; }

private:
  type_t t;
};

class exception_node : public expression::node {
public:
  explicit exception_node( script_exception::type_t t ) : t(t) {}
private:
  virtual long i_evaluate( const method& m ) const {
    throw script_exception(t); return 0;
  }
  virtual string s_evaluate( const method& m ) const {
    throw script_exception(t); return ""; 
  }
  script_exception::type_t t;
};


RINGING_END_ANON_NAMESPACE

// ---------------------------------------------------------------------
//
// The parser itself
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
    kwd_abort  /* abort */
  };

  shared_pointer<expression::node> 
  static run( const string& str )
  {
    vector<token> toks( tokenise(str) );
    parser p;
    return p.make_node( toks.begin(), toks.end() ); 
  }

private:
  parser();

  class etokeniser;
  static vector<token> tokenise( string str );

  shared_pointer<expression::node> 
  make_node( vector<token>::const_iterator first,
	     vector<token>::const_iterator last );

  shared_pointer<expression::node> 
  split_expr( vector<token>::const_iterator first,
	      vector<token>::const_iterator middle,
	      vector<token>::const_iterator last );

  shared_pointer<expression::node> 
  handle_left_infix( vector<token>::const_iterator first,
		     vector<token>::const_iterator last,
		     const vector<tok_types>& toks );

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
  
  prec[5].push_back(times); 
  prec[5].push_back(divide);
  prec[5].push_back(modulo);
}

expression::expression( const string& str ) 
  : pimpl( parser::run(str) )
{}

bool expression::b_evaluate( const method& m ) const
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

string expression::evaluate( const method& m ) const
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


class expression::parser::etokeniser : public tokeniser 
{
public:
  etokeniser( istream& in )
    : tokeniser(in), 
      v1("$"), v2("%"), 
      q("\'",  string_lit), qq("\"",  string_lit),
      t1 ("<=", ilesseq), t2 (">=", igreatereq),
      t3 ("==", iequals), t4 ("!=", inoteq),
      t5 ("lt", sless),   t6 ("gt", sgreater),
      t7 ("le", slesseq), t8 ("ge", sgreatereq),
      t9 ("eq", sequals), t10("ne", snoteq),
      t11("&&", logand),  t12("||", logor),
      k1 ("suppress", kwd_suppress),  
      k2 ("abort",   kwd_abort)
  {
    add_qtype(&v1); add_qtype(&v2);  add_qtype(&q);   add_qtype(&qq); 
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
      case question: case colon: case variable: 
      case iless: case igreater: case ilesseq: case igreatereq: 
      case sless: case sgreater: case slesseq: case sgreatereq: 
      case iequals: case inoteq: case sequals: case snoteq: 
      case logand: case logor: case string_lit: 
      case kwd_suppress: case kwd_abort:
	return;
    
    throw argument_error( make_string() << "Unknown token in input: " << t );
  }

private:
  class var_impl : public tokeniser::basic_token
  {
  public:
    // ch should be "$" or "%"
    var_impl(const char* ch) : basic_token(ch, variable) {}

  private:
    virtual parse_result parse( string::const_iterator& i, 
				string::const_iterator e, 
				token& tok ) const
    {
      string::const_iterator j = i;
      if ( *j != '$' ) return failed; ++j;
      while ( j < e && isdigit(*j) ) ++j;
      if ( j == e ) return more; // Or fail?
      if ( !isalpha(*j) ) return failed; ++j;
      tok.assign(i,j); tok.type( variable ); i = j;
      return done;
    }
  };

  var_impl v1, v2;
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
# define ICASE( label, tmpl ) \
    case label:		      \
      return ptr_t	      \
	( new i_binary_i_node< RINGING_PREFIX_STD tmpl<long> >(arg1, arg2) )

      ICASE( plus,       plus          );
      ICASE( minus,      minus         );
      ICASE( times,      multiplies    );
      ICASE( divide,     divides       );
      ICASE( modulo,     modulus       );
      ICASE( iless,      less          );
      ICASE( igreater,   greater       );
      ICASE( ilesseq,    less_equal    );
      ICASE( igreatereq, greater_equal );
      ICASE( iequals,    equal_to      );
      ICASE( inoteq,     not_equal_to  );

# undef ICASE
# define SCASE( label, tmpl ) \
    case label:		      \
      return ptr_t	      \
	( new s_binary_i_node< RINGING_PREFIX_STD tmpl<string> >(arg1, arg2) )

      SCASE( sless,      less          );
      SCASE( sgreater,   greater       );
      SCASE( slesseq,    less_equal    );
      SCASE( sgreatereq, greater_equal );
      SCASE( sequals,    equal_to      );
      SCASE( snoteq,     not_equal_to  );

# undef SCASE

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


class histogram_entry
{
public:
  struct cmp
  { 
    typedef bool value_type;
    typedef histogram_entry first_argument_type;
    typedef histogram_entry second_argument_type;
    
    bool operator()( const histogram_entry &x, 
		     const histogram_entry &y ) const;
  };

  histogram_entry( const format_string &f, const method &m );

  void print( ostream &os, size_t count ) const;

  RINGING_FAKE_DEFAULT_CONSTRUCTOR( histogram_entry );

private:
  friend struct cmp;

  const format_string &f;

  row lead_head;
  string pn, compressed_pn;
  map< size_t, row > rows;
  map< size_t, string > paths;
  method changes;
  unsigned int blow_count, lh_order;
  string lh_code;
  string name, full_name;
  string class_name, stage_name;
  int music_score;
  string falseness_group;
};

RINGING_START_ANON_NAMESPACE

string variable_node::s_evaluate( const method& m ) const
{
  make_string ms;
  histogram_entry( fs, m ).print( ms.out_stream(), 0 );
  return ms;
}

RINGING_END_ANON_NAMESPACE

bool histogram_entry::cmp::operator()( const histogram_entry &x, 
				       const histogram_entry &y ) const
{
  assert( &x.f == &y.f );

  if ( x.f.has_lead_head )
    {
      if ( x.lead_head < y.lead_head )
	return true;
      else if ( x.lead_head > y.lead_head )
	return false;
    }
  
  if ( x.f.has_pn )
    {
      if ( x.pn < y.pn )
	return true;
      else if ( x.pn > y.pn )
	return false;
    }

  if ( x.f.has_compressed_pn )
    {
      if ( x.compressed_pn < y.compressed_pn )
	return true;
      else if ( x.compressed_pn > y.compressed_pn )
	return false;
    }

  if ( x.f.has_rows.size() )
    for ( vector<size_t>::const_iterator i( x.f.has_rows.begin() ), 
	    e( x.f.has_rows.end() ); i != e; ++i )
      {
	if ( *x.rows.find(*i) < *y.rows.find(*i) )
	  return true;
	else if ( *x.rows.find(*i) > *y.rows.find(*i) )
	  return false;
      }

  if ( x.f.has_changes.size() )
    for ( vector<size_t>::const_iterator i( x.f.has_changes.begin() ), 
	    e( x.f.has_changes.end() ); i != e; ++i )
      {
	if ( x.changes[*i-1] < y.changes[*i-1] )
	  return true;
	else if ( x.changes[*i-1] > y.changes[*i-1] )
	  return false;
      }

  if ( x.f.has_path.size() )
    for ( vector<size_t>::const_iterator i( x.f.has_path.begin() ), 
	    e( x.f.has_path.end() ); i != e; ++i )
      {
	if ( *x.paths.find(*i) < *y.paths.find(*i) )
	  return true;
	else if ( *x.paths.find(*i) > *y.paths.find(*i) )
	  return false;	 
      }

  if ( x.f.has_blow_count )
    {
      if ( x.blow_count < y.blow_count )
	return true;
      else if ( x.blow_count > y.blow_count )
	return false;
    }

  if ( x.f.has_lh_order )
    {
      if ( x.lh_order < y.lh_order )
	return true;
      else if ( x.lh_order > y.lh_order )
	return false;
    }

  if ( x.f.has_lh_code )
    {
      if ( x.lh_code < y.lh_code )
	return true;
      else if ( x.lh_code > y.lh_code )
	return false;
    }

  if ( x.f.has_name )
    {
      if ( x.name < y.name )
	return true;
      else if ( x.name > y.name )
	return false;
    }

  if ( x.f.has_full_name )
    {
      if ( x.full_name < y.full_name )
	return true;
      else if ( x.full_name > y.full_name )
	return false;
    }

  if ( x.f.has_stage_name )
    {
      if ( x.stage_name < y.stage_name )
	return true;
      else if ( x.stage_name > y.stage_name )
	return false;
    }

  if ( x.f.has_class_name )
    {
      if ( x.class_name < y.class_name )
	return true;
      else if ( x.class_name > y.class_name )
	return false;
    }

  if ( x.f.has_music_score )
    {
      if ( x.music_score < y.music_score )
	return true;
      else if ( x.music_score > y.music_score )
	return false;
    }

  if ( x.f.has_falseness_group )
    {
      if ( x.falseness_group < y.falseness_group )
	return true;
      else if ( x.falseness_group > y.falseness_group )
	return false;
    }

  // They're equal
  return false;
}

histogram_entry::histogram_entry( const format_string &f, const method &m )
  : f( f ),
    music_score(0)
{
  if ( f.has_lead_head )
    lead_head = m.lh();

  if ( f.has_pn )
    {
      make_string tmp;
      for ( method::const_iterator i(m.begin()), e(m.end()); i!=e; ++i )
	tmp << *i << ".";
      pn = tmp;
    }

  if ( f.has_compressed_pn )
    {
      compressed_pn = get_compressed_pn(m);
    }

  if ( f.has_rows.size() )
    for ( vector<size_t>::const_iterator i( f.has_rows.begin() ), 
	    e( f.has_rows.end() ); i != e; ++i )
      {
	rows[*i] = row( m.bells() );
	for ( method::const_iterator i2( m.begin() ), 
		e2( m.begin() + *i ); i2 != e2; ++i2 )
	  rows[*i] *= *i2;
      }

  if ( f.has_changes.size() || f.has_expression )
    changes = m;

  if ( f.has_path.size() )
    for ( vector<size_t>::const_iterator i( f.has_path.begin() ), 
	    e( f.has_path.end() ); i != e; ++i )
      {
	make_string os; bell b(*i-1); os << b;
	for ( method::const_iterator i2( m.begin() ), 
		e2( m.end() ); i2 != e2; ++i2 )
	  os << (b *= *i2);
	paths[*i] = os;
      }

  if ( f.has_blow_count )
    blow_count = max_blows_per_place( m );

  if ( f.has_lh_order )
    lh_order = m.leads();

  if ( f.has_lh_code )
    lh_code = m.lhcode();

  if ( f.has_name || f.has_full_name )
    {
      const method &m2 = method_libraries::lookup_method( m );

      if ( f.has_name ) name = m2.name();
      if ( f.has_full_name ) full_name = m2.fullname();
    }

  if ( f.has_class_name )
    class_name = method::classname( m.methclass() );

  if ( f.has_stage_name )
    stage_name = method::stagename( m.bells() );

  if ( f.has_music_score )
    music_score = musical_analysis::analyse( m );

  if ( f.has_falseness_group )
    falseness_group = falseness_group_codes( m );
}


void histogram_entry::print( ostream &os2, size_t count ) const
{
  make_string os;

  try 
    {
      // State
      bool in_expr(false);   // Are we inside a $[ ... ]?
      size_t parens(0u);     // Depth of ()s within a $[ ... ].
      string::const_iterator expr_start;  // Start of the $[ ... ] expression.

      for ( string::const_iterator iter( f.fmt.begin() ),
	      end( f.fmt.end() ); iter != end; ++iter )
	{
	  if ( *iter == '$' || !in_expr && *iter == '%' ) 
	    {
	      string::const_iterator iter2(++iter);
	      while ( iter != end && isdigit(*iter) )
		++iter;

	      int num_opt = 0;
	      if ( iter2 != iter )
		num_opt = atoi( string( &*iter2, &*iter ).c_str() );

	      if (*iter == '[') { 
		expr_start = iter+1; 
		in_expr = true; 
	      }


	      if ( !in_expr ) switch ( *iter )
		{
		case '%': os << '%'; break;
		case '$': os << '$'; break;
		case 'c': os << setw(num_opt) << count; break;
		case 'l': os << lead_head;  break;
		case 'p': os << pn; break;
		case 'q': os << compressed_pn; break;
		case 'r': os << rows.find( num_opt )->second; break;
		case 'h': os << changes[ num_opt-1 ]; break;
		case 'b': os << setw(num_opt) << blow_count; break;
		case 'o': os << setw(num_opt) << lh_order; break;
		case 'd': os << lh_code; break;
		case 'n': os << name; break;
		case 'N': os << full_name; break;
		case 'C': os << class_name; break;
		case 'S': os << stage_name; break;
		case 'M': os << setw(num_opt) << music_score; break;
		case 'F': os << falseness_group; break;
		case 'P': os << paths.find( num_opt )->second; break;
		}
	    }
	  else if ( *iter == '\\' )
	    {
	      switch ( *++iter )
		{
		case 't': os << '\t';  break;
		case 'n': os << '\n';  break;
		default:  os << *iter; break;
		}
	    }
	  else if ( *iter == '(' && in_expr )
	    {
	      ++parens;
	    }
	  else if ( *iter == ')' && in_expr )
	    {
	      if ( parens == 0 )
		throw argument_error
		  ( "Unmatched parenthesis in $[ ] expression" );
	      --parens;
	    }
	  else if ( *iter == ']' && in_expr )
	    {
	      if ( parens > 0 )
		throw argument_error
		  ( "Unmatched parenthesis in $[ ] expression" );
	      in_expr = false;
	  
	      map<string, expression>::const_iterator expr
		= f.exprs.find( string( expr_start, iter ) );

	      if ( expr == f.exprs.end() )
		throw logic_error
		  ( "Internal error: Unable to locate expression" );
	      os << expr->second.evaluate( changes );
	    }
	  else if (!in_expr)
	    os << *iter;
	}
    }
  catch ( const script_exception& sc )
    {
      switch ( sc.type() )
	{
	case script_exception::suppress_output:
	  return;

	case script_exception::abort_search:
	  os2 << string(os);
	  if (f.line_break) os2 << "\n";
	  os2 << flush;	  
	  throw exit_exception();
	  
	default:
	  throw;
	}
    }

  os2 << string(os);
  if (f.line_break) os2 << "\n";
  os2 << flush;
}


void format_string::add_method_to_stats( const method &m ) const
{
  statistics::add_entry( histogram_entry( *this, m ) );
}

void format_string::print_method( const method &m, ostream &os ) const
{
  histogram_entry( *this, m ).print( os, 0 );
}

format_string::format_string( const string &fmt, 
			      format_string::format_type type )
  : has_lead_head(false),
    has_pn(false),
    has_compressed_pn(false),
    has_blow_count(false),
    has_lh_order(false),
    has_lh_code(false),

    has_name(false),
    has_full_name(false),
    has_class_name(false),
    has_stage_name(false),

    has_music_score(false),
    has_falseness_group(false),

    line_break(true),

    has_expression(false),

    fmt(fmt)
{
  assert( type == stat_type || type == normal_type );

  // State
  bool in_expr(false);   // Are we inside a $[ ... ]?
  size_t parens(0u);     // Depth of ()s within a $[ ... ].
  string::const_iterator expr_start;  // Start of the $[ ... ] expression.

  for ( string::const_iterator iter( fmt.begin() ), end( fmt.end() ); 
	iter != end; ++iter )
    {
      if ( *iter == '$' || !in_expr && *iter == '%' ) 
	{
	  if ( ++iter == end ) 
	    throw argument_error( "Format ends with an unescaped `$'" );

	  string::const_iterator iter2(iter);

	  while ( iter != end && isdigit(*iter) )
	    ++iter;

	  if ( iter == end ) 
	    throw argument_error( "End of format reached whilst processing "
				  "numeric argument to `$'" );

	  if (*iter == '[')
	    {
	      expr_start = iter+1;
	      if ( in_expr )
		throw argument_error( "Cannot nest $[ ] expressions" );
	      in_expr = true;
	    }

	  // Errors for formats that can only be used with -H or -R
	  switch ( *iter )
	    {
	    case 'c': 
	      if ( type != stat_type )
		throw argument_error( make_string() << "The `$" << *iter << "'"
				      " can only be used in stats formats" );
	      break;

	    case 'p': case 'q': case 'n': case 'N': case 'P': 
	    case '[': // Don't want to think about this in stats, yet.
	      if ( type != normal_type )
		throw argument_error( make_string() << "The `$" << *iter << "'"
				      " can only be used in output formats" );
	      break;

	    case '%': case '$': case 'l': case 'r': case 'b': 
	    case 'C': case 'S': case 'M': case 'h': case 'F':
	    case 'o': case 'd':
	      // Can be used in either
	      break;

	    default:
	      throw argument_error( make_string() << "Unknown format "
				    "specifier: `$" << *iter << "'" );
	    }


	  int num_opt = atoi( string( &*iter2, &*iter ).c_str() );

	  // Errors for formats that must or mustn't have a numeric argument
	  switch ( *iter )
	    {
	    case '%': case '$': case 'c': case 'b': case 'M': 
	    case 'o': case '[':
	      // Option may but needn't have a number
	      break;

	    case 'n': case 'N': case 'p': case 'q': case 'l': 
	    case 'C': case 'S': case 'F': case 'd':
	      if ( iter != iter2 )
		throw argument_error
		  ( make_string() << "The `$" << *iter << "' "
		    "format specifier must not be preceeded by "
		    "a number" );
	      break;

	    case 'r': case 'h': case 'P':
	      if ( iter == iter2 )
		throw argument_error
		  ( make_string() << "The `$" << *iter << "' "
		    "format specifier must be preceeded by a number" );
	      break;
	    }

	  // Mark the option as used
	  switch ( *iter )
	    {
	    case 'l': 
	      has_lead_head = true; 
	      break;

	    case 'p': 
	      has_pn = true; 
	      break;

	    case 'q': 
	      has_compressed_pn = true; 
	      break;

	    case 'b':
	      has_blow_count = true;
	      break;

	    case 'o':
	      has_lh_order = true;
	      break;

	    case 'd':
	      has_lh_code = true;
	      break;

	    case 'r':
	      if ( find( has_rows.begin(), has_rows.end(), num_opt ) 
		     == has_rows.end() )
		has_rows.push_back( num_opt );
	      break;

	    case 'h':
	      if ( find( has_changes.begin(), has_changes.end(), num_opt ) 
		   == has_changes.end() )
		has_changes.push_back( num_opt );
	      break;

	    case 'P':
	      if ( find( has_path.begin(), has_path.end(), num_opt ) 
		   == has_path.end() )
		has_path.push_back( num_opt );
	      break;

	    case 'n':
	      has_name = true;
	      break;

	    case 'N':
	      has_full_name = true;
	      break;

	    case 'C':
	      has_class_name = true;
	      break;

	    case 'S':
	      has_stage_name = true;
	      break;

	    case 'M':
	      has_music_score = true;
	      break;

	    case 'F':
	      has_falseness_group = true;
	      break;

	    case '[':
	      has_expression = true;
	      break;
	    }
	}
      else if ( *iter == '\\' )
	{
	  if ( ++iter == end ) 
	    {
	      line_break = false;
	      this->fmt.erase(this->fmt.end()-1);
	      return;
	    }
	}
      else if ( *iter == '(' && in_expr )
	{
	  ++parens;
	}
      else if ( *iter == ')' && in_expr )
	{
	  if ( parens == 0 )
	    throw argument_error( "Unmatched parenthesis in $[ ] expression" );
	  --parens;
	}
      else if ( *iter == ']' && in_expr )
	{
	  if ( parens > 0 )
	    throw argument_error( "Unmatched parenthesis in $[ ] expression" );
	  in_expr = false;

	  try 
	    {
	      string expr( expr_start, iter );
	      if ( exprs[expr].null() )
		exprs[expr] = expression(expr);
	    } 
	  catch ( const argument_error& e ) 
	    {
	      // Add more context to the error
	      throw argument_error( make_string() << "In $[...] expression: " 
				    << e.what() );
	    }
	}	
    }
}



struct statistics::impl
{
  typedef map< histogram_entry, size_t, histogram_entry::cmp > map_type;
  map_type histogram;
};

statistics::statistics()
  : pimpl( new impl )
{
}

statistics::~statistics()
{
}

statistics::impl &statistics::instance()
{
  static statistics tmp;
  return *tmp.pimpl;
}

size_t statistics::output( ostream &os )
{
  size_t count(0u);

  for ( impl::map_type::const_iterator 
	  i( instance().histogram.begin() ),
	  e( instance().histogram.end() );
	i != e;  ++i )
    {
      i->first.print( os, i->second );
      count += i->second;
    }

  return count;
}

void statistics::add_entry( const histogram_entry &entry )
{
  ++instance().histogram[entry];
}

void clear_status()
{
  cerr << '\r' << string( 60, ' ' ) << '\r';
}

void output_status( const method &m )
{
  make_string tmp;
  for ( method::const_iterator i(m.begin()), e(m.end()); i!=e; ++i )
    tmp << *i << ".";
  string s = tmp;

  if ( s.size() > 45 ) 
    s = s.substr(0, 45) + "...";
  
  clear_status();
  cerr << "Trying " << s << flush;
}

void output_count( unsigned long c )
{
  cout << "Found " << c << " methods\n";
}
