// -*- C++ -*- methodutils.h - utility functions missing from the ringing-lib
// Copyright (C) 2002, 2003, 2004, 2010, 2011 
// Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

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

bool have_same_places( const change &a, const change &b );
bool is_cyclic_le( const row &lh, int hunts );
bool is_division_false( const method &m, const change &c, 
                        size_t div_start, size_t cur_div_len );
bool is_too_many_places( const method &m, const change &c, size_t max );

bool has_rotational_symmetry( const method &m );

bool has_consec_places( const change &c, size_t max_count = 1u );
bool division_bad_parity_hack( const method &m, const change &c, 
                               size_t div_start, size_t cur_div_len );


// S -- invariant under reflection in a change
// M -- invariant under reflection through the middle of a row
// G -- invariant under reflection through the middle of a row followed by
//      translation by half a lead
// R -- invariant under rotation
string method_symmetry_string( const method& m );

string tenors_together_coursing_order( const method& m );

// Returns true if a < b using the traditional ordering
//   x < 12 < 1234 < 14 < 34
bool compare_changes( change const& a, change const& b );

char old_lhcode( method const& m );

unsigned long staticity( method const& m );

#endif // METHSEARCH_METHODUTILS_INCLUDED
