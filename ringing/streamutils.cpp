// -*- C++ -*- streamutils.cpp - Utilities to cope with old iostream libraries
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

// The getline function is taken from the GNU standard C++ library [see 
// http://gcc.gnu.org/libstdc++/ for details], and is under the following 
// copyright:

//   Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002
//   Free Software Foundation, Inc.

// $Id$


#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include <ringing/streamutils.h>


RINGING_START_NAMESPACE

RINGING_USING_STD

make_string::operator string()
{
#if RINGING_USE_STRINGSTREAM
  return os.str();
#else
  string s( os.str(), os.pcount() ); 
  os.freeze(0); 
  return s;
#endif
}


RINGING_END_NAMESPACE


#if defined(_MSC_VER) && _MSC_VER <= 1200 && defined(_YVALS) && (!defined(_CPPLIB_VER) || _CPPLIB_VER < 306)
istream& getline( istream& in, string &str, char delim )
{
  size_t extracted = 0;
  bool testdelim = false;
  istream::sentry cerb(in, true);
  if (cerb) 
    {
      str.erase();
      size_t n = str.max_size();

      int idelim = string::traits_type::to_int_type(delim);
      streambuf* sb = in.rdbuf();
      int c = sb->sbumpc();
      const int eof = string::traits_type::eof();
      testdelim = string::traits_type::eq_int_type(c, idelim);

      while (extracted <= n 
	     && !string::traits_type::eq_int_type(c, eof)
	     && !testdelim)
	{
	  str += string::traits_type::to_char_type(c);
	  ++extracted;
	  c = sb->sbumpc();
	  testdelim = string::traits_type::eq_int_type(c, idelim);
	}
      if (string::traits_type::eq_int_type(c, eof))
	in.setstate(ios_base::eofbit);
    }
  if (!extracted && !testdelim)
    in.setstate(ios_base::failbit);
  return in;
}
#endif
