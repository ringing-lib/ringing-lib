// main.cpp - Entry point for touchsearch
// Copyright (C) 2002, 2003, 2007 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/change.h>
#include <ringing/method.h>
#include <ringing/streamutils.h>
#include <ringing/table_search.h>
#include <ringing/touch.h>
#include <ringing/pointers.h>
#include <ringing/litelib.h>

#include "prog_args.h"
#include "iteratorutils.h"

#include <string>
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif
#if RINGING_OLD_IOSTREAMS
#include <iostream.h>
#include <fstream.h>
#else
#include <iostream>
#include <fstream>
#endif


RINGING_USING_NAMESPACE

// NB.  Once ostream &operator<<( ostream &, const touch & ) has
// been defined, there will be no need for this.

// A function object to attempt to sanely print the touch
class print_touch
{
public:
  typedef touch argument_type;
  typedef void result_type;
  
  print_touch( arguments const& args, method const& meth, 
               string const& filter_line ) 
    : args(args), meth(meth), filter_line(filter_line)
  {}
    
  void operator()( const touch &t ) const
  {
    if (args.filter_mode)
      cout << filter_line << "\t";

    int n=0;
    for ( touch::const_iterator i( t.begin() ); i != t.end(); ++i )
      if ( ++n % meth.size() == 0 ) {
        size_t cn = find( args.calls.begin(), args.calls.end(), *i ) 
          - args.calls.begin();
        if ( cn < args.calls.size() ) cout << args.call_strs[cn];
        else cout << '.';
      }

    cout << "  (" << n << " changes)" << endl;
  } 

private:
  arguments const& args;
  method const& meth;
  string filter_line;
};

class have_finished
{
public:
  typedef touch argument_type;
  typedef bool result_type;

  have_finished( arguments const& args )
    : args(args), i(new size_t(0u))
  {}

  bool operator()( const touch& )
  {
    // Abort when we reach the --limit
    return ++*i >= args.search_limit 
      // Or after the first one for a --filter search
      || args.filter_mode;
  }

private:
  arguments const& args;
  shared_pointer<size_t> i;
};

void search( arguments const& args, method const& meth, 
             string const& filter_line )
{
  const size_t leadlen = meth.size() * args.pends.size();
  pair<size_t, size_t> leads = range_div( args.length, leadlen );
  
  table_search searcher( meth, args.calls, args.pends, leads,
                         args.ignore_rotations );
  print_touch printer( args, meth, filter_line );

  touch_search_until( searcher, iter_from_fun(printer), have_finished(args) );
}

void filter( arguments const& args )
{
  litelib in( args.bells, std::cin );
  for ( library::const_iterator i=in.begin(), e=in.end(); i!=e; ++i )
    {
      method filter_method;
      try {
        filter_method = i->meth();
      } 
      catch ( std::exception const& ex ) {
        std::cerr << "Error reading method from input stream: "
                  << ex.what() << "\n";
        string pn;  try { pn = i->pn(); } catch (...) {}
        if ( pn.size() ) std::cerr << "Place notation: '" << pn << "'\n";
        std::cerr << std::flush;

        continue;
      }

      string filter_line = i->pn();
      filter_line += "\t";
      filter_line += i->get_facet<litelib::payload>();

      search( args, filter_method, filter_line );
    }
}

int main( int argc, char *argv[] )
{
  try
    {
      arguments args( argc, argv );

      if ( args.filter_mode )
        filter( args );
      else
        search( args, args.meth, string() );
    }
  catch ( const exception &ex )
    {
      cerr << "Unexpected error: " << ex.what() << endl;
      exit(1);
    }
  catch ( ... )
    {
      cerr << "An unknown error occured" << endl;
      exit(1);
    }

  return 0;
}

