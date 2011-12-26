// -*- C++ -*- splices.cpp - utility to find splices between methods
// Copyright (C) 2010, 2011 Richard Smith <richard@ex-parrot.com>

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
#include <list>
#include <set>
#include <map>
#include <utility>

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  init_val<int,0>      bells;

  init_val<bool,false> in_course;
  init_val<bool,false> out_of_course;
  init_val<bool,false> half_leads;

  init_val<int,-1>     only_n_leads;
  init_val<bool,false> null_splices;
  init_val<bool,false> same_le;

  init_val<bool,false> show_pn;
  init_val<bool,false> group_splices;
  init_val<bool,false> print_group;
  init_val<bool,false> print_falseness;

  init_val<bool,false> filter_mode;
  init_val<bool,false> read_rows;

  vector<string>       meth_str;
  vector<method>       meth;

  vector<row>          rows;

  arguments( int argc, char *argv[] );
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

arguments::arguments( int argc, char *argv[] )
{
  arg_parser ap( argv[0], "splices -- search for splices\v"
    "If no methods are given on the command line, then methods are read from "
    "standard input and all splices between them are located (unless -I is "
    "specified, in which case lead splices are removed).  If one method is "
    "given on the command line, then methods are read from standard input and "
    "tested for splices with the method on the command line.  If two "
    "methods are given on the command line, then the splice between those "
    "methods is found.", "OPTIONS METHODS" );

  bind( ap );

  if ( !ap.parse(argc, argv) )
    {
      ap.usage();
      exit(1);
    }
  
  if ( !validate(ap) )
    exit(1);
}

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );
           
  p.set_default( new strings_opt( '\0', "", "", "", meth_str ) );

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
         ( 'o', "out-of-course",
           "Only display splices between an in-course and an out-of-course "
           "lead head", 
           out_of_course ) );

  p.add( new boolean_opt
         ( 'h', "half-lead",
           "Calculate half-lead splices instead of whole lead splices",
           half_leads ) );

  p.add( new boolean_opt
         ( 'e', "same-lead-ends",
           "Only look at pairs of methods with the same lead end change",
           same_le ) );

  p.add( new boolean_opt
         ( 'p', "place-notation",
           "Use place notations instead of names", 
           show_pn ) );

  p.add( new boolean_opt
         ( 'g', "group-together",
           "Group together methods with mutual splices", 
           group_splices ) );

  p.add( new boolean_opt
         ( 'G', "print-group",
           "Print the elements of the splice group",
           print_group ) );

  p.add( new boolean_opt
         ( 'F', "print-falseness",
           "Print the elements of the inter-method falseness table",
           print_falseness ) );

  p.add( new integer_opt
         ( 'l', "leads",
           "Only display methods with a NUM-lead splice", "NUM",
           only_n_leads ) );

  p.add( new boolean_opt
         ( 'I', "filter",
           "Run as a method filter; if no methods are given on the command "
           "line, then lead-splices are excluded",
           filter_mode ) );

  p.add( new boolean_opt
         ( '\0', "read-rows",
           "Read the rows of a method from standard input",
           read_rows ) );
}

bool arguments::validate( arg_parser& ap )
{               
  if ( bells <= 0 )
    {
      ap.error( "Then number of bells must be positive" );
      return false;
    }

  if ( bells > int(bell::MAX_BELLS) )
    {
      ap.error( make_string() << "The number of bells must be less than "
                << bell::MAX_BELLS+1 );
      return false;
    }

  if ( (in_course || read_rows) && out_of_course )
    { 
      ap.error( "The -o option cannot be used with either -i or --read-rows" );
      return false;
    }

  unsigned int meth_num = 0;
  for ( vector<string>::const_iterator i=meth_str.begin(), e=meth_str.end();
        i != e; ++i )
    {
      try {  
        meth.push_back
          ( method( *i, bells, make_string() << "Method " << ++meth_num ) );
      } catch ( const exception& e ) {
        ap.error( make_string() << "Invalid method place notation: "
                  << e.what() );
        return false;
      }
    }

  if ( null_splices && only_n_leads != -1 ) 
    {
      ap.error( "The -l and -n options are incompatible" );
      return false;
    }

  if ( group_splices && (null_splices || print_group || print_falseness) ) 
    {
      ap.error
        ( "The -g option is incompatible with the -n, -G and -F options" );
      return false;
    }

  if ( filter_mode && ( print_group || print_falseness || group_splices 
                        || show_pn ) ) 
    {
      ap.error
        ( "The -g, -p, -G and -F options cannot be used when filtering" );
      return false;
    }

  if ( filter_mode && ( only_n_leads != -1 || null_splices ) && meth.empty() ) 
    {
      ap.error
        ( "The -l and -n options cannot be used when filtering" );
      return false;
    }

  if ( filter_mode && meth.size() > 1 ) 
    {
      ap.error
        ( "At most one method can be provided on the command line when "
          "filtering" );
      return false;
    }

  if ( (print_group || print_falseness) && meth.size() != 2 ) 
    {
      ap.error
        ( "The -G or -F options needs two methods on the command line" );
      return false;
    }

  if ( meth.size() > 2 )
   {
     ap.error
       ( "At most two methods can be provided on the command line" );
     return false;
   }

  return true;
}

