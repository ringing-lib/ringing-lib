// -*- C++ -*- music-tests.cc - Tests for the method class
// Copyright (C) 2002 Mark Banner <mark@standard8.co.uk>

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

#include <ringing/music.h>
#include "test-base.h"

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

// ---------------------------------------------------------------------
// Tests for class music_details

void test_music_details_expression(void)
{
  // These tests ensure the expression is checked correctly
  // within the class.
  RINGING_TEST( music_details("12345").get() == "12345" );
  RINGING_TEST( music_details("*45").get() == "*45" );
  RINGING_TEST( music_details("1??45").get() == "1??45" );
  RINGING_TEST( music_details("*[56]78").get() == "*[56]78" );
  RINGING_TEST( music_details("*[56][56]78").get() == "*[56][56]78" );
  // Future functionality
  // RINGING_TEST( music_details("*{5+}*").get() == "*{5+}*" );
  // RINGING_TEST( music_details("?{4-}?").get() == "?{4-}?" );
#ifdef RINGING_USE_EXCEPTIONS
  RINGING_TEST_THROWS( music_details("[[45]6]"), music_details::invalid_regex );
  RINGING_TEST_THROWS( music_details("[45"), music_details::invalid_regex );
  RINGING_TEST_THROWS( music_details("[*5]"), music_details::invalid_regex );
  RINGING_TEST_THROWS( music_details("56+"), music_details::invalid_regex );
  //  RINGING_TEST_THROWS( music_details("?{4*}"), music_details::invalid_regex );
  //  RINGING_TEST_THROWS( music_details("?4}"), music_details::invalid_regex );
  //  RINGING_TEST_THROWS( music_details("*{4}*"), music_details::invalid_regex );
#else
  RINGING_TEST( music_details("[45").get() == "" );
  RINGING_TEST( music_details("[*5]").get() == "" );
  RINGING_TEST( music_details("56+").get() == "" );
  //  RINGING_TEST( music_details("?{4*}").get() == "" );
  //  RINGING_TEST( music_details("?4}").get() == "" );
  //  RINGING_TEST( music_details("*{4}*").get() == "" );
#endif

  // Now test the set function
  music_details md;
  md.set("12345");
  RINGING_TEST( md.get() == "12345" );
  md.set("*?[56]78");
  RINGING_TEST( md.get() == "*?[56]78");
#ifdef RINGING_USE_EXCEPTIONS
  RINGING_TEST_THROWS( md.set("[45"), music_details::invalid_regex );
  RINGING_TEST_THROWS( md.set("[5*]"), music_details::invalid_regex );
  RINGING_TEST_THROWS( md.set("56+"), music_details::invalid_regex );
  /*  RINGING_TEST_THROWS( md.set("?{4*}"), music_details::invalid_regex );
  RINGING_TEST_THROWS( md.set("?4}"), music_details::invalid_regex );
  RINGING_TEST_THROWS( md.set("*{4}*"), music_details::invalid_regex );*/
#else
  md.set("[45");
  RINGING_TEST( md.get() == "" );
  md.set("[5*]");
  RINGING_TEST( md.get() == "" );
  md.set("56+");
  RINGING_TEST( md.get() == "" );
  /*  md.set("?{4*}");
  RINGING_TEST( md.get() == "" );
  md.set("?4}");
  RINGING_TEST( md.get() == "" );
  md.set("*{4}*");
  RINGING_TEST( md.get() == "" );*/
#endif
}

void test_music_details_possible_matches(void)
{
  // Test full line and ?
  RINGING_TEST( music_details("12345").possible_matches(5) == 1 );
  RINGING_TEST( music_details("12??5").possible_matches(5) == 2 );
  RINGING_TEST( music_details("1???5").possible_matches(5) == 6 );
  RINGING_TEST( music_details("????5").possible_matches(5) == 24 );
  RINGING_TEST( music_details("?????").possible_matches(5) == 120 );

  // Test 1 * at start
  RINGING_TEST( music_details("*2345").possible_matches(5) == 1 );
  RINGING_TEST( music_details("*345").possible_matches(5)  == 2 );
  RINGING_TEST( music_details("*45").possible_matches(5)   == 6 );
  RINGING_TEST( music_details("*5").possible_matches(5)    == 24 );
  RINGING_TEST( music_details("*").possible_matches(5)     == 120 );

  // Test 1 * at end
  RINGING_TEST( music_details("1234*").possible_matches(5) == 1 );
  RINGING_TEST( music_details("123*").possible_matches(5)  == 2 );
  RINGING_TEST( music_details("12*").possible_matches(5)   == 6 );
  RINGING_TEST( music_details("1*").possible_matches(5)    == 24 );
  
  // Test 2 *'s
  RINGING_TEST( music_details("*345*").possible_matches(5) == 6 );
  RINGING_TEST( music_details("*456*").possible_matches(6) == 24 );
  RINGING_TEST( music_details("*5678*").possible_matches(8) == 120 );

  // Test []
  RINGING_TEST( music_details("*[56]78").possible_matches(8) == 240 );
  RINGING_TEST( music_details("87[65]*").possible_matches(8) == 240 );
  RINGING_TEST( music_details("*[56][56]78").possible_matches(8) == 48 );
  /*
  // Test {}
  RINGING_TEST( music_details("*{4+}*").possible_matches(6) == 18);
  RINGING_TEST( music_details("{5-}*").possible_matches(8) == 24);
  */
  // Finish with a couple of complex ones.
  RINGING_TEST( music_details("87[65]*[65]").possible_matches(8) == 48 );
  RINGING_TEST( music_details("[12]*[56]?[56]78").possible_matches(8) == 24 );
}

void test_music_details_score(void)
{
  // Test the setting of the score.
  music_details md; RINGING_TEST( md.raw_score() == 1);
  music_details md1("12345", 2); RINGING_TEST( md1.raw_score() == 2 );
  md.set("12345", 4); RINGING_TEST( md.raw_score() == 4);
  
  // Score testing can't be done here, as it requires music/music_node
  // to set it - or hacking into the music_details private functions.
}

// ---------------------------------------------------------------------
// Register the tests

RINGING_END_ANON_NAMESPACE

RINGING_START_TEST_FILE( music )
  
  // Tests for the music_details class
  RINGING_REGISTER_TEST( test_music_details_expression )
  RINGING_REGISTER_TEST( test_music_details_possible_matches )
  RINGING_REGISTER_TEST( test_music_details_score )

RINGING_END_TEST_FILE

RINGING_END_NAMESPACE
