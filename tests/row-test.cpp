// -*- C++ -*- row-tests.cc - Tests for the bell and change classes 
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

#include <ringing/row.h>
#include <ringing/streamutils.h>
#include "test-base.h"

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

// ---------------------------------------------------------------------
// Tests for utility functions in row.h

void test_hcf(void)
{
  RINGING_TEST( hcf( 1, 1 ) == 1 );
  RINGING_TEST( hcf( 1, 9 ) == 1 );
  RINGING_TEST( hcf( 5, 3 ) == 1 );
  RINGING_TEST( hcf( 8, 12 ) == 4 );
  RINGING_TEST( hcf( 15, 12 ) == 3 );
}

void test_lcm(void)
{
  RINGING_TEST( lcm( 1, 1 ) == 1 );
  RINGING_TEST( lcm( 8, 8 ) == 8 );
  RINGING_TEST( lcm( 3, 4 ) == 12 );
  RINGING_TEST( lcm( 1, 5 ) == 5 );
  RINGING_TEST( lcm( 6, 4 ) == 12 );
}

// ---------------------------------------------------------------------
// Tests for class row

void test_row_equals(void)
{  
  RINGING_TEST(    row(          ) == row( ""         )   );
  RINGING_TEST(    row(          ) != row( "1"        )   );
  RINGING_TEST(    row( 6        ) == row( "123456"   )   );

  RINGING_TEST(    row( "123456" ) == row( "123456"   )   );
  RINGING_TEST( !( row( "123456" ) != row( "123456"   ) ) );

  RINGING_TEST(    row( "123456" ) != row( "123465"   )   );
  RINGING_TEST( !( row( "123456" ) == row( "123465"   ) ) );

  RINGING_TEST(    row( "123456" ) != row( "1234"     )   );
  RINGING_TEST( !( row( "123456" ) == row( "1234"     ) ) );

  RINGING_TEST(    row( "123456" ) != row( "12345678" )   );
  RINGING_TEST( !( row( "123456" ) == row( "12345678" ) ) );
}

void test_row_copy(void)
{
  row a( "123654" ), c(a);
  RINGING_TEST( a == c );

  row b( "12753468" ); b.swap(a);
  RINGING_TEST( b == c );
  RINGING_TEST( a != c );

  a = b;
  RINGING_TEST( a == c );
  RINGING_TEST( b == c );
}

void test_row_invalid(void)
{
  RINGING_TEST_THROWS( row( "124"       ), row::invalid  );
  RINGING_TEST_THROWS( row( "12#"       ), bell::invalid );
  RINGING_TEST_THROWS( row( "5444"      ), row::invalid  );
  RINGING_TEST_THROWS( row( "098765432" ), row::invalid  );
}

void test_row_subscript(void)
{
  RINGING_TEST( row( "1"     )[ 0] ==  0 );
  RINGING_TEST( row( "15234" )[ 0] ==  0 );
  RINGING_TEST( row( "15234" )[ 2] ==  1 );
  RINGING_TEST( row( "15234" )[ 4] ==  3 );
  RINGING_TEST( row( 12      )[11] == 11 );
}

void test_row_multiply_row(void)
{
  RINGING_TEST( row( "4312" ) *
		row( "2341" ) ==
		row( "3124" ) );

  RINGING_TEST( row( "7631425" ) * 
		row( "2347165" ) == 
		row( "6315724" ) );

  RINGING_TEST( row( "12387654" ) * 
		row( "631245"   ) ==
		row( "63128754" ) );

  RINGING_TEST( row( "24531"    ) *
		row( "57863124" ) ==
		row( "17865243" ) );
  
  row r;
  RINGING_TEST( ( r *= "34512" ) == "34512" );
  RINGING_TEST( ( r *= "14253" ) == "31425" );
}

void test_row_divide_row(void)
{
  RINGING_TEST( row( "642153" ) / 
		row( "235164" ) == 
		row( "164325" ) );

  RINGING_TEST( row( "53678421" ) /
		row( "425613"   ) ==
		row( "83456721" ) );

  RINGING_TEST( row( "51324"  ) /
		row( "645231" ) ==
		row( "624135" ) );
  
  row r;
  RINGING_TEST( ( r /= "623415"   ) == "523461"   );
  RINGING_TEST( ( r /= "623415"   ) == "623415"   );
  RINGING_TEST( ( r /= "87654321" ) == "87514326" );

  RINGING_TEST( r.bells() == 8 );
}

