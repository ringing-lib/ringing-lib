// -*- C++ -*- methodutils.h - utility functions missing from the ringing-lib
// Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

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


#ifndef METHSEARCH_METHODUTILS_INCLUDED
#define METHSEARCH_METHODUTILS_INCLUDED

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include <string>
#include <cstddef>

// Forward declare ringing::method
RINGING_START_NAMESPACE
class row;
class change;
class method;
RINGING_END_NAMESPACE


RINGING_USING_NAMESPACE
RINGING_USING_STD

change merge_changes( const change &a, const change &b );
bool have_same_places( const change &a, const change &b );
bool is_pble( const row &lh, int hunts );
bool is_cyclic_le( const row &lh, int hunts );
bool is_division_false( const method &m, const change &c, size_t divlen );
bool is_too_many_places( const method &m, const change &c, size_t max );

bool has_rotational_symmetry( const method &m );

size_t max_blows_per_place( const method &m );
bool has_consec_places( const change &c, size_t max_count = 1u );
bool division_bad_parity_hack( const method &m, const change &c, 
			       size_t divlen );
string get_compressed_pn( const method &m );


#endif // METHSEARCH_METHODUTILS_INCLUDED