class splices {
public:
  splices( arguments const& args ) : args(args) {}

  void find_splices( library const& lib );

private:
  static string name_or_pn( arguments const& args, method const& m );
  class sort_function;

  bool test_splice( method const& a, method const& b );
  string test_splice( method const& a, vector<row> const& b, 
                      string const& b_name = string() );
  bell get_pivot( group const& sg ) const;
  pair<bell, bell> get_swapping_pair( group const& sg ) const;
  string describe_splice( group const& sg ) const;
  void save_splice( method const& a, method const& b, string const& d );
  void print_splice_groups() const;
  bool is_just_le_vars( set<method> const& s ) const;
  void print_set( set<method> const& s ) const;
  set<method, sort_function> get_lead_splices( method const& m ) const;
  static method get_method( library_entry const& e );
  bool has_lead_splices( method const& m ) const;

  typedef list< pair< string, set<method> > > table_type;
  table_type table;
  arguments const& args;
};

string splices::name_or_pn( arguments const& args, method const& m )
{
  string astr = m.name();
  // If we have no names, use place notations
  if ( astr.empty() || args.show_pn ) 
    return m.format( method::M_FULL_SYMMETRY | method::M_DASH );
  else
    return astr;
}

// Does the set of methods just consist of lead-end variants 
bool splices::is_just_le_vars( set<method> const& s ) const
{
  if (s.size() < 2) return true;
  set<method>::const_iterator first=s.begin(), i=first, e=s.end();
  for (++i; i!=e; ++i) 
    if ( !equal( i->begin(), i->end()-1, first->begin() ) )
      return false;
  return true;
}

// Attempt to be clever.  Sort numerically if they're numbers;
// sort numerically and then by suffix for things like 142B;
// and sort alphabetically otherwise.
struct splices::sort_function {
  sort_function( arguments const& args ) : args(args) {}

  bool operator()( method const& xm, method const& ym ) const {
    string x = name_or_pn(args, xm), y = name_or_pn(args, ym); 
    if ( y.empty() ) return false;
    if ( x.empty() ) return true;
    if ( isdigit(x[0]) && isdigit(y[0]) ) {
      int xd = strtol(x.c_str(), NULL, 10);
      int yd = strtol(y.c_str(), NULL, 10);
      if (xd < yd) return true;
      if (xd > yd) return false;
    } 
    else if ( isdigit(x[0]) ) return true;
    else if ( isdigit(y[0]) ) return false;
    return x < y;
  }

private:
  arguments const& args;
};

set<method, splices::sort_function> 
  splices::get_lead_splices( method const& m ) const 
{
  set<method, sort_function> ls(( sort_function(args) ));

  for ( table_type::const_iterator i=table.begin(), e=table.end(); i!=e; ++i )
    if ( i->first == "1-lead" && i->second.find(m) != i->second.end() ) {
      for ( set<method>::const_iterator 
              i2=i->second.begin(), e2=i->second.end(); i2 != e2; ++i2 )
        if ( i2->back() == m.back() )
          ls.insert(*i2);
      break;
    }

  if (ls.size() == 1) ls.clear();

  return ls;
}

void splices::print_set( set<method> const& s ) const
{
  set<method, sort_function> s2( s.begin(), s.end(), sort_function(args) );

  bool need_sep = false;
  for ( set<method, sort_function>::const_iterator 
          i=s2.begin(), e=s2.end(); i != e; ++i ) {
    if (need_sep) cout << ", ";

    set<method, sort_function> ls( get_lead_splices(*i) );
    if ( ls.size() ) {
      cout << "[";
      bool need_sep_2 = false;
      for ( set<method, sort_function>::const_iterator 
              i2=ls.begin(), e2=ls.end(); i2 != e2; ++i2 ) {
        if (need_sep_2) cout << ", ";
        cout << name_or_pn(args, *i2); 
        need_sep_2 = true;

        if (*i2 != *i) s2.erase(*i2);
      }
      cout << "]";
    }
    else 
      cout << name_or_pn(args, *i); 

    need_sep = true;
  }
}