void test_row_multiply_change(void)
{
  row r;
  RINGING_TEST( ( r *= change( 6, "X" ) ) == "214365" );
  RINGING_TEST( ( r *= change( 6, "1" ) ) == "241635" );
  RINGING_TEST( ( r *= change( 8, "X" ) ) == "42615387" );
  RINGING_TEST( ( r *= change( 5, "3" ) ) == "24651387" );

  RINGING_TEST( row( "214365" ) * change( 7, "5" ) == row( "1234675" ) );
}

void test_row_inverse(void)
{
  RINGING_TEST( row( "654321"   ).inverse() == "654321"   );
  RINGING_TEST( row( "312"      ).inverse() == "231"      );  
  RINGING_TEST( row( "18234567" ).inverse() == "13456782" );
}

void test_row_print(void)
{
  RINGING_TEST( row(          ).print() == ""       );
  RINGING_TEST( row( "123456" ).print() == "123456" );

  string s = make_string() << row("1536472");
  RINGING_TEST( s == "1536472" );
}

void test_row_bells(void)
{
  RINGING_TEST( row(       ).bells() == 0 );
  RINGING_TEST( row(7      ).bells() == 7 );
  RINGING_TEST( row("12345").bells() == 5 );
}

void test_row_rounds(void)
{
  RINGING_TEST( row(            ).rounds() == ""      );
  RINGING_TEST( row( "54321"    ).rounds() == "12345" );
  RINGING_TEST( row( "84567123" ).rounds() == row(8)  );
}

void test_row_named_rows(void)
{
  RINGING_TEST( row::rounds(0) == "" );
  RINGING_TEST( row::rounds(1) == "1" );
  RINGING_TEST( row::rounds(2) == "12" );
  RINGING_TEST( row::rounds(5) == "12345" );
  RINGING_TEST( row::rounds(8) == "12345678" );

  RINGING_TEST( row::queens(0) == "" );
  RINGING_TEST( row::queens(1) == "1" );
  RINGING_TEST( row::queens(2) == "12" );
  RINGING_TEST( row::queens(5) == "13524" ); // "24135" ???
  RINGING_TEST( row::queens(8) == "13572468" );

  RINGING_TEST( row::kings(0) == "" );
  RINGING_TEST( row::kings(1) == "1" );
  RINGING_TEST( row::kings(2) == "12" );
  RINGING_TEST( row::kings(5) == "53124" ); // "42135" ???
  RINGING_TEST( row::kings(8) == "75312468" );

  RINGING_TEST( row::tittums(0) == "" );  
  RINGING_TEST( row::tittums(1) == "1" );
  RINGING_TEST( row::tittums(2) == "12" );
  RINGING_TEST( row::tittums(5) == "14253" ); // 31425 ???
  RINGING_TEST( row::tittums(8) == "15263748" );

  RINGING_TEST( row::reverse_rounds(0) == "" );
  RINGING_TEST( row::reverse_rounds(1) == "1" );
  RINGING_TEST( row::reverse_rounds(2) == "21" );
  RINGING_TEST( row::reverse_rounds(5) == "54321" );
  RINGING_TEST( row::reverse_rounds(8) == "87654321" );
}

void test_row_pblh(void)
{
  RINGING_TEST( row::pblh(0) == row() );
  RINGING_TEST( row::pblh(1) == row("1") );
  RINGING_TEST( row::pblh(2) == row("12") );
  RINGING_TEST( row::pblh(3) == row("132") );
  RINGING_TEST( row::pblh(5) == row("13524") );
  RINGING_TEST( row::pblh(6) == row("135264") );

  RINGING_TEST( row::pblh(3,10) == row("123") ); // or error ???
  RINGING_TEST( row::pblh(3,2)  == row("123") );
  RINGING_TEST( row::pblh(3,2)  == row("123") );
  RINGING_TEST( row::pblh(6,2)  == row("124635") );
  RINGING_TEST( row::pblh(7,2)  == row("1246375") );
  RINGING_TEST( row::pblh(9,5)  == row("123457968") );
}

