// -*- C++ -*- method-tests.cc - Tests for the method class
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

#include <ringing/method.h>
#include "test-base.h"


RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

// ---------------------------------------------------------------------
// Tests for class method

void test_method_equals(void)
{
  RINGING_TEST( method()              == method( 0, 6, "name" ) );
  RINGING_TEST( method()              == method( "", 9 )        );
  RINGING_TEST( method("-", 6)        != method("-", 4)         );
  RINGING_TEST( method( "&-12,1", 8 ) == method( "+ -12-18", 8 ) );
}

void test_method_name(void)
{
  RINGING_TEST( method( "&-12,1", 8 ).name()           == string("Untitled") );
  RINGING_TEST( method( "&-12,1", 8, "Bastow" ).name() == string("Bastow"  ) );

  {
    method m( "&-12,1", 8 );
    m.name( "Bastow" );
    RINGING_TEST( m.name() == string("Bastow") );
  }
  {
    method m( "&-12,1", 8 );
    m.name( string("Bastow") );
    RINGING_TEST( m.name() == string("Bastow") );
  }
} 

void test_method_fullname(void)
{
  RINGING_TEST( method( "&-12,1", 8, 
			"Bastow" ).fullname() 
		== "Bastow Little Bob Major" );

  RINGING_TEST( method( "&34.1.5.1.5,2", 5, 
			"Reverse Canterbury Pleasure" ).fullname()
		== "Reverse Canterbury Pleasure Place Doubles" );

  RINGING_TEST( method( "3.1", 3, 
			"Original" ).fullname()
		== "Original Singles" );

  RINGING_TEST( method( "&-5-4.5-5.36.4-4.5-4-1,8", 8, 
			"Bristol" ).fullname()
		== "Bristol Surprise Major" );

  RINGING_TEST( method( "-", 8, 
			"Cross" ).fullname()
		== "Cross Differential Major" );

  RINGING_TEST( method( "-4-6-6-4-6-6-2", 6, 
			"Tetley's Smoothflow" ).fullname()
		== "Tetley's Smoothflow Differential Hybrid Minor" );
}

void test_method_fullname_grandsire(void)
{
  RINGING_TEST( method( "3,&1-1-1-", 6,
			"Grandsire" ).fullname()
		== "Grandsire Minor" );

  RINGING_TEST( method( "6,&1-1-1.4", 6,
			"Reverse Grandsire" ).fullname()
		== "Reverse Grandsire Minor" );

  RINGING_TEST( method( "3,&1-1-1.4", 6,
			"Double Grandsire" ).fullname()
		== "Double Grandsire Minor" );

  RINGING_TEST( method( "+3,&1.9.1.5.1", 9,
			"Little Grandsire" ).fullname()
		== "Little Grandsire Caters" );

  RINGING_TEST( method( "+3.1.7.1.7.1.7.1.7.1.7.1.5.1", 7,
			"Union" ).fullname()
		== "Union Triples" );
}

void test_method_stagename(void)
{
  RINGING_TEST( method::stagename(0 ) == string("0") ); // ???
  RINGING_TEST( method::stagename(1 ) == string("1") );
  RINGING_TEST( method::stagename(2 ) == string("2") );

  RINGING_TEST( method::stagename(3 ) == string("Singles"   ) );
  RINGING_TEST( method::stagename(4 ) == string("Minimus"   ) );
  RINGING_TEST( method::stagename(5 ) == string("Doubles"   ) );
  RINGING_TEST( method::stagename(6 ) == string("Minor"     ) );
  RINGING_TEST( method::stagename(7 ) == string("Triples"   ) );
  RINGING_TEST( method::stagename(8 ) == string("Major"     ) );
  RINGING_TEST( method::stagename(9 ) == string("Caters"    ) );
  RINGING_TEST( method::stagename(10) == string("Royal"     ) );
  RINGING_TEST( method::stagename(11) == string("Cinques"   ) );
  RINGING_TEST( method::stagename(12) == string("Maximus"   ) );
  RINGING_TEST( method::stagename(13) == string("Sextuples" ) );
  RINGING_TEST( method::stagename(14) == string("Fourteen"  ) );
  RINGING_TEST( method::stagename(15) == string("Septuples" ) );
  RINGING_TEST( method::stagename(16) == string("Sixteen"   ) );
  RINGING_TEST( method::stagename(17) == string("Octuples"  ) );
  RINGING_TEST( method::stagename(18) == string("Eighteen"  ) );
  RINGING_TEST( method::stagename(19) == string("Nonuples"  ) );
  RINGING_TEST( method::stagename(20) == string("Twenty"    ) );
  RINGING_TEST( method::stagename(21) == string("Decuples"  ) );
  RINGING_TEST( method::stagename(22) == string("Twenty-two") );

  RINGING_TEST( method::stagename(54) == string("54") );
}

