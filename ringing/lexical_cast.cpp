// -*- C++ -*- lexical_cast.cpp - Convert types to and from strings
// Copyright (C) 2002, 2005, 2009 Richard Smith <richard@ex-parrot.com>

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

// Much of this file is taken from the boost lexical_cast library,
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/lexical_cast.h>


RINGING_START_NAMESPACE

RINGING_USING_STD

const char* bad_lexical_cast::what() const throw()
{
  return "bad cast: "
    "source type value could not be interpreted as target";
}

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
