// -*- C++ -*- extent.h - Classes for iterating through an extent
// Copyright (C) 2001-2 Richard Smith <richard@ex-parrot.com>

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
#include <ringing/extent.h>
#if RINGING_OLD_INCLUDES
#include <algorithm.h>
#else
#include <algorithm>
#endif


RINGING_START_NAMESPACE

RINGING_USING_STD

struct extent_iterator::bellsym_cmp
{
  bool operator()( char a, char b )
  {
    bell aa, bb;
    aa.from_char(a);
    bb.from_char(b);
    return aa < bb;
  }
};

extent_iterator::extent_iterator( unsigned int nw, unsigned int nh, 
				  unsigned int nt )
  : nw(nw), nh(nh), nt(nt), end(false), 
    r(nt), s(r.print())
{}

extent_iterator::extent_iterator( unsigned int nw, unsigned int nh )
  : nw(nw), nh(nh), nt(nw+nh), 
    end(false),
    r(nt), s(r.print())
{}

extent_iterator &extent_iterator::operator++()
{
  end = ! next_permutation( s.begin() + nh, s.end() + (nh + nw - nt),
			    bellsym_cmp() );
  if (!end) r = s.c_str();
  return *this;
}

RINGING_END_NAMESPACE