void test_method_classname(void)
{
  RINGING_TEST( method::classname( method::M_UNKNOWN      ) 
		== string() ); // ??? (or exception?)
  RINGING_TEST( method::classname( method::M_PRINCIPLE    )
		== string() ); // ???
  RINGING_TEST( method::classname( method::M_BOB          ) 
		== string("Bob") );
  RINGING_TEST( method::classname( method::M_PLACE        ) 
		== string("Place") );
  RINGING_TEST( method::classname( method::M_TREBLE_BOB   ) 
		== string("Treble Bob") );
  RINGING_TEST( method::classname( method::M_SURPRISE     ) 
		== string("Surprise") );
  RINGING_TEST( method::classname( method::M_DELIGHT      ) 
		== string("Delight") );
  RINGING_TEST( method::classname( method::M_TREBLE_PLACE ) 
		== string("Treble Place") );
  RINGING_TEST( method::classname( method::M_ALLIANCE     ) 
		== string("Alliance") );
  RINGING_TEST( method::classname( method::M_HYBRID       )
		== string("Hybrid") );
  RINGING_TEST( method::classname( method::M_SLOW_COURSE  )
		== string("Slow Course") );
  RINGING_TEST( method::classname( method::M_DIFFERENTIAL )
		== string("Differential") );

  RINGING_TEST( method::classname( method::M_BOB          |
				   method::M_LITTLE       )
		== string("Little Bob") );
  RINGING_TEST( method::classname( method::M_HYBRID       | 
				   method::M_LITTLE       )
		== string("Little Hybrid") );
  RINGING_TEST( method::classname( method::M_SURPRISE     | 
				   method::M_DIFFERENTIAL )
		== string("Differential Surprise") );
  RINGING_TEST( method::classname( method::M_TREBLE_PLACE |
				   method::M_LITTLE       | 
				   method::M_DIFFERENTIAL )
		== string("Differential Little Treble Place") );
}

void test_method_length(void)
{
  RINGING_TEST( method( "&-12,16", 6 ).length() == 4 );
  RINGING_TEST( method( "&-12,16", 6 ).size()   == 4 );
  RINGING_TEST( method(              ).size()   == 0 );
}

void test_method_bells(void)
{
  RINGING_TEST( method( "",          6 ).bells() == 6 );
  RINGING_TEST( method( "&-1-1-1,2", 6 ).bells() == 6 );
}

void test_method_lh(void)
{
  RINGING_TEST( method(                      ).lh() == ""         );
  RINGING_TEST( method( "&-1-1-1-1,2",     8 ).lh() == "13527486" );
  RINGING_TEST( method( "&-1-1-1-1,1",     8 ).lh() == "12345678" );
  RINGING_TEST( method( "&-3-4-2-3-4-5,2", 6 ).lh() == "156342"   );
  RINGING_TEST( method( "+5.3.1.3.1.3",    5 ).lh() == "24153"    );
}

void test_method_issym(void)
{
  RINGING_TEST(   method(                            ).issym() );
  RINGING_TEST(   method( "&-1-1-1,2",             6 ).issym() );
  RINGING_TEST(   method( "&-1-1-1,1",             6 ).issym() );
  RINGING_TEST( ! method( "3,&1-1-1-",             6 ).issym() ); // ???
  RINGING_TEST( ! method( "3.1.5.1.5.1.5.1.5.123", 5 ).issym() );
  RINGING_TEST(   method( "&-4-36-5-1,8",          8 ).issym() );
  RINGING_TEST( ! method( "+5.3.1.3.1.3",          5 ).issym() ); // ???
}

// TODO  -- Write tests for isdouble, isregular, huntbells, leads, methclass,
// lhcode and the bell overloads of issym, isplain, hasdodges and hasplaces.

// ---------------------------------------------------------------------
// Register the tests

RINGING_END_ANON_NAMESPACE

RINGING_START_TEST_FILE( method )

  // Tests for the method class
  RINGING_REGISTER_TEST( test_method_equals )
  RINGING_REGISTER_TEST( test_method_name )
  RINGING_REGISTER_TEST( test_method_fullname )
  RINGING_REGISTER_TEST( test_method_fullname_grandsire )
  RINGING_REGISTER_TEST( test_method_stagename )
  RINGING_REGISTER_TEST( test_method_classname )
  RINGING_REGISTER_TEST( test_method_length )
  RINGING_REGISTER_TEST( test_method_bells )
  RINGING_REGISTER_TEST( test_method_lh )
  RINGING_REGISTER_TEST( test_method_issym )

RINGING_END_TEST_FILE

RINGING_END_NAMESPACE
