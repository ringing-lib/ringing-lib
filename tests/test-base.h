// -*- C++ -*- test-base.h - Framework for testing the ringing class library 
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

#ifndef RINGING_TEST_INCLUDED
#define RINGING_TEST_INCLUDED

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#include <iosfwd.h>
#else
#include <stdexcept>
#include <iosfwd>
#endif

// Macros for abstracting the use of namespace ringing::test
#if RINGING_USE_NAMESPACES
# define RINGING_USING_TEST using namespace ringing::test;
# define RINGING_START_NAMESPACE_TEST namespace test {
# define RINGING_END_NAMESPACE_TEST }
#else
# define RINGING_USING_TEST
# define RINGING_START_NAMESPACE_TEST
# define RINGING_END_NAMESPACE_TEST
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_NAMESPACE_TEST

class location
{
public:
  location( const char *file, unsigned int line ) : file(file), line(line) {}
  friend ostream &operator<<( ostream &os, const location &loc );

private:
  const char *file;
  unsigned int line;
};

class test_failure : public domain_error
{
public:
  test_failure( const location &loc, const string &str );
};

class condition_failure : public test_failure
{
public:
  condition_failure( const location &loc, const char *condition );
};

class missing_exception : public test_failure
{
public:
  missing_exception( const location &loc, const char *statement );
};

class unexpected_exception : public test_failure
{
public:
  unexpected_exception( const location &loc );
  unexpected_exception( const location &loc, const exception &e );
};

void register_test( const char *str, void (*test)(void) );
bool run_tests( bool verbose );

void handle_exception( const location& loc );

RINGING_END_NAMESPACE_TEST

RINGING_END_NAMESPACE

#define RINGING_TEST( CONDITION )					\
  do {									\
    RINGING_USING_TEST							\
    location loc( __FILE__, __LINE__ );					\
    try {								\
      if ( !(CONDITION) ) throw condition_failure( loc, #CONDITION );	\
    } catch ( ... ) {							\
      handle_exception(loc);						\
    }									\
  } while ( false )


#define RINGING_TEST_THROWS( STATEMENT, EXCEPTION )			\
  do {									\
    RINGING_USING_TEST							\
    location loc( __FILE__, __LINE__ );					\
    try {								\
      (STATEMENT); throw missing_exception( loc, #STATEMENT );		\
    } catch ( const EXCEPTION & ) {					\
      ;									\
    } catch ( ... ) {							\
      handle_exception(loc);						\
    }									\
  } while ( false )

#define RINGING_REGISTER_TEST( TEST_FUNCTION )				\
  register_test( #TEST_FUNCTION, TEST_FUNCTION );

#define RINGING_START_TEST_FILE( FILE )					\
  void register_##FILE##_tests() {					\
    RINGING_USING_TEST

#define RINGING_END_TEST_FILE						\
  }

#define RINGING_RUN_TEST_FILE( FILE )					\
  extern void register_##FILE##_tests();				\
  register_##FILE##_tests();

#endif
