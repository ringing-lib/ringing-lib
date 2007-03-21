// -*- C++ -*- row-tests.cc - Tests for the row class
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

#include <ringing/place_notation.h>
#include <ringing/row.h>
#include <ringing/streamutils.h>
#include "test-base.h"

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

// ---------------------------------------------------------------------
// Tests for class bell

void test_bell(void)
{
  RINGING_TEST( bell() == 0 );
  RINGING_TEST( bell(4) == 4 );
  RINGING_TEST( bell(bell::MAX_BELLS) == int(bell::MAX_BELLS) );
}

void test_bell_from_char(void)
{
  bell b; 

  b.from_char('5'); RINGING_TEST( b == 4 );
  b.from_char('0'); RINGING_TEST( b == 9 );
  b.from_char('T'); RINGING_TEST( b == 11 );

  RINGING_TEST_THROWS( b.from_char('%'), bell::invalid );
}

void test_bell_to_char(void)
{
  RINGING_TEST( bell(5).to_char() == '6' );
  RINGING_TEST( bell(10).to_char() == 'E' );
  RINGING_TEST( bell(14).to_char() == 'C' );
  RINGING_TEST( bell(bell::MAX_BELLS).to_char() == '*' );
}

void test_bell_output(void)
{
  string s = make_string() << bell(5) << bell(12) << bell(bell::MAX_BELLS); 
  RINGING_TEST( s == "6A*" );
}

// ---------------------------------------------------------------------
// Tests for class change

void test_change_equals(void)
{
  RINGING_TEST( change(       ) == change(0, ""     ) );
  RINGING_TEST( change(3      ) == change(3, "123"  ) );
  RINGING_TEST( change(4      ) == change(4, "1234" ) );
  RINGING_TEST( change(5, "3" ) == change(5, "3"    ) );
  RINGING_TEST( change(6, "1" ) != change(6, "5"    ) );
  RINGING_TEST( change(8, "X" ) == change(8, "-"    ) );
  RINGING_TEST( change(6, "-" ) != change(8, "-"    ) );
  RINGING_TEST( change(6, "-" ) != change(8, "78"   ) );
}

void test_change_copy(void)
{
  change a(6, "12"), c(a);
  RINGING_TEST( a == c );

  change b(8, "14"); b.swap(a);
  RINGING_TEST( b == c );
  RINGING_TEST( a != c );

  a = b;
  RINGING_TEST( a == c );
  RINGING_TEST( b == c );
}

void test_change_invalid(void)
{
  RINGING_TEST_THROWS( change(5, "18"),   change::invalid );
  RINGING_TEST_THROWS( change(5, "XYZ"),  bell::invalid );
  RINGING_TEST_THROWS( change(5, "!$"),   bell::invalid );
  RINGING_TEST_THROWS( change(5, ""),     change::invalid );
  RINGING_TEST_THROWS( change(5, "321"),  change::invalid );
  RINGING_TEST_THROWS( change(5, "11"),   change::invalid );
  RINGING_TEST_THROWS( change(5, "12 3"), bell::invalid );
}

void test_change_implicit_places(void)
{
  RINGING_TEST( change(6, "4" ) == change(6, "14" ) );
  RINGING_TEST( change(5, "-" ) == change(5, "5"  ) );
  RINGING_TEST( change(7, "12") == change(7, "127") );

  RINGING_TEST_THROWS( change(5, "15"),  change::invalid );
}

void test_change_set(void)
{
  change a; 
  RINGING_TEST( a != change( 6, "16" ) );
  
  a.set( 6, "16" );
  RINGING_TEST( a == change( 6, "16" ) );
}

void test_change_reverse(void)
{
  RINGING_TEST( change(5, "1" ).reverse() == change(5, "5" ) );
  RINGING_TEST( change(6, "16").reverse() == change(6, "16") );
  RINGING_TEST( change(8, "X" ).reverse() == change(8, "X" ) );
  RINGING_TEST( change(4, "12").reverse() == change(4, "34") );
}

void test_change_print(void)
{
  RINGING_TEST( change(           ).print() == ""     );
  RINGING_TEST( change( 8,  "-"   ).print() == "X"    );
  RINGING_TEST( change( 8,  "18"  ).print() == "18"   );
  RINGING_TEST( change( 10, "18"  ).print() == "18"   );
  RINGING_TEST( change( 10, "4"   ).print() == "14"   );
  RINGING_TEST( change( 10, "129" ).print() == "1290" );
  RINGING_TEST( change( 16, "E"   ).print() == "ED"   );
}