void test_row_cyclic(void)
{
  RINGING_TEST( row::cyclic(0) == row() );
  RINGING_TEST( row::cyclic(1) == row("1") );
  RINGING_TEST( row::cyclic(2) == row("12") );
  RINGING_TEST( row::cyclic(3) == row("132") );
  RINGING_TEST( row::cyclic(5) == row("13452") );
  RINGING_TEST( row::cyclic(8) == row("13456782") );

  RINGING_TEST( row::cyclic(3,0) == row("231") );
  RINGING_TEST( row::cyclic(3,2) == row("123") );
  RINGING_TEST( row::cyclic(3,3) == row("123") );
  RINGING_TEST( row::cyclic(3,9) == row("123") ); // or error ???
  RINGING_TEST( row::cyclic(8,0) == row("23456781") );
  RINGING_TEST( row::cyclic(8,2) == row("12456783") );
  RINGING_TEST( row::cyclic(9,2) == row("124567893") );

  RINGING_TEST( row::cyclic(8,1,-1) == row("18234567") );
  RINGING_TEST( row::cyclic(8,1,0)  == row("12345678") );
  RINGING_TEST( row::cyclic(8,1,2)  == row("14567823") );
  RINGING_TEST( row::cyclic(8,1,5)  == row("17823456") );
  RINGING_TEST( row::cyclic(8,1,7)  == row("12345678") );
  RINGING_TEST( row::cyclic(8,1,13) == row("18234567") );
}

void test_row_isrounds(void)
{
  RINGING_TEST(   row(0).isrounds() );
  RINGING_TEST(   row(1).isrounds() );
  RINGING_TEST(   row(2).isrounds() );

  RINGING_TEST(   row("12345678").isrounds() );
  RINGING_TEST( ! row("21").isrounds() );
}

void test_row_ispblh(void)
{
  RINGING_TEST( row::pblh(0).ispblh() == 1 ); // ???
  // RINGING_TEST( row::pblh(1).ispblh() == 1 ); // ??? 
  // RINGING_TEST( row::pblh(2).ispblh() == 1 ); // ???
  RINGING_TEST( row::pblh(3).ispblh() == 1 );
  RINGING_TEST( row::pblh(6).ispblh() == 1 );
  RINGING_TEST( row::pblh(9).ispblh() == 1 );

  RINGING_TEST(   row::rounds(0).ispblh() ); // ??? 
  RINGING_TEST( ! row::rounds(1).ispblh() );
  RINGING_TEST( ! row::rounds(2).ispblh() );
  RINGING_TEST( ! row::rounds(3).ispblh() );
  RINGING_TEST( ! row::rounds(6).ispblh() );
  RINGING_TEST( ! row::rounds(9).ispblh() );

  RINGING_TEST(   row( "135264" ).ispblh() );
  RINGING_TEST(   row( "156342" ).ispblh() );
  RINGING_TEST( ! row( "165342" ).ispblh() );

  RINGING_TEST(   row( "1246375" ).ispblh() );
  RINGING_TEST(   row( "1267453" ).ispblh() );
  RINGING_TEST( ! row( "1267543" ).ispblh() );

  RINGING_TEST( row("123457968").ispblh() );  

  {
    row r( row::pblh(10) ); 
    RINGING_TEST( (r * r * r).ispblh() );
  }
}

void test_row_sign(void)
{
  RINGING_TEST( row(  ).sign() == +1 );
  RINGING_TEST( row(1 ).sign() == +1 );
  RINGING_TEST( row(2 ).sign() == +1 );
  RINGING_TEST( row(17).sign() == +1 );

  RINGING_TEST( row("234561").sign() == -1 );
  RINGING_TEST( row("234516").sign() == +1 );
  RINGING_TEST( row("352461").sign() == +1 );

  RINGING_TEST( row("312647958").sign() == -1 );
  RINGING_TEST( row("167832495").sign() == +1 );
}

void test_row_cycles(void)
{
  RINGING_TEST( row().cycles() == "" );
  RINGING_TEST( row(1).cycles() == "1" );
  RINGING_TEST( row(5).cycles() == "1,2,3,4,5" );
  RINGING_TEST( row("124536").cycles() == "1,2,345,6" );
  RINGING_TEST( row("214563").cycles() == "12,3456" );
  RINGING_TEST( row("2145673").cycles() == "12,34567" );
}

