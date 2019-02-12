// -*- C++ -*- bell.cpp - A simple class representing a bell's position
// Copyright (C) 2001, 2008, 2009, 2011 Martin Bright <martin@boojum.org.uk>
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
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#else
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cctype>
#endif
#if RINGING_OLD_IOSTREAMS
#include <istream.h>
#include <iomanip.h> // For noskipws
#else
#include <istream>
#include <iomanip>
#endif
#if RINGING_OLD_INCLUDES
#include <algorithm.h>
#else
#include <algorithm>
#endif

#include <ringing/bell.h>
#include <ringing/istream_impl.h>

RINGING_USING_STD
  
RINGING_START_NAMESPACE

static int (*bells_case_mapper)(int) = &::toupper;
char const* bell::symbols = "1234567890ETABCDFGHJKLMNPQRSUVWYZ";
unsigned int bell::MAX_BELLS = 33;

static string string_transform( char const* str, int (*func)(int) ) {
  string s(str);
  std::transform(s.begin(), s.end(), s.begin(), func);
  return s;
}

void bell::set_symbols( char const* syms, size_t n ) {
  if ( syms ) {
    if ( string_transform(syms, &::toupper) == syms ) 
      bells_case_mapper = &::toupper;
    else if ( string_transform(syms, &::tolower) == syms )
      bells_case_mapper = &::tolower;
    else 
      bells_case_mapper = NULL;

    symbols = syms;
    MAX_BELLS = n == size_t(-1) ? strlen(syms) : n;
  } else {
    symbols = "1234567890ETABCDFGHJKLMNPQRSUVWYZ";
    MAX_BELLS = 33;
    bells_case_mapper = &::toupper;
  }
}

const char* bell::symbols_env_var = "BELL_SYMBOLS";

void bell::set_symbols_from_env( char const* env ) {
  if ( char const* var = getenv(env) ) set_symbols(var);
}

bell::invalid::invalid()
  : invalid_argument("The bell supplied was invalid") {}

bool bell::is_symbol(char c)
{
  if ( bells_case_mapper ) c = bells_case_mapper(c);
  if ( strchr(symbols, c) ) return true;

  // I/1 and O/0 ambiguities:
  switch (c) {
    case 'I': case '1':
      return ( !strchr(symbols, 'I') ^ !strchr(symbols, '1') );
    case 'O': case '0':
      return ( !strchr(symbols, 'O') ^ !strchr(symbols, '0') );
  }
 
  return false;
}

bell bell::read_char(char c) 
{
  if ( bells_case_mapper ) c = bells_case_mapper(c);
  bell b;
  if ( char const* cp = strchr(symbols, c) )
    b.x = cp - symbols;
  else {
    // I/1 and O/0 ambiguities:
    switch (c) {
      case 'I': if (is_symbol('1')) return read_char('1');
      case '1': if (is_symbol('I')) return read_char('I');
      case 'O': if (is_symbol('0')) return read_char('0');
      case '0': if (is_symbol('O')) return read_char('O');
    }
#if RINGING_USE_EXCEPTIONS
    throw invalid();
#else
    b.x = MAX_BELLS + 1;
#endif
  }
  return b;
}

bell bell::read_extended(char const* str, char const** endp)
{
  char const* dummy;
  if (!endp) endp = &dummy;

  if ( *str != '{' ) {
    bell b( read_char(*str) ); 
    *endp = ++str;
    return b;
  } else {
    unsigned long val = strtoul(++str, const_cast<char**&>(endp), 10);
    while (isspace(**endp)) ++*endp;
    if ( *str == '-' || **endp != '}' ||
         RINGING_BELL_BITS < sizeof(unsigned long) * CHAR_BIT
         && val > (1ul<<RINGING_BELL_BITS) )
#if RINGING_USE_EXCEPTIONS
      throw invalid();
#else
      return bell(-1);  // MAX_BELLS+1 would be a bad choice.
#endif
    ++*endp; 
    return bell( val-1 );
  }
}

RINGING_API ostream& operator<<(ostream& o, bell const& b)
{
  if (b < bell::MAX_BELLS)
    o << b.to_char();
  else
    o << '{' << ( (unsigned long)b + 1 ) << '}';
  return o;
}

RINGING_API istream& operator>>(istream& i, bell& b)
{
  istream_flag_sentry cerberus(i);
  if ( cerberus ) {
#if RINGING_USE_EXCEPTIONS
    try {
#endif
      if (i.peek() != '{') 
        b.from_char( (char) i.get() );
      else {
        i.get(); // drop the '{'
        unsigned long val;  i >> noskipws >> val;
        if ( i.get() != '}' ||
             RINGING_BELL_BITS < sizeof(unsigned long) * CHAR_BIT
             && val > (1ul<<RINGING_BELL_BITS) )
#if RINGING_USE_EXCEPTIONS
          throw bell::invalid();
#else
          i.setstate( ios_base::badbit );
#endif
        b = bell( val-1 );
      }

#if RINGING_USE_EXCEPTIONS 
    }
    catch ( ... ) {
      try { i.setstate( ios_base::badbit ); } catch(...) {}
      if ( i.exceptions() & ios_base::badbit ) throw;
    }
#endif
  }
  return i;
}

RINGING_END_NAMESPACE
