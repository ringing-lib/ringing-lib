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
