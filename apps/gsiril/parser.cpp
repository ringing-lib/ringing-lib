// parser.cpp - Tokenise and parse lines of input
// Copyright (C) 2002, 2003, 2004, 2005, 2007, 2008, 2010, 2011, 2012, 2013,
// 2019 Richard Smith <richard@ex-parrot.com>

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
#include "stringutils.h"
#include "expr_base.h"
#include "expression.h"
#include "statement.h"
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
    reverse     = '~',
    ctrl_z      = '\x1A',   /* EOF marker in microSIRIL */
    comment     = tokeniser::first_token,
    string_lit,
    transp_lit,
    pn_lit,
    def_assign, /* ?= */
    logic_and,
    logic_or,
    regex_lit
  };
}

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

class mstokeniser : public tokeniser
{
public:
  mstokeniser( istream& in, arguments const& args )
    : tokeniser( in, keep_new_lines, 
		 args.case_insensitive ? case_insensitive : case_sensitive ), 
      args( args ),
      c( args.msiril_syntax ? "/" : "//" ), 
      r( "/",   tok_types::regex_lit,  string_token::one_line ),
      q( "'",   tok_types::transp_lit, string_token::one_line ), 
      qq( "\"", tok_types::string_lit, string_token::one_line ), 
      sym( "&" ), asym( "+" ), defass( "?=", tok_types::def_assign ),
      land( "&&", tok_types::logic_and ), lor( "||", tok_types::logic_or )
  {
    // Note:  It is important that &sym is added after &land; and
    // similarly, that &r is added after &c.  This is because && is a
    // longer, and thus more specialised prefix than &, and similarly
    // for // and /.

    add_qtype(&c);
    if ( !args.msiril_syntax ) add_qtype(&r);
    add_qtype(&q);    add_qtype(&qq);
    add_qtype(&defass); add_qtype(&land); add_qtype(&lor);
    add_qtype(&sym);  add_qtype(&asym);

    if ( args.sirilic_syntax ) 
      set_id_chars( "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz",
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz"
                    "01234567890"  "%-!" );  // Sirilic adds '%', '-', '!'
  }

  virtual void validate( const token& t ) const
  {
    using namespace tok_types;

    switch ( t.type() ) {
    case name: case num_lit: case open_paren: case close_paren:
    case comma: case times: case assignment: case new_line: case semicolon:
    case comment: case string_lit: case transp_lit: case pn_lit: 
    case def_assign: case logic_and: case logic_or: case reverse:
      return;

    case ctrl_z:
      // Obsolete microSIRIL features
      if ( args.msiril_syntax ) return;
      break;

    case regex_lit: case open_brace: case close_brace: case colon:
      // These can only work when msiril comments are disabled
      if ( !args.msiril_syntax ) return;
      break;
    }
     
    throw runtime_error( make_string() << "Unknown token in input: " << t );
  }

private:
  const arguments& args;

  line_comment_impl c;
  string_token r, q, qq;
  pn_impl sym, asym;
  basic_token defass, land, lor;
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
  virtual int line() const { return tok.line(); }

  int bells() const { return args.bells; }
  void bells(int new_b);

  vector< token > tokenise_command();

  expression make_expr( vector< token >::const_iterator first, 
			vector< token >::const_iterator last ) const;

  bool is_enclosed( vector< token >::const_iterator first, 
		    vector< token >::const_iterator last,
		    tok_types::enum_t open, tok_types::enum_t close,
		    string const& name ) const;

  bool find_first( vector< token >::const_iterator first, 
		   vector< token >::const_iterator last,
		   tok_types::enum_t tt,
		   vector< token >::const_iterator &result ) const;

  bool find_last( vector< token >::const_iterator first, 
		  vector< token >::const_iterator last,
		  tok_types::enum_t tt,
		  vector< token >::const_iterator &result ) const;

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
	  if ( tokiter->type() != tok_types::comment &&
	       tokiter->type() != tok_types::new_line ) {
	    try {
	      tok.validate(*tokiter);
	    } catch (...) {
	      ++tokiter; // skip over the bogus token
	      throw;
	    }
	    toks.push_back(*tokiter);
	  }

	  // Keep track of nesting
	  if ( tokiter->type() == tok_types::open_paren || 
	       tokiter->type() == tok_types::open_brace ) 
	    ++nesting;
	  if ( tokiter->type() == tok_types::close_paren || 
	       tokiter->type() == tok_types::close_brace ) 
	    if ( nesting > 0 )
	      --nesting;

