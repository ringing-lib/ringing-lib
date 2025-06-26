// -*- C++ -*- rlstream.h - A streambuf that uses GNU readline
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

#ifndef GSIRIL_RLSTREAM_INCLUDED
#define GSIRIL_RLSTREAM_INCLUDED

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

// You almost certainly don't want to include this header directly.
// Use console_stream.h instead.
#if !RINGING_USE_READLINE
#error "This file requires GNU readline" 
#endif

#if RINGING_HAVE_OLD_IOSTREAMS
#include <streambuf.h>
#else
#include <iosfwd>
#include <streambuf>
#endif
#include <string>

RINGING_USING_NAMESPACE

class rl_streambuf : public streambuf
{
  // Typedefs
#if RINGING_HAVE_OLD_IOSTREAMS
private:
  typedef int int_type;
#else
public:
  typedef char                   char_type;
  typedef char_traits<char_type> traits;
  typedef traits::int_type       int_type;
  typedef traits::pos_type       pos_type;
  typedef traits::off_type       off_type;
#endif

public:
  // Constructors / Destructors
  rl_streambuf() {}
  virtual ~rl_streambuf();

  void set_prompt( const string &p ) { prompt = p; }

protected:
  // Overridden virtual functions
  virtual int_type    underflow() { return underflow_common(0); }
  virtual int_type    uflow()     { return underflow_common(1); }

private:
  // Helpers
  int_type underflow_common( int bump );

  // Unimplemented copy constructor and copy assignment operator
  rl_streambuf &operator=( const rl_streambuf & );
  rl_streambuf( const rl_streambuf & );

  // Data members
  string prompt;
};

#endif // GSIRIL_RLSTREAM_INCLUDED
