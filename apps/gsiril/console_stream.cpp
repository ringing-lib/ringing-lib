// console_stream.cpp - A more interactive version of std::cin 
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "console_stream.h"
#if RINGING_USE_READLINE
#include "rlstream.h"
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#include <streambuf.h>
#else
#include <iosfwd>
#include <iostream>
#include <streambuf>
#endif

RINGING_USING_NAMESPACE

void console_istream::set_prompt( const char *prompt ) 
{
#if RINGING_USE_READLINE
  if ( rl_streambuf *rlsb = dynamic_cast< rl_streambuf * >( buf.get() ) )
    rlsb->set_prompt(prompt); 
#endif
}

streambuf *console_istream::make_streambuf()
{
#if RINGING_USE_READLINE
  return new rl_streambuf;
#else
  return NULL;
#endif
}

console_istream::console_istream( bool interactive )
  : istream( NULL ),
    buf( interactive ? make_streambuf() : NULL )
{
  istream::init( buf.get() ? buf.get() : cin.rdbuf() );
}

console_istream::~console_istream()
{
}