	  ++tokiter;
	}
    }
  while  ( !toks.empty() && ( toks.back().type() == tok_types::comma 
			      || nesting ) && tokiter != tokend );

  if ( !toks.empty() && ( toks.back().type() == tok_types::comma || nesting ) )
    throw runtime_error( make_string() 
      << "Unexpected end of file midway through command" );
   
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
       ( cmd[0] == "end" || cmd[0] == "quit" || cmd[0] == "exit" 
         || cmd[0].type() == tok_types::ctrl_z ) )
    return statement( NULL );

  // Version directive
  if ( cmd.size() == 1 && cmd[0].type() == tok_types::name
       && cmd[0] == "version" )
    {
      cout << "Version: " RINGING_VERSION "\n";
      return statement( new null_stmt );
    }

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

  // Rounds directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::name 
       && cmd[0] == "rounds" && cmd[1].type() == tok_types::transp_lit )
    {
      return statement( new rounds_stmt(cmd[1]) );
    }

  // Import directive
  if ( !args.disable_import
       && cmd.size() == 2 && cmd[0].type() == tok_types::name
       && cmd[0] == "import" && ( cmd[1].type() == tok_types::name ||
				  cmd[1].type() == tok_types::string_lit ) )
    return statement( new import_stmt(cmd[1]) );
  

  // Prove command
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[0] == "prove" 
       // Make a half-hearted attempt to support a variable called 'prove':
       && cmd[1].type() != tok_types::assignment
       && cmd[1].type() != tok_types::def_assign )
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

  throw runtime_error( "Unknown command: " + cmd[0] + " ..." );
}



//////////////////////////////////////////////////////////

// Is the range [first, last) enclosed in a correctly-matched (open, close)
// pair?  E.g. ``(&-6-6-6,+2)'' is enclosed in parentheses; 
// ``(&-6-6-6),(+2)'' is not.
bool msparser::is_enclosed( vector< token >::const_iterator first, 
			    vector< token >::const_iterator const last,
			    tok_types::enum_t open, tok_types::enum_t close,
			    string const& name ) const
{
  if ( first->type() != open || (last-1)->type() != close )
    return false;

  int depth = 0;
  for ( ; first != last; ++first )
    {
      if      ( first->type() == open  ) ++depth;
      else if ( first->type() == close ) --depth;
      
      if ( depth == 0 && first != last-1 ) 
	return false;
    }

  if (depth) 
    throw runtime_error( make_string() << "Unmatched " << name );

  if (first+1 == last-1)
    throw runtime_error( make_string() << "Empty " << name );

  return true;
}

bool msparser::find_last( vector< token >::const_iterator first, 
			  vector< token >::const_iterator const last,
			  tok_types::enum_t tt,
			  vector< token >::const_iterator &result ) const
{
  int depth( 0 );
  bool found( false );
  for ( ; first != last; ++first )
    if      ( first->type() == tok_types::open_brace ||
	      first->type() == tok_types::open_paren )
      ++depth;
    else if ( first->type() == tok_types::close_brace ||
	      first->type() == tok_types::close_paren )
      --depth;
    else if ( !depth && first->type() == tt ) 
      result = first, found = true;

  if (depth) 
    throw runtime_error( "Syntax Error (Unmatched brackets?)" );

  return found;
}


bool msparser::find_first( vector< token >::const_iterator first, 
			   vector< token >::const_iterator const last,
			   tok_types::enum_t tt,
			   vector< token >::const_iterator &result ) const
{
  int depth( 0 );
  for ( ; first != last; ++first )
    if      ( first->type() == tok_types::open_brace ||
	      first->type() == tok_types::open_paren )
      ++depth;
    else if ( first->type() == tok_types::close_brace ||
	      first->type() == tok_types::close_paren )
      --depth;
    else if ( !depth && first->type() == tt ) {
      result = first;
      return true;
    }

  if (depth) 
    throw runtime_error( "Syntax Error (Unmatched brackets?)" );

  return false;
}

