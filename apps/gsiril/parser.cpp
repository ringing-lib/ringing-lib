// parser.cpp - Tokenise and parse lines of input
// Copyright (C) 2002 Richard Smith <richard@ex-parrot.com>

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
#include "util.h"
#include "execution_context.h"
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
#include <ringing/row.h>
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE

RINGING_START_ANON_NAMESPACE

void strip_comment( string &line, bool quoted )
{
  // Note:  This function does not use string::iterators due to
  // an optimiser in gcc-3.1 which previously caused a segfault here. 
  for ( const char *i = line.data(), *e = line.data() + line.size(); 
	i != e; ++i )
    {
      if ( *i == '"' )
	{
	  quoted = !quoted;
	}
      else if ( !quoted && *i == '/' )
	{
	  line.erase(i - line.data());
	  break;
	}
    }
  return;
}

bool unmatched_quotes( const string &str )
{
  bool quoted = false;

  for ( string::const_iterator i( str.begin() ), e( str.end() ); i != e; ++i )
    if ( *i == '"' )
      quoted = !quoted;

  return quoted;
}

void trim_whitespace( string &line, bool quoted )
{
  if ( !quoted ) 
    trim_leading_whitespace( line );

  if ( !quoted ^ unmatched_quotes(line) && line.size() ) 
    trim_trailing_whitespace( line );
}

void make_lowercase( string &line, bool quoted )
{
  for ( string::iterator i( line.begin() ), e( line.end() ); i != e; ++i )
    if ( *i == '"' )
      quoted = !quoted;
    else if ( !quoted )
      *i = tolower(*i);
}

string read_line(istream &in, bool quoted = false )
{
  string line;
  getline( in, line );
  strip_comment( line, quoted );
  trim_whitespace( line, quoted );
  make_lowercase( line, quoted );
  return line;
}

string read_command(istream &in)
{
  string cmd = read_line(in);
  bool quoted = unmatched_quotes( cmd );

  while ( quoted || cmd.size() && cmd[cmd.size()-1] == ',' )
    {
      string line = read_line(in, quoted);
      if (quoted) cmd += "\n";
      quoted ^= unmatched_quotes(line);
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

RINGING_END_ANON_NAMESPACE

parser::parser() : b(-1) {}
parser::~parser() {}

pair< const string, expression >
parser::parse_definition( const string &cmd ) const
{
  string::size_type idx = cmd.find_first_of( '=' );

  if ( idx == string::npos )
    throw runtime_error( "Line contains no '='" );

  string name( cmd.substr( 0, idx ) );
  
  trim_trailing_whitespace( name );
  validate_name( name );

  return pair< const string, expression >
    ( name, expression( *this, tokenise_command( cmd.substr( idx + 1 ) ) ) );
}

bool parser::maybe_handle_prove_command( const string &cmd, ostream &out )
{
  if ( cmd.size() > 6 && cmd.substr( 0, 6 ) == "prove "
       && cmd.find("=") == string::npos )
    {
      string name( cmd.substr(6) );
      trim_leading_whitespace( name );

      run_proof( out, expression( *this, tokenise_command( name ) ) );
      return true;
    }
  else
    return false;
}

bool parser::maybe_handle_bells_command( const string &cmd, ostream &out )
{
  if ( cmd.size() > 5 && cmd.substr( cmd.size() - 5 ) == "bells" 
       && !isalpha( cmd[ cmd.size() - 6 ] ) )
    {
      int n = string_to_int( cmd.substr( 0, cmd.size() - 5 ) );

      if ( n > bell::MAX_BELLS )
	throw runtime_error( make_string() 
			     << "Number of bells must be less than "
			     << bell::MAX_BELLS + 1 );
      else if ( n <= 1 )
	throw runtime_error( "Number of bells must be greater than 1" );
      
      b = n;

      if (interactive)
	out << "Set bells to " << b << endl;
      return true;
    }
  else
    return false;
}

bool parser::maybe_handle_defintion( const string &cmd, ostream &out )
{
  if ( cmd.find( '=' ) == string::npos )
    return false;

  pair< const string, expression > defn( parse_definition( cmd ) );

  sym_table_t::iterator i = sym_table.find( defn.first );

  // Is it a redefinition?
  if ( i != sym_table.end() )
    {
      i->second = defn.second;
      if ( interactive )
	cout << "Refinition of '" << defn.first << "'." << endl;
    }
  else
    {
      sym_table.insert( defn );
  
      if ( interactive )
	cout << "Defintion of '" << defn.first << "' added." << endl;
    }
  
  // Is this the first symbol?  If so, this is the entry point
  if ( entry_sym.empty() )
    entry_sym = defn.first;

  return true;
}

void parser::read_file( istream &in, ostream &out )
{
  while (true)
    {
      try 
	{
	  string cmd = read_command(in);
	  
	  // Actual end of file
	  if ( !in )
	    { out << endl; break; }
	  
	  // Trap empty commands
	  else if ( cmd.empty() )
	    continue;
	  
	  // End marker
	  else if ( cmd == "end" || cmd == "quit" || cmd == "exit" )
	    break;

	  // A command to set the number of bells
	  else if ( maybe_handle_bells_command( cmd, out ) )
	    ;
	  
	  // A command to prove a touch 
	  else if ( maybe_handle_prove_command( cmd, out ) )
	    ;

	  // A symbol defintion
	  else if ( maybe_handle_defintion( cmd, out ) )
	    ;

	  else
	    throw runtime_error( "Unknown command" );
	}
      catch ( const exception &ex )
	{
	  out << "Error: " << ex.what() << endl;
	  if (!interactive) break;
	}
    }
}

void parser::init_with( const string &str )
{
  sym_table.insert( parse_definition( str ) );
}


bool parser::run_proof( ostream &out, const expression &node ) const

{
  try 
    {
      execution_context ectx( *this, out );
      node.execute( ectx );
      ectx.execute_symbol( ectx.final_symbol() );
    }
  catch ( const script_exception & )
    {
      return false;
    }
  catch ( const exception &ex )
    {
      out << "Error whilst proving: " << ex.what() << endl;
      return false;
    }

  return true;
}

// MicroSiril always proved the first entry in the file
bool parser::run_final_proof( ostream &out ) const
{
  if ( ! entry_sym.empty() && ! interactive )
    return run_proof( cout, lookup_symbol( entry_sym ) );

  return true;
}

expression parser::lookup_symbol( const string &sym ) const
{
  sym_table_t::const_iterator i = sym_table.find(sym);
  if ( i == sym_table.end() )
    throw runtime_error( make_string() << "Unknown symbol: " << sym );
  return i->second;
}
