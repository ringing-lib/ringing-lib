// -*- C++ -*- console_stream.h - A more interactive version of std::cin 
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

#ifndef GSIRIL_CONSOLE_STREAM_INCLUDED
#define GSIRIL_CONSOLE_STREAM_INCLUDED

#ifdef RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_HAVE_OLD_IOSTREAMS
#include <istream.h>
#else
#include <iosfwd>
#include <istream>
#endif
#include <ringing/pointers.h>

RINGING_USING_NAMESPACE

class console_istream : public istream
{
public:
#if !RINGING_HAVE_OLD_IOSTREAMS
  // Standard typedefs
  typedef char                   char_type;
  typedef char_traits<char_type> traits;
  typedef traits::int_type       int_type;
  typedef traits::pos_type       pos_type;
  typedef traits::off_type       off_type;
#endif

  // Constructor / destructor
  explicit console_istream( bool interactive );
 ~console_istream();

  // Public interface
  void set_prompt( const char *prompt );

private:
  // Helpers
  static streambuf *make_streambuf();

  // Data members
  scoped_pointer<streambuf> buf;
};


#endif // GSIRIL_CONSOLE_STREAM_INCLUDED
