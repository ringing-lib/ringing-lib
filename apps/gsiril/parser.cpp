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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include "parser.h"
#include "expression.h"
#include "common_expr.h"
#include "util.h"
#include "console_stream.h" // To fix getline bug in MSVC.
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

void strip_comment( string &line, char quote_state )
{
  // Note:  This function does not use string::iterators due to
  // an optimiser bug in gcc-3.1 which previously caused a segfault here. 
  for ( const char *i = line.data(), *e = line.data() + line.size(); 
	i != e; ++i )
    {
      if ( !quote_state && (*i == '"' || *i == '\'') )
	quote_state = *i;
      else if ( quote_state && *i == quote_state )
	quote_state = '\0';
      else if ( !quote_state && *i == '/' )
	{
	  line.erase(i - line.data());
	  break;
	}
    }
  return;
}

bool unmatched_quotes( const string &str, char &quote_state )
{
  //bool quoted = false;

  for ( string::const_iterator i( str.begin() ), e( str.end() ); i != e; ++i )
    if ( !quote_state && (*i == '"' || *i == '\'') )
      quote_state = *i;
    else if ( quote_state && *i == quote_state )
      quote_state = '\0';

  return quote_state;
}

void trim_whitespace( string &line, char quote_state )
{
  if ( !quote_state ) 
    trim_leading_whitespace( line );

  if ( !unmatched_quotes(line, quote_state) && line.size() ) 
    trim_trailing_whitespace( line );
}

void make_lowercase( string &line, char quote_state )
{
  for ( string::iterator i( line.begin() ), e( line.end() ); i != e; ++i )
    if ( !quote_state && (*i == '"' || *i == '\'') )
      quote_state = *i;
    else if ( quote_state && *i == quote_state )
      quote_state = '\0';
    else if ( !quote_state )
      *i = tolower(*i);
}

string read_line(istream &in, char &quote_state )
{
  string line;
  getline( in, line );

  strip_comment( line, quote_state );
  trim_whitespace( line, quote_state );
  make_lowercase( line, quote_state );

  // This one updates quote_state, the previous ones did not
  unmatched_quotes( line, quote_state );

  if ( quote_state ) line += "\n";

  return line;
}

string read_command(istream &in)
{
  char quote_state = '\0';
  string cmd = read_line( in, quote_state );

  while ( quote_state || cmd.size() && cmd[cmd.size()-1] == ',' )
    {
      string line = read_line( in, quote_state );
      ///if (quote_state) cmd += "\n"; // Replace the new line
      cmd += line;
    }

  return cmd;
}

void validate_name( const string &name )
{
  if ( !name.size() )
    throw runtime_error( "Attempted to define an unnamed variable" );

  if ( !isalpha( *name.begin() ) )
    throw runtime_error( "Variables must begin with a letter" );

  string::const_iterator i( name.begin() ), e( name.end() );
  // NB.  Microsirii allows '-', '!' and '%' as well.  I don't.
  while ( i != e && ( isalnum(*i) || *i == '_' ) )
    ++i;
 
  if ( i != e )
    throw runtime_error( "Variables must only contain letters, "
			 "numbers or an underscore" );
}

//////////////////////////////////////////////////////////

struct token_type
{ 
  enum enum_t 
  { 
    open_paren,
    close_paren,
    comma,
    times,
    string_lit,
    pn_lit,
    num_lit,
    name,
    transp_lit
  };
};

typedef pair< token_type::enum_t, string > token;

