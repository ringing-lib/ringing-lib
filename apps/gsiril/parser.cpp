// parser.cpp - Tokenise and parse lines of input
// Copyright (C) 2002, 2003, 2004 Richard Smith <richard@ex-parrot.com>

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

#include "parser.h"
#include "tokeniser.h"
#include "util.h"
#include "expression.h"
#include "common_expr.h"
#include "prog_args.h"
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#include <vector.h>
#else
#include <stdexcept>
#include <vector>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#else
#include <cctype>
#endif
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE

RINGING_START_ANON_NAMESPACE

//////////////////////////////////////////////////////////

namespace tok_types
{
  enum enum_t 
  { 
    name        = 'A',
    num_lit     = '0',
    open_paren  = '(',
    close_paren = ')',
    open_brace  = '{',
    close_brace = '}',
    colon       = ':',
    comma       = ',',
    times       = '*',
    assignment  = '=',
    new_line    = '\n',
    semicolon   = ';',
    comment     = tokeniser::first_token,
    string_lit,
    transp_lit,
    pn_lit,
    def_assign, /* ?= */
    regex_lit
  };
};

class pn_impl : public tokeniser::basic_token
{
public:
  pn_impl(const char *init) : basic_token(init, tok_types::pn_lit) {}
  
private:
  virtual parse_result parse( string::const_iterator& i, 
			      string::const_iterator e, 
			      token& tok ) const
  {
    string::const_iterator j = i;
    if ( *j != '&' && *j != '+' ) return failed; ++j;
    while ( j < e && ( *j == '.' || *j == '-' || isalnum(*j) ) ) ++j;
    tok.assign(i, j); tok.type( tok_types::pn_lit ); i = j;
    return done;
  }
};

class line_comment_impl : public tokeniser::basic_token
{
public:
  line_comment_impl( char const* delim )
    : basic_token( delim, tok_types::comment) {}
  
private:
  virtual parse_result parse( string::const_iterator& i, 
			      string::const_iterator e, 
			      token& tok ) const
  {
    string::const_iterator j = i;
    while ( j < e && *j != '\n' ) ++j;
    tok = string(i, j); tok.type( tok_types::comment ); i = j;
    return done;
  }
};

