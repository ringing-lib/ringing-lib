// -*- C++ -*- test-main.cpp - Entry point to the test framework
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

#include <ringing/common.h>
#include "test-base.h"

RINGING_START_NAMESPACE

RINGING_USING_STD

int test_main()
{
  RINGING_RUN_TEST_FILE( change )
  RINGING_RUN_TEST_FILE( row )

  RINGING_USING_TEST
  if ( run_tests( true ) ) 
    return 0;
  else
    return 1;
}

RINGING_END_NAMESPACE

int main()
{
  return RINGING_PREFIX test_main();
}