vector< token > 
tokenise_command( const string &input )
{
  string::const_iterator i( input.begin() ), e( input.end() );
  vector< token > tokens;

  while ( i != e )
    {
      // Skip leading whitespace
      while ( i != e && isspace(*i) ) ++i;  
      if ( i == e ) break;

      switch (*i)
	{
	case '(':
	  ++i;
	  tokens.push_back( token( token_type::open_paren, "(" ) );
	  break;

	case ')':
	  ++i;
	  tokens.push_back( token( token_type::close_paren, ")" ) );
	  break;

	case ',':
	  ++i;
	  tokens.push_back( token( token_type::comma, "," ) );
	  break;

	case '*':
	  ++i;
	  tokens.push_back( token( token_type::times, "*" ) );
	  break;
	  
	case '"':
	  {
	    ++i;
	    string::const_iterator j = find( i, e, '"' );
	    if ( j == e ) 
	      throw runtime_error( "Unterminated string literal" );
	    tokens.push_back( token( token_type::string_lit, string(i, j) ) );
	    i = ++j;
	  }
	  break;

	case '\'':
	  {
	    ++i;
	    string::const_iterator j = find( i, e, '\'' );
	    if ( j == e ) 
	      throw runtime_error( "Unterminated transposition literal" );
	    tokens.push_back( token( token_type::transp_lit, string(i, j) ) );
	    i = ++j;
	  }
	  break;

	case '+': case '&':
	  {
	    string::const_iterator j = i;
	    while ( j != e && ( *j != ',' && *j != ')' ) ) ++j;
	    tokens.push_back( token( token_type::pn_lit, string(i, j) ) );
	    i = j;
	  }
	  break;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  {
	    string::const_iterator j = i;
	    while ( j != e && isdigit(*j) ) ++j;
	    tokens.push_back( token( token_type::num_lit, string(i, j) ) );
	    i = j;
	  }
	  break;

	case 'a': case 'b': case 'c': case 'd': case 'e':
	case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o':
	case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y': 
	case 'z':
	  {
	    string::const_iterator j = i;
	    while ( j != e && ( isalnum(*j) || *j == '_' ) ) ++j;
	    tokens.push_back( token( token_type::name, string(i, j) ) );
	    i = j;
	  }
	  break;

	default:
	  throw runtime_error( "Syntax error" );
	}
    }

  return tokens;
}

//////////////////////////////////////////////////////////

class default_parser : public parser
{
public:
  default_parser() : b(-1) {}

private:
  virtual statement parse( istream& in );
  int bells() const { return b; }

  bool is_bells_directive(const string& cmd) const;
  statement handle_bells_directive(const string& cmd);

  bool is_prove_command(const string& cmd) const;
  statement handle_prove_command(const string& cmd);

  bool is_definition(const string& cmd) const;
  statement handle_definition(const string& cmd);

  bool is_import_command(const string& cmd) const;
  statement handle_import_command(const string& cmd);

  expression make_expr( const vector< token >& tokens ) const;
  expression make_expr( vector< token >::const_iterator first, 
			vector< token >::const_iterator last ) const;

  int b;
};

inline bool default_parser::is_bells_directive(const string& cmd) const 
{
  return cmd.size() > 5 && cmd.substr( cmd.size() - 5 ) == "bells" 
    && !isalpha( cmd[ cmd.size() - 6 ] );
}

statement default_parser::handle_bells_directive(const string& cmd)
{
  int n = string_to_int( cmd.substr( 0, cmd.size() - 5 ) );
  
  if ( n > (int) bell::MAX_BELLS )
    throw runtime_error( make_string() 
			 << "Number of bells must be less than "
			 << bell::MAX_BELLS + 1 );
  else if ( n <= 1 )
    throw runtime_error( "Number of bells must be greater than 1" );
  
  b = n;

  return statement( new bells_stmt(b) );
}

inline bool default_parser::is_import_command(const string& cmd) const
{
  return cmd.size() > 7 && cmd.substr( 0, 7 ) == "import "
    && cmd.find("=") == string::npos;
}

statement default_parser::handle_import_command(const string& cmd)
{
  string name( cmd.substr(7) );
  trim_leading_whitespace(name);
  trim_trailing_whitespace(name);

  if (name.size() && name[0] == '"')
    {
      if (name[ name.size()-1 ] != '"' )
	throw runtime_error( "Resource name is incorrectly quoted" );
      name = name.substr(1,name.size()-2);
    }

  return statement( new import_stmt(name) );
}

inline bool default_parser::is_prove_command(const string& cmd) const
{
  return cmd.size() > 6 && cmd.substr( 0, 6 ) == "prove "
    && cmd.find("=") == string::npos;
}

statement default_parser::handle_prove_command(const string& cmd)
{
  string expr( cmd.substr(6) );
  trim_leading_whitespace( expr );
  return statement
    ( new prove_stmt( make_expr( tokenise_command( expr ) ) ) );
}

inline bool default_parser::is_definition(const string& cmd) const
{
  return !( cmd.find( '=' ) == string::npos );
}

