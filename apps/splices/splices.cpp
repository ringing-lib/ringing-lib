// -*- C++ -*- splices.cpp - utility to find splices between methods
// Copyright (C) 2010 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/mathutils.h>
#include <ringing/streamutils.h>
#include <ringing/methodset.h>
#include <ringing/litelib.h>
#include <ringing/group.h>
#include <ringing/falseness.h>
#include "args.h"

#include <iostream>

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  init_val<int,0>      bells;

  init_val<int,-1>     only_n_leads;
  init_val<bool,false> null_splices;
  init_val<bool,false> same_le;

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

  p.add( new boolean_opt
         ( 'n', "null-splice",
           "Display pairs of methods with no splice", 
           null_splices ) );

  p.add( new boolean_opt
         ( 'i', "in-course",
           "Only display splices for in-course lead heads", 
           in_course ) );

  p.add( new boolean_opt
         ( 'e', "same-lead-ends",
           "Only look at pairs of methods with the same lead end change",
           same_le ) );

  p.add( new integer_opt
         ( 'l', "leads",
           "Only display methods with a NUM-lead splice", "NUM",
           only_n_leads ) );
}

bool arguments::validate( arg_parser& ap )
{               
  if ( bells <= 0 )
    {
      ap.error( "Then number of bells must be positive" );
      return false;
    }

  if ( bells >= int(bell::MAX_BELLS) )
    {
      ap.error( make_string() << "The number of bells must be less than "
                << bell::MAX_BELLS );
      return false;
    }

  if ( null_splices && only_n_leads != -1 ) 
    {
      ap.error( "The -l and -n options are incompatible" );
      return false;
    }


  return true;
}

string describe_splice( group const& sg )
{
  return make_string() << (sg.size()/2) << "-lead";
}

int main( int argc, char *argv[] )
{
  arguments args;

  {
    arg_parser ap( argv[0], "musgrep -- grep rows for music", "OPTIONS" );
    args.bind( ap );

    if ( !ap.parse(argc, argv) )
      {
        ap.usage();
        return 1;
      }
    
    if ( !args.validate(ap) )
      return 1;
  }

  // Read methods into memory -- we need to make multiple passes over
  // the list and we can't do that on standard input.
  methodset meths;
  {
    meths.store_facet< litelib::payload >();
    litelib in( args.bells, cin );
    copy( in.begin(), in.end(), back_inserter(meths) );  
  }

  for ( methodset::const_iterator i=meths.begin(), e=meths.end(); i!=e; ++i ) 
  {
    methodset::const_iterator j=i;  ++j;
    for ( ; j != e; ++j ) 
    {
      if ( args.same_le && i->meth().back() != j->meth().back() )
        continue;
 
      int flags = 0; 
      if ( args.in_course ) 
        flags |= falseness_table::in_course_only;
      group sg( falseness_table( i->meth(), j->meth(), flags )
                  .generate_group() );

      if ( args.null_splices &&
           sg.size() == factorial(args.bells-1) / (args.in_course ? 2 : 1) )
        continue;

      if ( args.only_n_leads != -1 && sg.size() != args.only_n_leads * 2 )
        continue;

      string a = i->get_facet< litelib::payload >();
      string b = j->get_facet< litelib::payload >();
      cout << a << " & " << b << "\t" << describe_splice(sg) << "\n";
    }
  }
}
