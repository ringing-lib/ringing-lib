// -*- C++ -*- falseness.h - things to analyse falseness
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

#ifndef METHSEARCH_FALSENESS_INCLUDED
#define METHSEARCH_FALSENESS_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface "methsearch/falseness"
#endif

#include <ringing/pointers.h>
#include <string>

// Forward declare ringing::method
RINGING_START_NAMESPACE
class method;
class row;
RINGING_END_NAMESPACE

RINGING_USING_NAMESPACE
RINGING_USING_STD

string falseness_group_codes( const method &m );

bool might_support_positive_extent( const method &m );
bool might_support_extent( const method &m );

bool is_cps( const method &m );

#endif // METHSEARCH_FALSENESS_INCLUDED