statement default_parser::handle_definition(const string& cmd)
{
  string::size_type idx = cmd.find_first_of( '=' );

  if ( idx == string::npos )
    throw runtime_error( "Line contains no '='" );

  string name( cmd.substr( 0, idx ) );
  
  trim_trailing_whitespace( name );
  validate_name( name );

  return statement( new definition_stmt
    ( name, make_expr( tokenise_command( cmd.substr( idx + 1 ) ) ) ) );
}


statement default_parser::parse( istream& in )
{
  string cmd = read_command(in);
      
  // Actual end of file
  if ( !in ) 
    return statement( NULL );

  // Trap empty commands
  else if ( cmd.empty() )
    return statement( new null_stmt );
  
  // End marker
  else if ( cmd == "end" || cmd == "quit" || cmd == "exit" )
    return statement();

  // Is it a `bells' directive?
  else if ( is_bells_directive(cmd) )
    return handle_bells_directive(cmd);

  // Is it a `prove' command?
  else if ( is_prove_command(cmd) )
    return handle_prove_command(cmd);

  // Is it a `import' command?
  else if ( is_import_command(cmd) )
    return handle_import_command(cmd);

  // Or a definition?
  else if ( is_definition(cmd) )
    return handle_definition(cmd);

  else
    throw runtime_error( "Unknown command" );
}

//////////////////////////////////////////////////////////

expression 
default_parser::make_expr( vector< token >::const_iterator first, 
			   vector< token >::const_iterator last ) const
{
  typedef vector< token >::const_iterator iter_t;

  if ( first == last ) 
    throw runtime_error( "Expression expected" );

  // Parentheses first
  if ( first->first == token_type::open_paren && 
       (last-1)->first == token_type::close_paren )
    {
      int depth = 0;
      bool ok = true;
      for ( iter_t i(first); ok && i != last; ++i )
	{
	  if ( i->first == token_type::open_paren )
	    ++depth;
	  else if ( i->first == token_type::close_paren )
	    --depth;

	  if ( depth == 0 && i != last-1 ) 
	    ok = false;
	}

      if (ok && depth) 
	throw runtime_error( "Unmatched parentheses 1" );

      if (ok)
	return make_expr( first+1, last-1 );
    }

  // Comma is the lowest precedence operator
  // It is left associative
  {
    int depth = 0;
    for ( iter_t i(first); i != last; ++i )
      {
	if ( i->first == token_type::open_paren )
	  ++depth;
	else if ( i->first == token_type::close_paren )
	  --depth;
	else if ( i->first == token_type::comma && depth == 0 )
	  return expression
	    ( new list_node( make_expr( first, i ),
			     make_expr( i+1, last ) ) );
      }

    if (depth) 
      throw runtime_error( "Unmatched parentheses" );
  }

  // A number literal in a repeated block is the
  // only remaining construct that is not a single token.
  if ( first->first == token_type::num_lit )
    {
      if ( first+1 != last && (first+1)->first == token_type::times )
	return expression
	  ( new repeated_node( string_to_int( first->second ),
			       make_expr( first+2, last ) ) );
      else
	return expression
	  ( new repeated_node( string_to_int( first->second ),
			       make_expr( first+1, last ) ) );
    }

  // Everything left is a literal of some sort
  if ( last - first != 1 )
    throw runtime_error( "Parse error" );

  switch ( first->first )
    {
    case token_type::string_lit:
      return expression( new string_node( first->second ) );

    case token_type::name:
      return expression( new symbol_node( first->second ) );

    case token_type::pn_lit:
      return expression( new pn_node( bells(), first->second ) );

    case token_type::transp_lit:
      return expression( new transp_node( bells(), first->second ) );

    default:
      throw runtime_error( "Unknown token in input" );
      return expression( NULL ); // To keep MSVC 5 happy
    }
}

expression default_parser::make_expr( const vector< token >& tokens ) const
{
  if ( tokens.empty() )
    return expression( new nop_node );
  else
    return make_expr( tokens.begin(), tokens.end() );
}

//////////////////////////////////////////////////////////

RINGING_END_ANON_NAMESPACE

shared_pointer<parser> make_default_parser()
{
  return shared_pointer<parser>( new default_parser );
}