void test_row_order(void)
{
  RINGING_TEST( row(          ).order() == 1  );
  RINGING_TEST( row(1         ).order() == 1  );
  RINGING_TEST( row("21"      ).order() == 2  );
  RINGING_TEST( row("234561"  ).order() == 6  );
  RINGING_TEST( row("21436578").order() == 2  );
  RINGING_TEST( row("2315674" ).order() == 12 );
}

void test_row_comparison(void)
{
  // ???
  RINGING_TEST(    row( "654321" ) >  row( 8 )   );
  RINGING_TEST(    row( "654321" ) >= row( 8 )   );
  RINGING_TEST( !( row( "654321" ) <  row( 8 ) ) );
  RINGING_TEST( !( row( "654321" ) <= row( 8 ) ) );

  RINGING_TEST(    row( "654321" ) >  row( 6 )   );
  RINGING_TEST(    row( "654321" ) >= row( 6 )   );
  RINGING_TEST( !( row( "654321" ) <  row( 6 ) ) );
  RINGING_TEST( !( row( "654321" ) <= row( 6 ) ) );

  RINGING_TEST( !( row( "1432" ) >  row( "1432" ) ) );
  RINGING_TEST(    row( "1432" ) >= row( "1432" )   );
  RINGING_TEST( !( row( "1432" ) <  row( "1432" ) ) );
  RINGING_TEST(    row( "1432" ) <= row( "1432" )   );

  RINGING_TEST(    row( "1234" ) < row( "1243" )   );
  RINGING_TEST(    row( "1243" ) < row( "1432" )   );
  RINGING_TEST(    row( "1432" ) < row( "4312" )   );
}


// ---------------------------------------------------------------------
// Tests for the permute functions

void test_permuter_with_changes(void)
{
  vector< change > changes;  
  changes.push_back( change( 5, "3" ) );  
  changes.push_back( change( 5, "1" ) );
  changes.push_back( change( 5, "5" ) );

  vector< row > rows;  
  rows.push_back( row( 5 ) );

  transform( changes.begin(), changes.end(), 
	     back_inserter( rows ),
	     permute( 5 ) );

  RINGING_TEST( rows.size() == 4 );
  RINGING_TEST( rows[0] == "12345" );
  RINGING_TEST( rows[1] == "21354" );
  RINGING_TEST( rows[2] == "23145" );
  RINGING_TEST( rows[3] == "32415" );
}

void test_permuter_with_rows(void)
{
  vector< row > input;  
  input.push_back( row( "213546" ) );  
  input.push_back( row( "214365" ) );  
  input.push_back( row( "654321" ) );  

  vector< row > output;  
  output.push_back( row( 6 ) );

  transform( input.begin(), input.end(), 
	     back_inserter( output ),
	     permute( 5 ) );

  RINGING_TEST( output.size() == 4 );
  RINGING_TEST( output[0] == "123456" );
  RINGING_TEST( output[1] == "213546" );
  RINGING_TEST( output[2] == "125364" );
  RINGING_TEST( output[3] == "463521" );
}


void test_row_permuter_with_changes(void)
{
  vector< change > changes;  
  changes.push_back( change( 5, "3" ) );  
  changes.push_back( change( 5, "1" ) );
  changes.push_back( change( 5, "5" ) );

  vector< row > rows;  
  rows.push_back( row( 5 ) );

  row r(5);

  transform( changes.begin(), changes.end(), 
	     back_inserter( rows ),
	     permute( r ) );

  RINGING_TEST( rows.size() == 4 );
  RINGING_TEST( rows[0] == "12345" );
  RINGING_TEST( rows[1] == "21354" );
  RINGING_TEST( rows[2] == "23145" );
  RINGING_TEST( rows[3] == "32415" );

  RINGING_TEST( r == "32415" );
}

void test_row_permuter_with_rows(void)
{
  vector< row > input;  
  input.push_back( row( "213546" ) );  
  input.push_back( row( "214365" ) );  
  input.push_back( row( "654321" ) );  

  vector< row > output;  
  output.push_back( row( 6 ) );

  row r(5);

  transform( input.begin(), input.end(), 
	     back_inserter( output ),
	     permute( r ) );

  RINGING_TEST( output.size() == 4 );
  RINGING_TEST( output[0] == "123456" );
  RINGING_TEST( output[1] == "213546" );
  RINGING_TEST( output[2] == "125364" );
  RINGING_TEST( output[3] == "463521" );

  RINGING_TEST( r == "463521" );
}

