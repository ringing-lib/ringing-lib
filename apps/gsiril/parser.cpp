// parser.cpp - Tokenise and parse lines of input
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

class mstokeniser : public tokeniser
{
public:
  enum tok_types 
  { 
    name        = 'A',
    num_lit     = '0',
    open_paren  = '(',
    close_paren = ')',
    comma       = ',',
    times       = '*',
    assignment  = '=',
    new_line    = '\n',
    comment     = first_token,
    string_lit,
    transp_lit,
    pn_lit
  };

  mstokeniser( istream& in, bool is_case_insensitive )
    : tokeniser( in, keep_new_lines, 
		 is_case_insensitive ? case_insensitive : case_sensitive ), 
      q( "'", transp_lit ), qq( "\"", string_lit ), 
      sym( "&" ), asym( "+" )
  {
    add_qtype(&c);
    add_qtype(&q);    add_qtype(&qq);
    add_qtype(&sym);  add_qtype(&asym);
  }


  virtual void validate( const token& t ) const
  {
    switch ( t.type() )
      case name: case num_lit: case open_paren: case close_paren:
      case comma: case times: case assignment: case new_line: case comment:
      case string_lit: case transp_lit: case pn_lit:
	return;
    
    throw runtime_error( make_string() << "Unknown token in input: " << t );
  }

private:
  class pn_impl : public tokeniser::basic_token
  {
  public:
    pn_impl(const char *init) : basic_token(init, pn_lit) {}

  private:
    virtual parse_result parse( string::const_iterator& i, 
				string::const_iterator e, 
				token& tok ) const
    {
      string::const_iterator j = i;
      if ( *j != '&' && *j != '+' ) return failed; ++j;
      while ( j < e && ( *j == '.' || *j == '-' || isalnum(*j) ) ) ++j;
      tok.assign(i, j); tok.type( pn_lit ); i = j;
      return done;
    }
  };

  class comment_impl : public tokeniser::basic_token
  {
  public:
    comment_impl() : basic_token("/", comment) {}

  private:
    virtual parse_result parse( string::const_iterator& i, 
				string::const_iterator e, 
				token& tok ) const
    {
      string::const_iterator j = i;
      while ( j < e && *j != '\n' ) ++j;
      tok = string(i, j); tok.type( comment ); i = j;
      return done;
    }
  };

  comment_impl c;
  string_token q, qq;
  pn_impl sym, asym;
};


//////////////////////////////////////////////////////////

class msparser : public parser
{
public:
  msparser( istream& in, const arguments& args ) 
    : args(args), tok(in, args.case_insensitive),
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

  do 
    {
      // Skip lines which are purely whitespace or comments
      while ( tokiter != tokend && 
	      ( tokiter->type() == mstokeniser::new_line ||
		tokiter->type() == mstokeniser::comment ) )
	++tokiter;

      while ( tokiter != tokend && tokiter->type() != mstokeniser::new_line )
	{
	  // TODO:  Get the tokeniser to discard comments these automatically
	  if ( tokiter->type() != mstokeniser::comment )
	    toks.push_back(*tokiter);

	  ++tokiter;

	  if (toks.size())
	    tok.validate( toks.back() );
	}
    }
  while  ( !toks.empty() && toks.back().type() == mstokeniser::comma );
   
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
  if ( cmd.size() == 2 && cmd[0].type() == mstokeniser::num_lit
       && cmd[1].type() == mstokeniser::name && cmd[1] == "bells" )
    {
      bells( string_to_int( cmd[0] ) );
      return statement( new bells_stmt(args.bells) );
    }
  
  // Import directive
  if ( cmd.size() == 2 && cmd[0].type() == mstokeniser::name
       && cmd[0] == "import" && ( cmd[1].type() == mstokeniser::name ||
				  cmd[1].type() == mstokeniser::string_lit ) )
    return statement( new import_stmt(cmd[1]) );
  

  // Prove command
  if ( cmd.size() > 1 && cmd[0].type() == mstokeniser::name
       && cmd[0] == "prove" )
    return statement
      ( new prove_stmt( make_expr( cmd.begin() + 1, cmd.end() ) ) );

  // Definition
  if ( cmd.size() > 1 && cmd[0].type() == mstokeniser::name
       && cmd[1].type() == mstokeniser::assignment )
    return statement
      ( new definition_stmt
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
  if ( first->type() == mstokeniser::open_paren && 
       (last-1)->type() == mstokeniser::close_paren )
    {
      int depth = 0;
      bool ok = true;
      for ( iter_t i(first); ok && i != last; ++i )
	{
	  if ( i->type() == mstokeniser::open_paren )
	    ++depth;
	  else if ( i->type() == mstokeniser::close_paren )
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

  // Comma is the lowest precedence operator
  // It is left associative
  {
    int depth = 0;
    for ( iter_t i(first); i != last; ++i )
      {
	if ( i->type() == mstokeniser::open_paren )
	  ++depth;
	else if ( i->type() == mstokeniser::close_paren )
	  --depth;
	else if ( i->type() == mstokeniser::comma && depth == 0 )
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
  if ( first->type() == mstokeniser::num_lit )
    {
      iter_t begin_arg = first + 1;

      if ( begin_arg != last && begin_arg->type() == mstokeniser::times )
	++begin_arg;

      if ( begin_arg == last )
	throw runtime_error
	  ( "The repetition operator requires an argument" );

      return expression
	( new repeated_node( string_to_int( *first ),
			     make_expr( begin_arg, last ) ) );
    }

  // Everything left is a literal of some sort
  if ( last - first != 1 )
    throw runtime_error( "Parse error" );

  switch ( first->type() )
    {
    case mstokeniser::string_lit:
      return expression( new string_node( *first ) );

    case mstokeniser::name:
      return expression( new symbol_node( *first ) );

    case mstokeniser::pn_lit:
      return expression( new pn_node( bells(), *first ) );

    case mstokeniser::transp_lit:
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