void test_change_bells(void)
{
  RINGING_TEST( change(       ).bells() == 0 );
  RINGING_TEST( change(7      ).bells() == 7 );
  RINGING_TEST( change(8, "14").bells() == 8 );
}

void test_change_sign(void)
{
  RINGING_TEST( change(        ).sign() == +1 );
  RINGING_TEST( change(4       ).sign() == +1 );
  RINGING_TEST( change(9       ).sign() == +1 );  
  RINGING_TEST( change(9, "1"  ).sign() == +1 );
  RINGING_TEST( change(9, "9"  ).sign() == +1 );
  RINGING_TEST( change(9, "129").sign() == -1 );
  RINGING_TEST( change(6, "12" ).sign() == +1 );
  RINGING_TEST( change(6, "-"  ).sign() == -1 );
  RINGING_TEST( change(8, "-"  ).sign() == +1 );
}

void test_change_findswap(void)
{
  const change c(12, "145T");
  RINGING_TEST( ! c.findswap(0)  ); // 1-2
  RINGING_TEST(   c.findswap(1)  ); // 2-3
  RINGING_TEST( ! c.findswap(2)  ); // 3-4
  RINGING_TEST( ! c.findswap(3)  ); // 4-5
  RINGING_TEST(   c.findswap(5)  ); // 6-7
  RINGING_TEST( ! c.findswap(10) ); // E-T

  // ???
  //RINGING_TEST_THROWS( c.findswap(11), change::out_of_range );
}

void test_change_findplace(void)
{
  const change c(10, "1230");
  RINGING_TEST(   c.findplace(0) );
  RINGING_TEST(   c.findplace(1) );
  RINGING_TEST(   c.findplace(2) );
  RINGING_TEST( ! c.findplace(3) );
  RINGING_TEST( ! c.findplace(8) );
  RINGING_TEST(   c.findplace(9) );

  // ???
  //RINGING_TEST_THROWS( c.findplace(10), change::out_of_range );
}

void test_change_swappair(void)
{
  change c( 8, "-" );

  RINGING_TEST( ! c.swappair(0) );  RINGING_TEST( c.print() == "12"   );
  RINGING_TEST(   c.swappair(1) );  RINGING_TEST( c.print() == "14"   ); // ???
  RINGING_TEST(   c.swappair(0) );  RINGING_TEST( c.print() == "34"   ); // ???
  RINGING_TEST(   c.swappair(2) );  RINGING_TEST( c.print() == "X"    );

  RINGING_TEST( ! c.swappair(2) );  RINGING_TEST( c.print() == "34"   );
  RINGING_TEST( ! c.swappair(0) );  RINGING_TEST( c.print() == "1234" );
  RINGING_TEST(   c.swappair(1) );  RINGING_TEST( c.print() == "14"   );
  RINGING_TEST( ! c.swappair(6) );  RINGING_TEST( c.print() == "1478" );

  RINGING_TEST_THROWS( c.swappair(11), change::out_of_range );
  RINGING_TEST_THROWS( c.swappair(7),  change::out_of_range );
}

void test_change_internal(void)
{
  RINGING_TEST( ! change(           ).internal() );
  RINGING_TEST( ! change( 1         ).internal() );
  RINGING_TEST( ! change( 2         ).internal() );
  RINGING_TEST(   change( 3         ).internal() );
  RINGING_TEST( ! change( 5, "1"    ).internal() );
  RINGING_TEST(   change( 5, "125"  ).internal() );
  RINGING_TEST( ! change( 6, "-"    ).internal() );
  RINGING_TEST( ! change( 6, "16"   ).internal() );
  RINGING_TEST(   change( 6, "1256" ).internal() );
  RINGING_TEST(   change( 4, "1234" ).internal() );
  RINGING_TEST(   change( 8, "4"    ).internal() );
  RINGING_TEST(   change( 8, "3"    ).internal() );
  RINGING_TEST( ! change( 8, "1"    ).internal() );
}