// ---------------------------------------------------------------------
// Tests for row_block class

void test_row_block_constructors(void)
{
  vector< change > changes;  
  changes.push_back( change( 5, "3" ) );  
  changes.push_back( change( 5, "1" ) );
  changes.push_back( change( 5, "5" ) );

  {
    row_block rb( changes );  
    
    RINGING_TEST( rb.size() == 4 );
    RINGING_TEST( rb[0] == "12345" );
    RINGING_TEST( rb[1] == "21354" );
    RINGING_TEST( rb[2] == "23145" );
    RINGING_TEST( rb[3] == "32415" );
  }

  {
    row_block rb( changes, "54321" );
    RINGING_TEST( rb[2] == "43521" );
  }
}

void test_row_block_set_start(void)
{
  vector< change > changes;  
  changes.push_back( change( 5, "3" ) );  
  changes.push_back( change( 5, "1" ) );
  changes.push_back( change( 5, "5" ) );

  row_block rb( changes );  
  RINGING_TEST( rb.set_start( "54321" ) == "54321" );
  RINGING_TEST( &rb.recalculate() == &rb );

  RINGING_TEST( rb[2] == "43521" );
}

void test_row_block_recalculate(void)
{
  vector< change > changes;  
  changes.push_back( change( 5, "3" ) );  
  changes.push_back( change( 5, "1" ) );
  changes.push_back( change( 5, "5" ) );

  row_block rb( changes );
  rb[2] = "54321";
  rb.recalculate(2);

  RINGING_TEST( rb.size() == 4 );
  RINGING_TEST( rb[0] == "12345" );
  RINGING_TEST( rb[1] == "21354" );
  RINGING_TEST( rb[2] == "54321" );
  RINGING_TEST( rb[3] == "45231" );
}

void test_row_block_get_changes(void)
{
  vector< change > changes;
  row_block rb( changes );

  RINGING_TEST( &rb.get_changes() == &changes );
}

// ---------------------------------------------------------------------
// Register the tests

RINGING_END_ANON_NAMESPACE

RINGING_START_TEST_FILE( row )

  // Tests for numeric utilities
  RINGING_REGISTER_TEST( test_hcf )
  RINGING_REGISTER_TEST( test_lcm )

  // Tests for the row class
  RINGING_REGISTER_TEST( test_row_copy )
  RINGING_REGISTER_TEST( test_row_equals )
  RINGING_REGISTER_TEST( test_row_invalid )
  RINGING_REGISTER_TEST( test_row_subscript )
  RINGING_REGISTER_TEST( test_row_multiply_row )
  RINGING_REGISTER_TEST( test_row_divide_row )
  RINGING_REGISTER_TEST( test_row_multiply_change )
  RINGING_REGISTER_TEST( test_row_inverse )
  RINGING_REGISTER_TEST( test_row_print )
  RINGING_REGISTER_TEST( test_row_bells )
  RINGING_REGISTER_TEST( test_row_rounds )
  RINGING_REGISTER_TEST( test_row_named_rows )
  RINGING_REGISTER_TEST( test_row_pblh )
  RINGING_REGISTER_TEST( test_row_cyclic )
  RINGING_REGISTER_TEST( test_row_isrounds )
  RINGING_REGISTER_TEST( test_row_ispblh )
  RINGING_REGISTER_TEST( test_row_sign )
  RINGING_REGISTER_TEST( test_row_cycles )
  RINGING_REGISTER_TEST( test_row_order )
  RINGING_REGISTER_TEST( test_row_comparison )

  // Tests for the permute functions
  RINGING_REGISTER_TEST( test_permuter_with_changes )
  RINGING_REGISTER_TEST( test_permuter_with_rows )
  RINGING_REGISTER_TEST( test_row_permuter_with_changes )
  RINGING_REGISTER_TEST( test_row_permuter_with_rows )

  // Tests for the row_block class
  RINGING_REGISTER_TEST( test_row_block_constructors )
  RINGING_REGISTER_TEST( test_row_block_set_start )
  RINGING_REGISTER_TEST( test_row_block_recalculate )
  RINGING_REGISTER_TEST( test_row_block_get_changes )

RINGING_END_TEST_FILE

RINGING_END_NAMESPACE
