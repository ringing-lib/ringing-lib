// -*- C++ -*- multtab.cpp - A precomputed multiplication table of rows
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
#include <ringing/multtab.h>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#else
#include <iostream>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

void multiplication_table::dump( ostream &os ) const
{
  for ( row_t i; i.n != table.size(); ++i.n )
    {
      os << i.n << ")  ";
	for ( vector< row_t >::const_iterator j( table[i.n].x.begin() );
	      j != table[i.n].x.end(); ++j )
	  {
	    os << j->n << " ";
	  }
	os << "\n";
    }

  os << endl;
}

multiplication_table::pre_col_t 
multiplication_table::compute_pre_mult( const row &r )
{
  for ( vector< table_row >::iterator i( table.begin() ); 
	i != table.end(); ++i )
    {
      i->x.push_back( find( r * i->r ) );
    }
  
  return pre_col_t( colcount++, this );
}

multiplication_table::post_col_t 
multiplication_table::compute_post_mult( const row &r )
{
  for ( vector< table_row >::iterator i( table.begin() ); 
	i != table.end(); ++i )
    {
      i->x.push_back( find( i->r * r ) );
    }
  
  return post_col_t( colcount++, this );
}

multiplication_table::row_t 
multiplication_table::find( const row &r )
{
  for ( row_t i; i.n != table.size(); ++i.n )
    if ( table[i.n].r == r ) 
      return i;
}

RINGING_END_NAMESPACE
