// -*- C++ -*- falseness.cpp - things to analyse falseness
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

struct falseness_group_table::impl
{
  static const struct init_data
  {
    const char *r;
    const char *s;

  } reg_init[], bnw_init[];

  impl( const init_data *i )
  {
    for ( ; i->r; ++i )
      table[i->r] = *i->s;
  }

  map< row, char > table;
};

const falseness_group_table::impl::init_data 
falseness_group_table::impl::reg_init[] = {
# include "reg-init"
  { NULL, NULL }
}, 
falseness_group_table::impl::bnw_init[] = {
# include "bnw-init"
  { NULL, NULL }
};

falseness_group_table::falseness_group_table()
{
}

falseness_group_table::~falseness_group_table()
{
}

falseness_group_table &falseness_group_table::instance()
{
  static falseness_group_table tmp;
  return tmp;
}

void falseness_group_table::init( type t )
{
  switch (t)
    {
    case regular:
      instance().pimpl.reset( new impl( impl::reg_init ) );
      break;

    case bnw:
      instance().pimpl.reset( new impl( impl::bnw_init ) );
      break;

    default:
      assert(false);
      break;
    }
}

string falseness_group_table::codes( const method &m )
{
  assert( instance().pimpl.get() );
  
  false_courses ft(m, false_courses::in_course_only 
		   | false_courses::tenors_together );
  string rv;

  for( falseness_table::const_iterator i( ft.begin() ), e( ft.end() );
       i != e; ++i )
    {
      if ( !i->isrounds() )
	{
	  char c( instance().pimpl->table[*i] );
	  if ( rv.find(c) == string::npos )
	    rv.append( 1u, c );
	}
      }
  
  if (rv.empty())
    rv = "[CPS]";
  else
    sort( rv.begin(), rv.end() );
  
  return rv;
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

  falseness_analysis( const method &m )
    : ft(m) 
  {    
    assert( m.isplain() ); 
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

bool might_support_extent( const method &m )
{
  return falseness_analysis(m).recurse( row( m.bells() ), +1 ); 
}

// TODO  Use the new false_courses class 
bool has_particular_fch( const row &fch, const method &m )
{
  const falseness_table ft( m );
  const row lh( m.lh() );
  
  row r1; 
  do 
    {
      r1 *= lh;
      row r2;
      do
	{
	  r2 *= lh;
	  if ( find( ft.begin(), ft.end(), r1 * fch * r2 ) != ft.end() )
	    return true;
	}
      while ( !r2.isrounds() );
    }
  while ( !r1.isrounds() );
  
  return false;
}

bool is_cps( const method &m )
{
  const false_courses fchs( m, false_courses::in_course_only 
			    |  false_courses::tenors_together );
  
  return fchs.size() == 1;
}

