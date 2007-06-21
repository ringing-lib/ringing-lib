// -*- C++ -*- extent.cpp - utility print an extent 
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

#include <ringing/common.h>
#include <ringing/extent.h>
#include <ringing/row.h>
#include <ringing/streamutils.h>

#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#else
#include <iostream>
#endif

#if RINGING_OLD_INCLUDES
#include <iterator.h>
#else
#include <iterator>
#endif

#include "args.h"
#include "init_val.h"

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments 
{
  init_val<int,0> bells;  // nt
  init_val<int,0> hunts;  // nh
  init_val<int,0> tenors; // nt-nw-nh

  init_val<bool,false> in_course;

  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt
         ( 'b', "bells",
           "The number of bells.  This option is required", "BELLS",
           bells ) );

  p.add( new integer_opt
         ( 'h', "hunts",
           "The number of fixed 'hunt' bells at front of each row", "NUM",
           hunts ) );

  p.add( new integer_opt
         ( 't', "tenors",
           "The number of fixed tenors at the end of each row", "NUM",
           tenors ) );

  p.add( new boolean_opt
         ( 'i', "in-course",
	   "Only list in-course rows",
           in_course ) );
}

bool arguments::validate( arg_parser& ap )
{
  if ( bells < hunts + tenors ) 
    {
      ap.error( "More hunt bells and fixed tenors than the "
                "total number of bells" );
      return false;
    }

  if ( bells >= int(bell::MAX_BELLS) )
    {
      ap.error( make_string() << "The number of bells must be less than "
                << bell::MAX_BELLS );
      return false;
    }

  return true;
}

int main( int argc, char *argv[] )
{
  arguments args;

  {
    arg_parser ap( argv[0], "extent -- print all rows", "OPTIONS" );
    args.bind( ap );

    if ( !ap.parse(argc, argv) )
      {
        ap.usage();
        return 1;
      }
    
    if ( !args.validate(ap) )
      return 1;
  }

  const unsigned int nw = args.bells - args.tenors - args.hunts;
  const unsigned int nh = args.hunts, nt = args.bells;

  for ( extent_iterator i(nw, nh, nt), e; i != e; ++i ) {
    if ( !args.in_course || i->sign() > 0 )
      cout << *i << "\n";
  }
}