expression 
msparser::make_expr( vector< token >::const_iterator first, 
		     vector< token >::const_iterator last ) const
{
  typedef vector< token >::const_iterator iter_t;

  if ( first == last ) 
    throw runtime_error( "Expression expected" );

  // Parentheses first
  if ( is_enclosed( first, last, tok_types::open_paren, 
		    tok_types::close_paren, "parentheses" ) )
    return make_expr( first+1, last-1 );


  // Alternative blocks are enclosed in braces
  if ( is_enclosed( first, last, tok_types::open_brace, 
		    tok_types::close_brace, "braces" ) )
    {
      expression chain( NULL );
      --last;
      while ( last != first && (last-1)->type() == tok_types::semicolon ) 
	--last;

      // Doesn't use recursion because the contents of the braces is 
      // not a single valid expression.
      while ( last != first ) {
	// The last semicolon, or the opening brace (FIRST) if there is none.
	iter_t i( first ); find_last( first+1, last, tok_types::semicolon, i );
	iter_t j( i );

	// Is there a test?
	expression test( NULL );
	if ( find_first( i+1, last, tok_types::colon, j ) )
	  test = make_expr( i+1, j );
	else
	  test = expression( new pattern_node( bells(), "*" ) );
	  
	// Is there a expression?
	expression expr( NULL );
	if ( j+1 != last )
	  expr = make_expr( j+1, last );
	else
	  expr = expression( new nop_node );

	chain = expression( new if_match_node( test, expr, chain ) );

	last = i;
	while ( last != first && (last-1)->type() == tok_types::semicolon ) 
	  --last;
      }
      if ( chain.isnull() )
	throw runtime_error( "Empty braces" );
      return chain;
    }

  iter_t split;

  // Assignment is the lowest precedence operator
  if ( find_first( first, last, tok_types::assignment, split ) )
    {
      if ( first == split )
	throw runtime_error
	  ( "Assignment operator needs first argument" );
      
      if ( first->type() != tok_types::name || 
	   first+1 != split )
	throw runtime_error
	  ( "First argument of assignment operator must be"
	    " a variable name" );
      
      if ( split+1 == last)
	return expression
	  ( new assign_node( *first, expression( new nop_node ) ) );
      else
	return expression
	  ( new assign_node( *first, make_expr( split+1, last ) ) );
    }

  // Comma is the next lowest precedence operator
  // It is left associative
  if ( find_first( first, last, tok_types::comma, split ) )
    {
      if ( first == split )
	throw runtime_error
	  ( "Binary operator \",\" needs first argument" );
      
      if ( split+1 == last)
	throw runtime_error
	  ( "Binary operator \",\" needs second argument" );
      
      return expression
	( new list_node( make_expr( first, split ),
			 make_expr( split+1, last ) ) );
    }

  // Logical operators -- Or (||) is lowest precedence
  if ( find_first( first, last, tok_types::logic_or, split ) )
    {
      if ( first == split )
	throw runtime_error
	  ( "Binary operator \"||\" needs first argument" );
      
      if ( split+1 == last)
	throw runtime_error
	  ( "Binary operator \"||\" needs second argument" );

      return expression
	( new or_node( make_expr( first, split ),
		       make_expr( split+1, last ) ) );
    }

  // Logical operators -- And (&&) is next lowest precedence
  if ( find_first( first, last, tok_types::logic_and, split ) )
    {
      if ( first == split )
	throw runtime_error
	  ( "Binary operator \"&&\" needs first argument" );
      
      if ( split+1 == last)
	throw runtime_error
	  ( "Binary operator \"&&\" needs second argument" );

      return expression
	( new and_node( make_expr( first, split ),
			make_expr( split+1, last ) ) );
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
	  ( make_string() << "The repetition operator requires an argument, "
            "or did you mean +" << *first << "?" );

      int times(-1);
      if ( first->type() == tok_types::num_lit )
	times = string_to_int( *first );

      return expression
	( new repeated_node( times,
			     make_expr( begin_arg, last ) ) );
    }

  if ( first->type() == tok_types::name && *first == "echo" ) 
    {
      if ( first + 1 == last )
	throw runtime_error( "The echo operator requires an argument" );
      if ( (first + 1)->type() != tok_types::string_lit )
	throw runtime_error( "The echo operator's argument must be a string" );

      return expression( new string_node( *(first+1), true ) );
    }

  // Reversals are high precedence unary prefix operators
  if ( first->type() == tok_types::reverse )
    {
      if ( first + 1 == last )
	throw runtime_error
	  ( "The reverse operator requires an argument" );

      return expression( new reverse_node( make_expr( first+1, last ) ) );
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
      else if ( *first == "rounds" )
	return expression( new isrounds_node() );
      else
	return expression( new symbol_node( *first ) );

    case tok_types::pn_lit:
      return expression( new pn_node( bells(), *first ) );

    case tok_types::transp_lit:
      return expression( new transp_node( bells(), *first ) );

    case tok_types::regex_lit:
      return expression( new pattern_node( bells(), *first ) );

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
