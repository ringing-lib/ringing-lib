// -*- C++ -*- falseness.cpp - things to analyse falseness
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation "methsearch/falseness"
#endif

#include <ringing/method.h>
#include "falseness.h"
#if RINGING_OLD_INCLUDES
#include <map.h>
#include <algo.h>
#else
#include <map>
#include <algorithm>
#endif
#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#else
#include <cassert>
#endif
#include <ringing/row.h>
#include <ringing/falseness.h>


RINGING_USING_NAMESPACE
RINGING_USING_STD

string falseness_group_codes( const method &m )
{
  return false_courses
    ( m, m.bells() == 8 ? 0 : false_courses::tenors_together ).symbols();
}


// ---------------------------------------------------------------------
//
// Extent possibility
//
// This algorithm only works for methods with calls that have effect for a 
// single change at the lead end (so neither the bobs nor the singles in 
// Grandsire count).
// 
// The idea is to look at the graph whose vertices are the set of possible 
// lead heads and whose edges are mutually false lead heads.  For an extent
// to be possible using an arbitrary number of single-change lead end calls,
// the graph must be bipartite.
// 
class falseness_analysis
{
private:
  friend bool might_support_extent( const method &m );
  friend bool might_support_positive_extent( const method &m );

  falseness_analysis( const method &m, bool in_course_only )
    : ft(m, in_course_only ? falseness_table::in_course_only : 0) 
  {    
  }
  
  bool recurse( const row &r, int sign )
  {
    map< row, int >::iterator si = signs.find(r);

    if ( si == signs.end() )
      {
	signs[r] = sign;

	for( falseness_table::const_iterator i( ft.begin() ), e( ft.end() );
	     i != e; ++i )
	  {
	    if ( !i->isrounds() )
	      if ( !recurse( r * *i, -sign ) )
		return false;
	  }

	return true;
      }

    return si->second == sign;
  }


  falseness_table ft;
  map< row, int > signs;
};

bool might_support_positive_extent( const method &m )
{
  return falseness_analysis(m, true).recurse( row( m.bells() ), +1 ); 
}

bool might_support_extent( const method &m )
{
  assert( m.isplain() ); 
  return falseness_analysis(m, false).recurse( row( m.bells() ), +1 ); 
}

bool is_cps( const method &m )
{
  if ( m.bells() >= 8 ) {
    const false_courses fchs( m, false_courses::in_course_only 
			      |  false_courses::tenors_together );
  
    return fchs.size() == 1;
  } else { 
    const false_courses fchs( m, false_courses::in_course_only );
    if ( m.back().sign() == +1 )
      return fchs.size() == 2;
    else      
      return fchs.size() == 1;
  }
}

