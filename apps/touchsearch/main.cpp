// main.cpp - Entry point for touchsearch
// Copyright (C) 2002, 2003, 2007, 2009, 2010 
// Richard Smith <richard@ex-parrot.com>

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
#include "join_plan_search.h"

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
#include <cassert>


RINGING_USING_NAMESPACE

RINGING_ULLONG touch_count = 0ul;

// NB.  Once ostream &operator<<( ostream &, const touch & ) has
// been defined, there will be no need for this.

// A function object to attempt to sanely print the touch
class print_touch
{
public:
  typedef touch argument_type;
  typedef void result_type; 

  enum format_options {
    comma_separate   = 0x01,  // Separate leads with a comma
    print_length     = 0x04,  // Append (120 changes)
    write_out_repeat = 0x08   // Write abcd x3 or abcdabcdabcd
  };
 
  print_touch( arguments const& args, 
               string const& filter_line, format_options flags ) 
    : args(args), filter_line(filter_line), flags(flags)
  {}

  void operator()( const touch &t ) const
  {
    ++touch_count;
    if (args.quiet) return;

    if (args.filter_mode)
      cout << filter_line << "\t";

    touch_child_list const* tl 
      = dynamic_cast<touch_child_list const*>( t.get_head() );
    assert( tl );
     
    size_t len = 0;   row r(args.bells);
    do for ( list<touch_child_list::entry>::const_iterator
               i = tl->children().begin(), e = tl->children().end();  
               i != e;  ++i ) 
    {    
      for ( int n = 0; n < i->first; ++n ) {
        change c;
        for ( touch_node::const_iterator 
                li = i->second->begin(), le = i->second->end(); 
                li != le; ++li, ++len )
          r *= c = *li;

        size_t cn = find( args.calls.begin(), args.calls.end(), c ) 
            - args.calls.begin();

        if ( (flags & comma_separate) && len != 0 ) cout << ',';
        if ( cn < args.calls.size() ) cout << args.call_strs[cn];
        else cout << args.plain_name;
      }
    }
    while ( (flags & write_out_repeat) && r.order() > 1 );

    if ( r.order() > 1 ) 
      cout << " x" << r.order();

    if ( flags & print_length )
      cout << "  (" << (len*r.order()) << " changes)";

    cout << endl;
  } 

private:
  arguments const& args;
  string filter_line;
  int flags;
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
    return args.search_limit != -1 && ++*i >= (size_t)args.search_limit 
      // Or after the first one for a --filter search
      || args.filter_mode;
  }

private:
  arguments const& args;
  shared_pointer<size_t> i;
};

void read_plan( int bells, istream& in, map<row, method>& plan )
{
  string line;  
  int n=1;
  while ( getline(in, line) ) {
    RINGING_ISTRINGSTREAM linestr(line);
    row r;    string pn, name;
    if ( !(linestr >> r >> pn) )  {
      cerr << "Error reading line #" << n << "\n";
      continue;
    }
    linestr >> name; // We don't care about errors here
    ++n;
    plan[r] = method( pn, bells, name );
  }
}

void search( arguments const& args, method const& meth, 
             string const& filter_line )
{
  print_touch::format_options fmt = static_cast<print_touch::format_options>( 
    args.comma_separate ? 
      ( print_touch::comma_separate | print_touch::write_out_repeat ) 
    : print_touch::print_length );

  scoped_pointer<search_base> searcher;
  
  print_touch printer( args, filter_line, fmt );

  if ( args.use_plan ) {
    // XXX: Should this require a command-line option?
    join_plan_search::flags f = join_plan_search::no_internal_falseness;

    map<row, method> plan;  read_plan( args.bells, cin, plan );

    searcher.reset
      ( new join_plan_search( args.bells, plan, args.calls, args.length, f ) );
  } 
  else {
    table_search::flags f = static_cast<table_search::flags>( 
      table_search::length_in_changes | 
      (args.ignore_rotations ? table_search::ignore_rotations : 0) |
      (args.mutually_true_parts ? table_search::mutually_true_parts : 0) );

    searcher.reset
      ( new table_search( meth, args.calls, args.pends, args.length, f ) );
  }

  touch_search_until( *searcher, iter_from_fun(printer), have_finished(args) );
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

      if (!args.quiet && (args.count || args.raw_count))
        cout << "\n";
      if (args.raw_count)
        cout << touch_count << "\n";
      else if (args.count)
        cout << "Found " << touch_count 
             << (args.filter_mode ? " methods\n" : " touches\n");
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

