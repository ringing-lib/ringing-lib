// -*- C++ -*- extent-tests.cc - Tests for the extent iterators, etc
// Copyright (C) 2007 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/extent.h>
#include <ringing/mathutils.h>
#include "test-base.h"

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

void test_extent_length(void)
{
  for ( unsigned nh=0; nh<3; ++nh )
    for ( unsigned nw=0; nw<6; ++nw ) 
    {
      extent_iterator i(nw, nh), e;
      RINGING_TEST( i->bells() == nh+nw );
      RINGING_TEST( *i == row::rounds(nh+nw) );
      RINGING_TEST( distance( i, e ) == factorial(nw) );

      for ( unsigned nt=nh+nw; nt<nh+nw+4; ++nt )
      {
        i = extent_iterator(nw, nh, nt);
        RINGING_TEST( i->bells() == nt );
        RINGING_TEST( *i == row::rounds(nt) );
        RINGING_TEST( distance( i, e ) == factorial(nw) );
      }
    }
}

void test_extent_fixed_bells(void)
{
  for ( unsigned nh=0; nh<3; ++nh )
    for ( unsigned nw=0; nw<6; ++nw )
      for ( unsigned nt=nh+nw; nt<nh+nw+4; ++nt )
      {
        extent_iterator i(nw, nh, nt), e; 
        while (i != e) 
        {
          for ( unsigned n=0; n<nh; ++n ) 
            RINGING_TEST( (*i)[n] == n );
          for ( unsigned n=nh+nw; n<nt; ++n )
            RINGING_TEST( (*i)[n] == n );

          extent_iterator j = i++;
          RINGING_TEST( j < i );
        }  
      }
}

void test_extent_index(void)
{
  for ( unsigned nh=0; nh<3; ++nh )
    for ( unsigned nw=0; nw<6; ++nw )
      for ( unsigned nt=nh+nw; nt<nh+nw+4; ++nt )
      {
        extent ex(nw, nh, nt);
        for ( extent::const_iterator i(ex.begin()), e(ex.end()); i!=e; ++i ) {
          RINGING_TEST( ex[ex[*i]] == *i );
          RINGING_TEST( sign_of_nth_row_of_extent(ex[*i]) == i->sign() );
        }
      }
}


RINGING_END_ANON_NAMESPACE
  
RINGING_START_TEST_FILE( extent )

  RINGING_REGISTER_TEST( test_extent_length )
  RINGING_REGISTER_TEST( test_extent_fixed_bells )
  RINGING_REGISTER_TEST( test_extent_index )

RINGING_END_TEST_FILE

RINGING_END_NAMESPACE

