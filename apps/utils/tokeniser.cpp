// tokeniser.cpp - Tokenise lines of input
// Copyright (C) 2003 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/streamutils.h> // To get around bug in MSVC getline
#include "tokeniser.h"
#if RINGING_OLD_INCLUDES
#include <iterator.h>
#include <vector.h>
#else
#include <iterator>
#include <vector>
#endif
#include <string>
#if RINGING_HAVE_OLD_IOSTREAMS
#include <istream.h>
#else
#include <istream>
#endif
#if RINGING_OLD_C_INCLUDES
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#else
#include <cstddef>
#include <cstring>
#include <cctype>
#include <cassert>
#endif

RINGING_USING_NAMESPACE


void tokeniser::const_iterator::increment()
{
  if ( !tok->parse(val) )
    tok = NULL;
}

tokeniser::basic_token::basic_token( const char* initial_sequence, int type )
  : init_seq( initial_sequence ), t(type),
    len( strlen(initial_sequence) )
{
}

bool tokeniser::basic_token::matches( string::const_iterator i,
				      string::const_iterator e ) const
{
  return size_t(e-i) >= len && strncmp( &*i, init_seq, len ) == 0;
}

tokeniser::basic_token::parse_result 
tokeniser::basic_token::parse( string::const_iterator& i, 
			       string::const_iterator e, 
			       token& tok ) const
{
  if ( !matches(i, e) )
    return failed;

  tok = string(init_seq); tok.type(type()); i += len;
  return done;
}

tokeniser::string_token::string_token(const char* qstr, int type,
				      multi_line_policy mlp ) 
  : basic_token(qstr, type), mlp(mlp)
{
  assert( strlen(qstr) == 1 );
  q = qstr[0];
}

tokeniser::basic_token::parse_result 
tokeniser::string_token::parse( string::const_iterator& i, 
				string::const_iterator e, 
				token& tok ) const
{
  if ( *i != q )
    return failed;
  
  string::const_iterator j = i; ++j;
  while ( j < e && *j != q && ( mlp == multi_line || *j != '\n' ) )
    ++j;
  
  if (j == e)
    return more;
  
  if (*j != q) 
    return failed; // due to mlp

  tok = string(++i, j); tok.type( type() ); i = ++j;
  return done;
}

tokeniser::tokeniser( istream& in, 
		      tokeniser::new_line_policy nlp,
		      tokeniser::case_policy cp )
  : nlp( nlp ), cp( cp ),
    id_first_chars( NULL ), id_other_chars( NULL ),
    buffer(), i( buffer.begin() ), e( buffer.end() ), 
    in( &in ) 
{
}

void tokeniser::add_qtype( const basic_token* qtype )
{ 
  qtypes.push_back(qtype); 
}

void tokeniser::set_id_chars( const char *idfchs, const char *idochs )
{ 
  id_first_chars = idfchs; id_other_chars = idochs; 
}

void tokeniser::validate( const token& ) const
{
}

bool tokeniser::parse( token& tok )
{
  while ( i < e && isspace(*i) && !(nlp == keep_new_lines && *i == '\n' ) )
    ++i;

  while ( i == e )
    {
      getline(*in, buffer); buffer += '\n';
      if (!*in) return false;

      i = buffer.begin();
      e = buffer.end();
      
      while ( i < e && isspace(*i) && !(nlp == keep_new_lines && *i == '\n' ) )
	++i;
    }

  // Parse a quoted token
  for ( vector< const basic_token* >::const_iterator 
	  qi = qtypes.begin(), qe = qtypes.end(); qi != qe; ++qi )
    if ( (*qi)->matches( i, e ) )
      {
	while (true)
	  {
	    basic_token::parse_result pr = (*qi)->parse( i, e, tok );

	    if (pr == basic_token::done)
	      return true;
		
	    else if (pr == basic_token::failed || !*in)
	      break;

	    size_t offset = i - buffer.begin();
	    string line; getline(*in, line); line += '\n';
	    buffer += line;
	    i = buffer.begin() + offset;
	    e = buffer.end();
	  }
      }

  // Parse an ID
  if ( id_first_chars ? bool( strchr( id_first_chars, *i ) ) 
                      : bool( isalpha(*i) || *i == '_' ) )
    {
      string::const_iterator j = i++;
      while ( i < e && (id_other_chars ? bool( strchr(id_other_chars, *i) )
		                       : bool( isalnum(*i) || *i == '_' ) ) ) 
	++i;

      tok = string( j, i );
      if ( cp == case_insensitive )
	for ( string::iterator i1( tok.begin() ), e1( tok.end() );
	      i1 != e1; ++i1 )
	  *i1 = tolower(*i1);
      
      tok.type( id_first_chars ? id_first_chars[0] : 'A' );
      return true;
    }

  // Parse a number
  if ( isdigit(*i) )
    {
      string::const_iterator j = i++;
      while ( i < e && isdigit(*i) )
	++i;

      tok = string( j, i );
      tok.type('0');
      return true;
    }

  // Odd character
  tok = string( 1u, *i );
  tok.type( *i );
  ++i;
  return true;
}

