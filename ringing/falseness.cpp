// -*- C++ -*- falseness.cpp - Class for falseness table
// Copyright (C) 2001 Richard Smith <richard@ex-parrot.com>

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

#if __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/falseness.h>
#if RINGING_OLD_INCLUDES
#include <algo.h>
#include <iterator.h>
#include <set.h>
#else
#include <algorithm>
#include <iterator>
#include <set>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

falseness_table::falseness_table()
  : t(1, row())
{}

falseness_table::falseness_table( const method &m, int flags )
{
  // The falseness table is calculated using
  //   F = \{ a b^{-1} : a, b \in M \}
  // where M is the set of rows in the first lead of the method. 


  set<row> fs;

  row r1( m.bells() );
  for ( method::const_iterator i1( m.begin() ); i1 != m.end(); r1 *= *i1++ )
    {
      row r2( m.bells() );
      for ( method::const_iterator i2( m.begin() ); i2 != m.end(); r2 *= *i2++)
	{
	  row f = r1 / r2;
	  
	  if ( !( flags & no_fixed_treble ) && f[0] != 0 )
	    continue;
	  
	  if ( ( flags & in_course_only ) && f.sign() == -1 )
	    continue;
	  
	  fs.insert( f );
	}
    }

  
  // Put the falsenesses into the vector 
  t.reserve( fs.size() );
  copy( fs.begin(), fs.end(), back_inserter(t) );
}

RINGING_END_NAMESPACE
