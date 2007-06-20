// -*- C++ -*- bell.h - A simple class representing a bell's position in a row
// Copyright (C) 2001, 2007 Martin Bright <martin@boojum.org.uk> 
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


#ifndef RINGING_BELL_H
#define RINGING_BELL_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once 
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_C_INCLUDES
#include <limits.h>
#else
#include <climits>
#endif

#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <stdexcept.h>
#else
#include <ostream>
#include <stdexcept>
#endif

#if RINGING_AS_DLL
#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif
#endif

// WARNING: Changing this requires rebuilding the whole library.
#ifndef RINGING_BELL_BITS 
#define RINGING_BELL_BITS  CHAR_BIT // defined in limits.h
#endif

#if RINGING_BELL_BITS != 8
// The header stdint.h does not exist on all older systems.
#include <stdint.h>
#endif


RINGING_START_NAMESPACE
  
RINGING_USING_STD

class RINGING_API bell {
public:
  static const unsigned int MAX_BELLS;

  bell() : x(0) {}
  bell(int i) : x(i) {}
  bell& from_char(char c) {
    c = toupper(c);
    for(x = 0; x < MAX_BELLS && symbols[x] != c; x++);
    if(x == MAX_BELLS)
#if RINGING_USE_EXCEPTIONS
      throw invalid();
#else
      x = MAX_BELLS + 1;
#endif
      return *this;
  }
  operator int() const { return x; }
  bell& operator=(int i) { x = i; return *this; }

  bell& operator++() { ++x; return *this; }
  bell operator++(int) { return x++; }

  char to_char() const { return (x < MAX_BELLS) ? symbols[x] : '*'; }

  struct invalid : public invalid_argument {
    invalid();
  };

private:
#if RINGING_BELL_BITS == CHAR_BIT
  typedef unsigned char underlying_type;
#else
  typedef RINGING_CONCAT( uint, RINGING_CONCAT(RINGING_BELL_BITS, _t) ) 
    underlying_type;
#endif

  underlying_type x;
  static char symbols[];        // Symbols for the individual bells
};

inline RINGING_API ostream& operator<<(ostream& o, const bell b)
  { return o << b.to_char(); }

#if RINGING_AS_DLL
RINGING_EXPLICIT_STL_TEMPLATE vector<bell>;
#endif

RINGING_END_NAMESPACE

#endif 

