// -*- C++ -*- streamutils.h - Utilities to cope with old iostream libraries
// Copyright (C) 2002, 2003, 2005 Richard Smith <richard@ex-parrot.com>

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

// Part of this file is taken from the boost lexical_cast library,
// version 1.28.0 [see http://www.boost.org for details], and is under
// the following copyright:

//  Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
//  Permission to use, copy, modify, and distribute this software for any
//  purpose is hereby granted without fee, provided that this copyright and
//  permissions notice appear in all copies and derivatives.
//
//  This software is provided "as is" without express or implied warranty.

// $Id$

#ifndef RINGING_STREAMUTILS_H
#define RINGING_STREAMUTILS_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_USE_STRINGSTREAM
#if RINGING_OLD_INCLUDES
#include <sstream.h>
#else
#include <sstream>
#endif
#else // RINGING_USE_STRINGSTREAM
#if RINGING_OLD_INCLUDES
#include <strstream.h>
#else
#include <strstream>
#endif
#endif // RINGING_USE_STRINGSTREAM
#if RINGING_OLD_INCLUDES
#include <typeinfo.h>
#include <ostream.h>
#else
#include <typeinfo> // for bad_cast
#include <ostream>
#endif
#include <string>

RINGING_START_NAMESPACE

RINGING_USING_STD

// 
// Example usage:
//
// void fn( const string & );
//
// int main()
// {
//   fn( make_string() << "Hello world " << 42 << "!" );
// }
//
class make_string
{
public:
  template <class T> 
  make_string &operator<<( const T &val )
  { 
    os << val; 
    return *this; 
  }

  // Some versions of EGCS have problems deducing the template
  // parameter correctly for this, so we provide a specific
  // overload to do it.
  make_string &operator<<( ostream &(*manip)(ostream &) )
  {
    manip( os );
    return *this;
  }

  RINGING_API operator string();

  ostream& out_stream() { return os; }

private:
#if RINGING_USE_STRINGSTREAM
  ostringstream os;
#else
  ostrstream os;
#endif
};


// exception used to indicate runtime lexical_cast failure
class bad_lexical_cast : public bad_cast
{
public:
  virtual const char* what() const throw();
};

// Last function argument is to make MSVC happy.
// This is important as omitting it does not give a compile time error
template<typename Target, typename Source>
Target lexical_cast(Source arg, Target* = 0)
{
#if RINGING_USE_STRINGSTREAM
  stringstream interpreter;
#else
  strstream interpreter; // for out-of-the-box g++ 2.95.2
# endif
  Target result;
  
  if(!(interpreter << arg) || !(interpreter >> result) ||
     !(interpreter >> ws).eof())
    throw bad_lexical_cast();
  
  return result;
}


RINGING_END_NAMESPACE


// This exists because Visual Studio's default STL (an old version of the
// Dinkumware STL) has a bug in it's implementation of istream<>::getline.
// If we're using that STL, this provides a work around.  This overload gets
// chosen in preference to std::getline because it is non-templated.
#if defined(_MSC_VER) && _MSC_VER <= 1200 && defined(_YVALS) && (!defined(_CPPLIB_VER) || _CPPLIB_VER < 306)
istream& getline( istream& in, string &str, char c = '\n' );
#endif

#endif