void validate_regex( const token& tok, int bells )
{
  static string allowed;
  if ( allowed.empty() ) {
    allowed.append( row(bells).print() );
    allowed.append("*?[]");
  }

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

class mstokeniser : public tokeniser
{
public:
  mstokeniser( istream& in, arguments const& args )
    : tokeniser( in, keep_new_lines, 
		 args.case_insensitive ? case_insensitive : case_sensitive ), 
      args( args ),
      c( args.msiril_comments ? "/" : "//" ), r( "/", tok_types::regex_lit ),
      q( "'", tok_types::transp_lit ), qq( "\"", tok_types::string_lit ), 
      sym( "&" ), asym( "+" ), defass( "?=", tok_types::def_assign )
  {
    add_qtype(&c);
    if ( !args.msiril_comments ) add_qtype(&r);
    add_qtype(&q);    add_qtype(&qq);
    add_qtype(&sym);  add_qtype(&asym);
    add_qtype(&defass);
  }

  virtual void validate( const token& t ) const
  {
    using namespace tok_types;

    switch ( t.type() ) {
    case name: case num_lit: case open_paren: case close_paren:
    case comma: case times: case assignment: case new_line: case semicolon:
    case comment: case string_lit: case transp_lit: case pn_lit: 
    case def_assign:
      return;

    case regex_lit: case open_brace: case close_brace: case colon:
      // These can only work when msiril comments are disabled
      if ( !args.msiril_comments ) return;
      break;
    }
     
    throw runtime_error( make_string() << "Unknown token in input: " << t );
  }

private:
  const arguments& args;

  line_comment_impl c;
  string_token r, q, qq;
  pn_impl sym, asym;
  basic_token defass;
};


//////////////////////////////////////////////////////////

class msparser : public parser
{
public:
  msparser( istream& in, const arguments& args ) 
    : args(args), tok(in, args),
      tokiter(tok.begin()), tokend(tok.end())
  {}

private:
  virtual statement parse();
  int bells() const { return args.bells; }
  void bells(int new_b);

  vector< token > tokenise_command();

  expression make_expr( vector< token >::const_iterator first, 
			vector< token >::const_iterator last ) const;


  // Data members
  arguments args;
  mstokeniser tok;
  mstokeniser::const_iterator tokiter, tokend;
};

vector< token > msparser::tokenise_command()
{
  vector< token > toks;
  int nesting = 0;

  do 
    {
      // Skip empty commands
      while ( tokiter != tokend && 
	      ( tokiter->type() == tok_types::new_line ||
		tokiter->type() == tok_types::semicolon ||
		tokiter->type() == tok_types::comment ) )
	++tokiter;

      while ( tokiter != tokend && 
	      ( tokiter->type() != tok_types::new_line && 
		tokiter->type() != tok_types::semicolon 
		|| nesting ) )
	{
	  // TODO:  Get the tokeniser to discard comments these automatically
	  if ( tokiter->type() != tok_types::comment )
	    toks.push_back(*tokiter);

	  // Keep track of nesting
	  if ( tokiter->type() == tok_types::open_paren || 
	       tokiter->type() == tok_types::open_brace ) 
	    ++nesting;
	  if ( tokiter->type() == tok_types::close_paren || 
	       tokiter->type() == tok_types::close_brace ) 
	    if ( nesting > 0 )
	      --nesting;

	  ++tokiter;

	  if (toks.size())
	    tok.validate( toks.back() );
	}
    }
  while  ( !toks.empty() && ( toks.back().type() == tok_types::comma 
			      || nesting ) );
   
  return toks;
}

void msparser::bells(int b)
{
  if ( b > (int) bell::MAX_BELLS )
    throw runtime_error( make_string() 
			 << "Number of bells must be less than "
			 << bell::MAX_BELLS + 1 );
  else if ( b <= 1 )
    throw runtime_error( "Number of bells must be greater than 1" );

  args.bells = b;
}

statement msparser::parse()
{
  vector<token> cmd( tokenise_command() );

  // EOF
  if ( cmd.size() == 0 || cmd.size() == 1 && 
       ( cmd[0] == "end" || cmd[0] == "quit" || cmd[0] == "exit" ) )
    return statement( NULL );

  // Bells directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::num_lit
       && cmd[1].type() == tok_types::name && cmd[1] == "bells" )
    {
      bells( string_to_int( cmd[0] ) );
      return statement( new bells_stmt(args.bells) );
    }

  // Extents directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::num_lit
       && cmd[1].type() == tok_types::name && cmd[1] == "extents" )
    {
      return statement( new extents_stmt( string_to_int(cmd[0]) ) );
    }

  // Import directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::name
       && cmd[0] == "import" && ( cmd[1].type() == tok_types::name ||
				  cmd[1].type() == tok_types::string_lit ) )
    return statement( new import_stmt(cmd[1]) );
  

  // Prove command
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[0] == "prove" )
    return statement
      ( new prove_stmt( make_expr( cmd.begin() + 1, cmd.end() ) ) );

  // Definition
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[1].type() == tok_types::assignment )
    return statement
      ( new definition_stmt
        ( cmd[0], cmd.size() == 2 
	            ? expression( new nop_node )
	            : make_expr( cmd.begin() + 2, cmd.end() ) ) );

  // Default definition
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[1].type() == tok_types::def_assign )
    return statement
      ( new default_defn_stmt
        ( cmd[0], cmd.size() == 2 
	            ? expression( new nop_node )
	            : make_expr( cmd.begin() + 2, cmd.end() ) ) );

  throw runtime_error( "Unknown command" );
}


//////////////////////////////////////////////////////////

