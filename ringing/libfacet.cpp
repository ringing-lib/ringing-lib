// library.cpp : Library extensibility mechanism
// Copyright (C) 2004 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/libfacet.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

bool operator==( const library_facet_id &a, const library_facet_id &b )
{
  if ( !a.id ) a.id = library_facet_id::assign_id();
  if ( !b.id ) b.id = library_facet_id::assign_id();
  return a.id == b.id;
}

library_facet_id::library_facet_id()
{
  if ( !id ) id = assign_id();
}

int library_facet_id::assign_id()
{
  static int next_id;
  return ++next_id;
}

RINGING_END_NAMESPACE