bool splices::has_lead_splices( method const& m ) const
{
  for ( table_type::const_iterator i=table.begin(), e=table.end(); i!=e; ++i )
    if ( i->first == "1-lead" && i->second.find(m) != i->second.end() )
      return true;
  return false;
}

void splices::print_splice_groups() const
{
  for ( table_type::const_iterator i=table.begin(), e=table.end(); i!=e; ++i )
  {
    // We've got spurious lead splice entries here to enable bracketing
    // of lead splices within other types of splice.
    if ( args.only_n_leads > 1 && i->first == "1-lead" )
      continue;

    // If we're running with -ge, we want to group methods by their lead-end.
    if ( args.same_le ) {
      if ( is_just_le_vars(i->second) )
        continue;

      map< change, set<method> > by_le;
      for ( set<method>::const_iterator
            i2=i->second.begin(), e2=i->second.end(); i2 != e2; ++i2 )
        by_le[ i2->back() ].insert( *i2 );
 
      bool need_sep = false;
      for ( map< change, set<method> >::const_iterator 
            i2=by_le.begin(), e2=by_le.end(); i2 != e2; ++i2 ) {
        if (need_sep) cout << " / ";
        print_set( i2->second );
        need_sep = true;
      }
    }
    else 
      print_set( i->second );

    cout << "\t" << i->first << "\n";
  }
}

void splices::save_splice( method const& a, method const& b, string const& d )
{
  table_type::iterator found = table.end();
  for ( table_type::iterator i=table.begin(), e=table.end(); i!=e;  )
    if ( d == i->first && 
         ( i->second.find(a) != i->second.end() ||
           i->second.find(b) != i->second.end() ) ) 
    {
      if ( found == e ) {
        i->second.insert(a);
        i->second.insert(b); 
        found = i++;
      }
      else {
        // It's possible that a is part of one set and b is part of another.
        // If so, we need to merge them.  This can happen if some of the 
        // splices between set members are more specific and thus not 
        // shown.  We'll have already inserted a,b into found so no need
        // to do again.
        found->second.insert( i->second.begin(), i->second.end() );
        table.erase(i++);
      }
    }
    else ++i;

  if ( found == table.end() ) {
    set<method> meths;
    meths.insert(a);  meths.insert(b);
    table.push_back( make_pair( d, meths ) );
  }
}

bell splices::get_pivot( group const& sg ) const
{
  const int hunts = 1;
  for ( int b=hunts; b<sg.bells(); ++b ) {
    bool fixed = true;
    for ( group::const_iterator i=sg.begin(), e=sg.end(); fixed && i!=e; ++i )
      if ( (*i)[b] != b )
        fixed = false;
    if (fixed) return bell(b);
  }

  return bell(-1);
}

pair<bell, bell> splices::get_swapping_pair( group const& sg ) const
{
  const int hunts = 1;
  for ( int b1=hunts; b1<sg.bells(); ++b1 ) 
  for ( int b2=b1+1;  b2<sg.bells(); ++b2 ) {
    bool fixed = true;
    for ( group::const_iterator i=sg.begin(), e=sg.end(); fixed && i!=e; ++i )
      if ( !( (*i)[b1] == b1 && (*i)[b2] == b2 ) &&
           !( (*i)[b1] == b2 && (*i)[b2] == b1 ) )
        fixed = false;
    if (fixed) return make_pair( bell(b1), bell(b2) );
  }
  return make_pair( bell(-1), bell(-1) );
}
 
string splices::describe_splice( group const& sg ) const
{
  const int hunts = 1;

  make_string os;
  os << (sg.size()/2) << "-lead";

  int const f2 = factorial(sg.bells() - hunts - 2);
  int const f1 = f2 * (sg.bells() - hunts - 1);

  if (sg.size() > 2 && (sg.size() == f1 || sg.size() == f1/2)) {
    bell pivot = get_pivot(sg);
    if (pivot != bell(-1))
      os << " (pivot: " << pivot << ")";
  }
  if (sg.size() > 2 && (sg.size() == f2 || sg.size() == f2/2)) {
    pair<bell, bell> swaps = get_swapping_pair(sg);
    if (swaps.first != bell(-1) && swaps.second != bell(-1))
      os << " (swaps: " << swaps.first << "&" << swaps.second << ")";
  }
  return os;
}

