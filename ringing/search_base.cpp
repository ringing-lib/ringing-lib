// -*- C++ -*- search_base.cpp - Base class for searching
// Copyright (C) 2001, 2002 Richard Smith <richard@ex-parrot.com>

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

#ifdef RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/search_base.h>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

void search_base::run( search_base::outputer &o ) const
{
  scoped_pointer< context_base > ctx( new_context() );
  ctx->run( o );
}

RINGING_END_NAMESPACE