void test_change_count_places(void)
{
  RINGING_TEST( change( 8       ).count_places() == 8 );
  RINGING_TEST( change( 8, "3"  ).count_places() == 2 );
  RINGING_TEST( change( 8, "6"  ).count_places() == 2 );
  RINGING_TEST( change( 8, "36" ).count_places() == 2 );
  RINGING_TEST( change( 8, "18" ).count_places() == 2 );
  RINGING_TEST( change( 8, "78" ).count_places() == 2 );
  RINGING_TEST( change( 9, "7"  ).count_places() == 1 );
  RINGING_TEST( change( 9, "1"  ).count_places() == 1 );
  RINGING_TEST( change( 9, "9"  ).count_places() == 1 );
}

void test_change_comparison(void)
{
  RINGING_TEST(    change( 6, "1234" ) <  change( 8, "56" )   );
  RINGING_TEST(    change( 6, "1234" ) <= change( 8, "56" )   );
  RINGING_TEST( !( change( 6, "1234" ) >  change( 8, "56" ) ) );
  RINGING_TEST( !( change( 6, "1234" ) >= change( 8, "56" ) ) );

  RINGING_TEST(    change( 6, "1234" ) >  change( 6, "56" )   );
  RINGING_TEST(    change( 6, "1234" ) >= change( 6, "56" )   );
  RINGING_TEST( !( change( 6, "1234" ) <  change( 6, "56" ) ) );
  RINGING_TEST( !( change( 6, "1234" ) <= change( 6, "56" ) ) );

  RINGING_TEST( !( change( 6, "X" ) >  change( 6, "X" ) ) );
  RINGING_TEST(    change( 6, "X" ) >= change( 6, "X" )   );
  RINGING_TEST( !( change( 6, "X" ) <  change( 6, "X" ) ) );
  RINGING_TEST(    change( 6, "X" ) <= change( 6, "X" )   );

  // ??? 
  RINGING_TEST( change( 6         ) < change( 6, "3456" ) );
  RINGING_TEST( change( 6, "3456" ) < change( 6, "56"   ) );
  RINGING_TEST( change( 6, "56"   ) < change( 6, "36"   ) );
  RINGING_TEST( change( 6, "36"   ) < change( 6, "16"   ) );
}

void test_change_output(void)
{
  string s = make_string() << change( 6, "-" ) << '.' 
			   << change( 6, "3" ) << '.' 
			   << change( 6, "-" ) << '.' 
			   << change( 6, "4" );

  RINGING_TEST( s == "X.36.X.14" );
}

void test_change_many_bells(void)
{
  const int n( bell::MAX_BELLS + 4 );

  change c( n );

  RINGING_TEST( ! c.findswap( n-2 ) );
  RINGING_TEST(   c.swappair( n-2 ) );

  RINGING_TEST( ! c.findplace( n-1 ) );
  RINGING_TEST( ! c.findplace( n-2 ) );
  RINGING_TEST(   c.findplace( n-3 ) );

  RINGING_TEST(   c.findswap( n-2 ) );
  RINGING_TEST( ! c.findswap( n-3 ) );
  RINGING_TEST( ! c.findswap( n-4 ) );

  RINGING_TEST(   c.count_places() == n-2 );
  RINGING_TEST(   c.internal() );

  RINGING_TEST( ! c.reverse().findplace(0) );  
  RINGING_TEST(   c.reverse().findplace(3) );
  RINGING_TEST(   c.reverse().internal() );
}

void test_change_multiply_bell(void)
{
  bell b1( 3 );

  RINGING_TEST( (b1 *= change( 6, "-" )) == 2 );
  RINGING_TEST( (b1 *= change( 6, "3" )) == 2 );
  RINGING_TEST( (b1 *= change( 6, "-" )) == 3 );
  RINGING_TEST( (b1 *= change( 6, "4" )) == 3 );
  RINGING_TEST( (b1 *= change( 6, "-" )) == 2 );

  bell b2( 0 );

  RINGING_TEST( (b2 *= change( 6, "-" )) == 1 );
  RINGING_TEST( (b2 *= change( 6, "3" )) == 0 );
  RINGING_TEST( (b2 *= change( 6, "-" )) == 1 );
  RINGING_TEST( (b2 *= change( 6, "4" )) == 2 );
  RINGING_TEST( (b2 *= change( 6, "-" )) == 3 );

  RINGING_TEST( b2 * change( 6, "16" ) == 4 );
  RINGING_TEST( b2 == 3 );

  RINGING_TEST( bell(10) * change( 4, "X" ) == 10 );
  RINGING_TEST( bell(9) * change() == 9 );
}