expression 
msparser::make_expr( vector< token >::const_iterator first, 
		     vector< token >::const_iterator last ) const
{
  typedef vector< token >::const_iterator iter_t;

  if ( first == last ) 
    throw runtime_error( "Expression expected" );

  // Parentheses first
  if ( first->type() == tok_types::open_paren && 
       (last-1)->type() == tok_types::close_paren )
    {
      int depth = 0;
      bool ok = true;
      for ( iter_t i(first); ok && i != last; ++i )
	{
	  if ( i->type() == tok_types::open_paren )
	    ++depth;
	  else if ( i->type() == tok_types::close_paren )
	    --depth;

	  if ( depth == 0 && i != last-1 ) 
	    ok = false;
	}

      if (ok)
	{
	  if (depth) 
	    throw runtime_error( "Unmatched parentheses" );

	  if (first+1 == last-1 )
	    throw runtime_error( "Empty parentheses" );

	  return make_expr( first+1, last-1 );
	}
    }

  // Do Dixonoid braces next
  // syntax:  {regex: expr; regex: expr; ... }
  if ( first->type() == tok_types::open_brace && 
       (last-1)->type() == tok_types::close_brace )
    {
      int depth = 0;
      bool ok = true;
      for ( iter_t i(first); ok && i != last; ++i )
	{
	  if ( i->type() == tok_types::open_brace )
	    ++depth;
	  else if ( i->type() == tok_types::close_brace )
	    --depth;

	  if ( depth == 0 && i != last-1 ) 
	    ok = false;
	}
      
      if (ok) {
	if (depth) 
	  throw runtime_error( "Unmatched braces" );
	
	if (first+1 == last-1 )
	  throw runtime_error( "Empty braces" );
	
	expression chain( NULL );
	--last;
	while ( last != first && (last-1)->type() == tok_types::semicolon ) 
	  --last;

	while ( last != first ) {

	  // Find last semicolon respecting braces
	  iter_t i(first);
	  for ( iter_t j(first+1); j != last; ++j ) {
	    if ( j->type() == tok_types::open_brace )
	      ++depth;
	    else if ( j->type() == tok_types::close_brace )
	      --depth;
	    if ( !depth && j->type() == tok_types::semicolon )
	      i = j;
	  }
	  assert( i != last-1 );

	  if ( i[1].type() != tok_types::regex_lit )
	    throw runtime_error
	      ( "Brace group element should start with regular expression" );

	  if ( i+2 == last || i[2].type() != tok_types::colon )
	    throw runtime_error
	      ( "Regular expression should be followed by a colon" );

	  expression expr( NULL );
	  if ( i+3 != last)
	    expr = make_expr( i+3, last );
	  else 
	    expr = expression( new nop_node );

	  validate_regex( i[1], bells() );
	  chain = expression( new if_match_node( bells(), i[1], 
						 expr, chain ) );

	  last = i;

	  while ( last != first && (last-1)->type() == tok_types::semicolon ) 
	    --last;
	}
	if ( chain.isnull() )
	  throw runtime_error( "Empty braces" );
	return chain;
      }
    }

  // Assignment is the lowest precedence operator
  {
    int depth = 0;
    for ( iter_t i(first); i != last; ++i )
      {
	if ( i->type() == tok_types::open_paren )
	  ++depth;
	else if ( i->type() == tok_types::close_paren )
	  --depth;
	else if ( i->type() == tok_types::assignment && depth == 0 )
	  {
	    if ( first == i )
	      throw runtime_error
		( "Assignment operator needs first argument" );
	    
	    if ( first->type() != tok_types::name || 
		 first+1 != i )
	      throw runtime_error
		( "First argument of assignment operator must be"
		  " a variable name" );

	    if ( i+1 == last)
	      return expression
		( new assign_node( *first, expression( new nop_node ) ) );
	    else
	      return expression
		( new assign_node( *first, make_expr( i+1, last ) ) );
	  }
      }

    if (depth) 
      throw runtime_error( "Unmatched parentheses" );
  }

  // Comma is the next lowest precedence operator
  // It is left associative
  {
    int depth = 0;
    for ( iter_t i(first); i != last; ++i )
      {
	if ( i->type() == tok_types::open_paren )
	  ++depth;
	else if ( i->type() == tok_types::close_paren )
	  --depth;
	else if ( i->type() == tok_types::comma && depth == 0 )
	  {
	    if ( first == i )
	      throw runtime_error
		( "Binary operator \",\" needs first argument" );
	    
	    if ( i+1 == last)
	      throw runtime_error
		( "Binary operator \",\" needs second argument" );

	    return expression
	      ( new list_node( make_expr( first, i ),
			       make_expr( i+1, last ) ) );
	  }
      }

    if (depth) 
      throw runtime_error( "Unmatched parentheses" );
  }

  // A number literal in a repeated block is the
  // only remaining construct that is not a single token.
  if ( first->type() == tok_types::num_lit ||
       first->type() == tok_types::name && *first == "repeat" )
    {
      iter_t begin_arg = first + 1;

      if ( begin_arg != last && begin_arg->type() == tok_types::times )
	++begin_arg;

      if ( begin_arg == last )
	throw runtime_error
	  ( "The repetition operator requires an argument" );

      int times(-1);
      if ( first->type() == tok_types::num_lit )
	times = string_to_int( *first );

      return expression
	( new repeated_node( times,
			     make_expr( begin_arg, last ) ) );
    }

  // Everything left is a literal of some sort
  if ( last - first != 1 )
    throw runtime_error( make_string() << "Parse error before " << *first  );

  switch ( first->type() )
    {
    case tok_types::string_lit:
      return expression( new string_node( *first ) );

    case tok_types::name:
      if ( *first == "break" )
	return expression( new exception_node( script_exception::do_break ) );
      else
	return expression( new symbol_node( *first ) );

    case tok_types::pn_lit:
      return expression( new pn_node( bells(), *first ) );

    case tok_types::transp_lit:
      return expression( new transp_node( bells(), *first ) );

    default:
      throw runtime_error( "Unknown token in input" );
      return expression( NULL ); // To keep MSVC 5 happy
    }
}

//////////////////////////////////////////////////////////

RINGING_END_ANON_NAMESPACE

shared_pointer<parser> 
make_default_parser( istream& in, const arguments& args )
{
  return shared_pointer<parser>( new msparser( in, args ) );
}