string splices::test_splice( method const& a, vector<row> const& b,
                             string const& b_name )
{ 
  size_t max_size = factorial(args.bells-1);

  int flags = 0; 
  if ( args.in_course ) {
    flags |= falseness_table::in_course_only;
    max_size /= 2;
  }
  else if ( args.out_of_course ) {
    flags |= falseness_table::out_of_course_only;
    max_size /= 2;
  }
  if ( args.half_leads ) 
    flags |= falseness_table::half_lead_only;

  falseness_table ft( a, b, flags );
  if ( args.print_falseness ) {
    copy( ft.begin(), ft.end(), ostream_iterator<row>(cout, "\n") );
    return string();
  }

  group sg( ft.generate_group() );
  if ( args.print_group ) {
    copy( sg.begin(), sg.end(), ostream_iterator<row>(cout, "\n") );
    return string();
  }

  if ( !args.null_splices && sg.size() == max_size )
    return string();

  if ( args.only_n_leads != -1 && sg.size() != args.only_n_leads * 2 &&
       // Need to store lead splices if we're to group them
       ( !args.group_splices && !args.filter_mode || sg.size() != 2 ) )
    return string();

  if ( args.only_n_leads != -1 && sg.size() != args.only_n_leads * 2 )
    return string();

  string desc( describe_splice(sg) );
  if ( !args.group_splices && !args.filter_mode ) {
    cout << name_or_pn(args, a);
    if ( args.meth.size() != 1 ) 
      cout << " / " << b_name;
    cout << "\t" << desc << "\n";
  }
  return desc;
}

bool splices::test_splice( method const& a, method const& b )
{
  // If we're grouping splices, this gets done later.  This is so that
  // we can output, e.g.  Bv,Su / Bk,He  for surprise minor lead splices.
  if ( !args.group_splices && args.same_le && a.back() != b.back() )
    return false;

  int flags = row_block::no_final_lead_head; 
  if ( args.half_leads ) 
    flags |= row_block::half_lead_only;

  row lh(args.bells);
  string desc = test_splice( a, row_block(b, lh, flags), name_or_pn(args, b) );

  if ( desc.size() && ( args.group_splices || args.filter_mode ) )
    save_splice( a, b, desc );

  return desc.size();
}

method splices::get_method( library_entry const& e )
{
  method m( e.meth() );

  // Use litelib payloads as a proxy for the name as there's no way
  // for a litelib to unambiguously carry a name.
  if ( e.has_facet< litelib::payload >() )
    m.name( e.get_facet< litelib::payload >() );

  return m;
}

void splices::find_splices( library const& lib )
{
  // The source library may not support restarting (e.g. if its 
  // a litelib on stdin), so load them into a methodset as we 
  // find them.
  methodset meths;
  meths.store_facet< litelib::payload >();

  typedef library::const_iterator const_iterator;
  for ( const_iterator i=lib.begin(), e=lib.end(); i!=e; ++i ) 
  {
    method m( get_method(*i) );

    bool filter_ok = false;

    if ( args.read_rows ) {
      test_splice( m, args.rows );
    }

    else if ( args.meth.size() == 1 ) {
      if ( m != args.meth.front() ) 
        filter_ok = test_splice( m, args.meth.front() );
    }

    else {
      for ( const_iterator j=meths.begin(), f=meths.end(); j!=f; ++j )
        test_splice( m, get_method(*j) );

      meths.push_back(*i);

      filter_ok = !has_lead_splices(i->meth());
    }

    if ( args.filter_mode && filter_ok )
      cout << i->meth().format( method::M_FULL_SYMMETRY | method::M_DASH )
           << "\t" << i->get_facet< litelib::payload >() << "\n";
  }

  if ( args.group_splices ) 
    print_splice_groups();
}

int main( int argc, char *argv[] )
{
  bell::set_symbols_from_env();

  arguments args(argc, argv);

  splices spl( args );

  if ( args.read_rows ) 
    args.rows.assign( istream_iterator<row>(cin), istream_iterator<row>() );

  if ( args.read_rows || args.meth.size() >= 2 ) {
    methodset in( args.meth.begin(), args.meth.end() );
    spl.find_splices( in );
  }
  else {
    litelib in( args.bells, cin );
    spl.find_splices( in );
  }
}
