// -*- C++ -*- main.cpp - the entry point for methsearch 
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
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/falseness.h>
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#else
#include <iostream>
#endif
#include "args.h"
#include "prog_args.h"
#include "libraries.h"
#include "music.h"
#include "format.h"
#include "search.h"


RINGING_USING_NAMESPACE
RINGING_USING_STD

int main( int argc, char *argv[] )
{
  arguments args;

  {
    arg_parser ap(argv[0],
      "methsearch -- finds methods with particular properties.", 
		  "OPTIONS" );
    
    args.bind( ap );
    
    if ( !ap.parse(argc, argv) ) 
      {
	ap.usage();
	return 1;
      }

    if ( !args.validate( ap ) ) 
      return 1;
  }

  method_libraries::init();

  // So that errors with -M options are presented now rather than later.
  musical_analysis::force_init( args.bells );

  if ( formats_have_falseness_groups() )
    false_courses::optimise( args.bells );

  {
    method startmeth;
    try
      {
	startmeth = method( args.startmeth, args.bells );
      }
    catch ( const exception &e )
      {
	cerr << argv[0] 
	     << ": Unable to parse place-notation passed to start-at\n"
	     << e.what() << "\n";
	return 1;
      }

    try 
      {
	run_search( args, startmeth );
      }
    catch ( const exit_exception& ) 
      {}
  }

  if ( args.status ) clear_status();

  args.outputs.flush(); // Causes stats to be emitted.

  return 0;
}