// ---------------------------------------------------------------------
// Tests for the interpret_pn function

string get_pn_string( const vector<change> &ch )
{
  make_string ms;
  for ( vector<change>::const_iterator i( ch.begin() ), e( ch.end() );
	i != e;  ++i )
    ms << *i << ".";
  return ms;
}

void test_interpret_pn(void)
{
  vector<change> ch;

  {
    string pn( "-3-4" );
    interpret_pn( 6, pn.begin(), pn.end(), back_inserter(ch) );
    
    RINGING_TEST( ch.size() == 4 );
    RINGING_TEST( get_pn_string(ch) == "X.36.X.14." );
  }

  ch.clear();
    
  {
    string pn( "XX--.-.X" );
    interpret_pn( 6, pn.begin(), pn.end(), back_inserter(ch) );
    
    RINGING_TEST( ch.size() == 6 );
    RINGING_TEST( get_pn_string(ch) == "X.X.X.X.X.X." );
  }

  ch.clear();
    
  {
    string pn( "&3-36.4,2" );
    interpret_pn( 6, pn.begin(), pn.end(), back_inserter(ch) );

    RINGING_TEST( ch.size() == 8 );
    RINGING_TEST( get_pn_string(ch) == "36.X.36.14.36.X.36.12." );
  }

  ch.clear();
  
  {
    string pn( "3 , & 1 . 5 . 1 . 5 . 1" );
    interpret_pn( 5, pn.begin(), pn.end(), back_inserter(ch) );

    RINGING_TEST( ch.size() == 10 );
    RINGING_TEST( get_pn_string(ch) == "3.1.5.1.5.1.5.1.5.1." );
  }
}

void test_interpret_pn_exceptions(void)
{
  vector<change> ch;

  {
    string pn( "-41-4" );
    RINGING_TEST_THROWS
      ( interpret_pn( 6, pn.begin(), pn.end(), back_inserter(ch) ),
	change::invalid );
  }

  {
    string pn( "-18-4" );
    RINGING_TEST_THROWS
      ( interpret_pn( 6, pn.begin(), pn.end(), back_inserter(ch) ),
	change::invalid );
  }

  {
    string pn( "-12..-4" );
    RINGING_TEST_THROWS
      ( interpret_pn( 6, pn.begin(), pn.end(), back_inserter(ch) ),
	place_notation::invalid );
  }

  {
    string pn( "-!12'[]-4#" );
    RINGING_TEST_THROWS
      ( interpret_pn( 6, pn.begin(), pn.end(), back_inserter(ch) ),
	place_notation::invalid );
  }
}

// ---------------------------------------------------------------------
// Register the tests

RINGING_END_ANON_NAMESPACE

RINGING_START_TEST_FILE( change )

  // Tests for the bell class
  RINGING_REGISTER_TEST( test_bell )
  RINGING_REGISTER_TEST( test_bell_from_char )
  RINGING_REGISTER_TEST( test_bell_to_char )
  RINGING_REGISTER_TEST( test_bell_output )

  // Tests for the change class
  RINGING_REGISTER_TEST( test_change_equals )
  RINGING_REGISTER_TEST( test_change_copy )
  RINGING_REGISTER_TEST( test_change_invalid )
  RINGING_REGISTER_TEST( test_change_implicit_places )
  RINGING_REGISTER_TEST( test_change_set )
  RINGING_REGISTER_TEST( test_change_reverse )
  RINGING_REGISTER_TEST( test_change_print )
  RINGING_REGISTER_TEST( test_change_bells )
  RINGING_REGISTER_TEST( test_change_sign )
  RINGING_REGISTER_TEST( test_change_findswap )
  RINGING_REGISTER_TEST( test_change_findplace )
  RINGING_REGISTER_TEST( test_change_swappair )
  RINGING_REGISTER_TEST( test_change_internal )
  RINGING_REGISTER_TEST( test_change_count_places )
  RINGING_REGISTER_TEST( test_change_comparison )
  RINGING_REGISTER_TEST( test_change_output )
  RINGING_REGISTER_TEST( test_change_many_bells )
  RINGING_REGISTER_TEST( test_change_multiply_bell )

  // Tests for the interpret_pn function
  RINGING_REGISTER_TEST( test_interpret_pn )
  RINGING_REGISTER_TEST( test_interpret_pn_exceptions )

RINGING_END_TEST_FILE

RINGING_END_NAMESPACE
