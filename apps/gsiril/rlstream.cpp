// rlstream.cpp - A streambuf that uses GNU readline
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

#if !RINGING_USE_READLINE
#error "This file requires GNU readline" 
#endif
#include "rlstream.h"
#if RINGING_READLINE_NEEDS_STDIO_H
#if RINGING_OLD_C_INCLUDES
#include <stdio.h>
#else
#include <cstdio>
#endif
#endif
#include <readline/readline.h>
#include <readline/history.h>
#if RINGING_OLD_C_INCLUDES
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#else
#include <cstring>
#include <cctype>
#include <cstdlib>
#endif

RINGING_USING_NAMESPACE

rl_streambuf::~rl_streambuf() 
{ 
  if (eback()) free( eback() );
  streambuf::setg( NULL, NULL, NULL );
}

rl_streambuf::int_type rl_streambuf::underflow_common( int bump )
{
  if (eback()) free( eback() );
  streambuf::setg( NULL, NULL, NULL );

  char *b = const_cast<char *>
    ( readline( const_cast<char*>( prompt.c_str() ) ) );
#if RINGING_HAVE_OLD_IOSTREAMS
  if (!b) return -1;
#else
  if (!b) return traits::eof();
#endif

  streambuf::setg( b, b+bump, b + strlen(b) + 1 );
  for ( const char *p=eback(); p < egptr()-1; ++p )
    if ( !isspace(*p) )
      {
	add_history(b);
	break;
      }
  egptr()[-1] = '\n';
  return *b;
}

