// -*- C++ -*- bell.cpp - A simple class representing a bell's position
// Copyright (C) 2001, 2008, 2009 Martin Bright <martin@boojum.org.uk>
// and Richard Smith <richard@ex-parrot.com>

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// $Id$


#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_C_INCLUDES
#include <string.h>
#else
#include <cstring>
#endif

#include <ringing/bell.h>

RINGING_USING_STD
  
RINGING_START_NAMESPACE

char const* bell::symbols = "1234567890ETABCDFGHJKLMNPQRSUVWYZ";
unsigned int bell::MAX_BELLS = 33;

void bell::set_symbols( char const* syms, size_t n ) {
  if ( syms ) {
    symbols = syms;
    MAX_BELLS = n == size_t(-1) ? strlen(syms) : n;
  } else {
    symbols = "1234567890ETABCDFGHJKLMNPQRSUVWYZ";
    MAX_BELLS = 33;
  }
}
    
bell::invalid::invalid()
  : invalid_argument("The bell supplied was invalid") {}

bool bell::is_symbol(char c)
{
  c = toupper(c);
  bell b;
  for ( ; b.x < MAX_BELLS && symbols[b.x] != c; ++b.x)
    ;
  return b.x != MAX_BELLS;
}

bell bell::read_char(char c) 
{
  c = toupper(c);
  bell b;
  for ( ; b.x < MAX_BELLS && symbols[b.x] != c; ++b.x)
    ;
  if (b.x == MAX_BELLS)
#if RINGING_USE_EXCEPTIONS
    throw invalid();
#else
    b.x = MAX_BELLS + 1;
#endif
  return b;
}

RINGING_END_NAMESPACE
