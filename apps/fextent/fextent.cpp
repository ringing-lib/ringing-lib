// -*- C++ -*- fexent.cpp - search of maximal sets of mutually true leads
// Copyright (C) 2004 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/falseness.h>
#include <ringing/method.h>
#include <ringing/extent.h>
#include <ringing/multtab.h>
#include <ringing/pointers.h>
#include <ringing/streamutils.h>

#include <vector>
#include <cmath>
#include <iterator>
#include <iostream>
#include <cassert>

#include "args.h"
#include "init_val.h"

using namespace std;
using namespace ringing;

class arg_parser;

void status_out( string const& msg )
{
  static size_t status_size;
  const size_t trunc_limit(60);

  string m( msg );
  if ( m.size() > trunc_limit ) {
    m = m.substr( 0, trunc_limit );
    m.append("...");
  }
  
  cerr << "\r" << m;
  if ( m.size() < status_size ) 
    cerr << string( status_size - m.size(), ' ' );
  cerr << "\r" << flush;
  status_size = m.size();
}

inline bool clear_status() 
{
  status_out("");
}

struct arguments
{
  init_val<int,0>         bells;
  init_val<int, int(1E5)> num_steps;
  init_val<int,-1>        seed;

  init_val<bool,false>    whole_courses;
  init_val<bool,false>    tenors_together;
  init_val<bool,false>    in_course;
  init_val<bool,false>    principle;

  init_val<int,1>         loop;

  init_val<bool,false>    print_leads;
  init_val<int, 0>        min_leads;

  init_val<bool,false>    quiet;
  init_val<bool,false>    status;

  string                  meth_str;
  method                  meth;

  vector<string>          pend_strs;
  group                   pends;

  arguments( int argc, char** argv );

private:
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
  void generate_pends();
};

arguments::arguments( int argc, char** argv )
{
  arg_parser ap( argv[0], 
		 "fextent -- search for maximal sets of mutually true leads", 
		 "OPTIONS" );

  ap.add( new help_opt );
  ap.add( new version_opt );

  bind(ap);
  if ( !ap.parse(argc, argv) ) {
    ap.usage();
    exit(1);
  }

  if ( !validate(ap) ) 
    exit(1);
}

void arguments::generate_pends()
{
  vector<row> gens; 
  gens.reserve( pend_strs.size() );

  for ( vector<string>::const_iterator
	  i( pend_strs.begin() ), e( pend_strs.end() ); i != e; ++i ) 
    gens.push_back( *i );
  
  if ( gens.size() )
    pends = group( gens );
  else
    pends = group( row(bells) );
}

void arguments::bind( arg_parser& p )
{
  p.set_default( new string_opt( '\0', "", "", "", meth_str ) );

  p.add( new integer_opt
	 ( 'b', "bells",
	   "The default number of bells",  "BELLS",
	   bells ) );
  
  p.add( new integer_opt
	 ( 'n', "iterations",
	   "The number of iterations to perform",  "NUM",
	   num_steps ) );

  p.add( new boolean_opt
	 ( 'c', "whole-courses",
	   "Look for mutually true courses rather than leads",
	   whole_courses ) );

  p.add( new boolean_opt
	 ( 't', "tenors-together",
	   "Look for mutually true tenors-together leads (or courses)",
	   tenors_together ) );

  p.add( new boolean_opt
	 ( 'i', "in-course",
	   "Look for in-course lead-heads (or course-heads)",
	   in_course ) );

  p.add( new boolean_opt
	 ( 'p', "principle",
	   "Do not require a fixed treble",
	   principle ) );

  p.add( new integer_opt
	 ( 'l', "loop",
	   "Repeat some number of times (or indefinitely)", "NUM",
	   loop, -1 ) );

  p.add( new strings_opt
	 ( 'P', "part-end",
	   "Specify a part-end",  "ROW",
	   pend_strs ) );

  p.add( new integer_opt
	 ( '\0', "seed",
	   "Seed the random number generator",  "NUM",
	   seed ) );

  p.add( new boolean_opt
	 ( 'q', "quiet",
	   "Supress all output other than the maximum score",
	   quiet ) );

  p.add( new boolean_opt
	 ( '\0', "print-leads",
	   "Print the matching leads (or courses)",
	   print_leads ) );

  p.add( new integer_opt
	 ( '\0', "min-leads",
	   "Require at least this number of leads", "NUM",
	   min_leads ) );

}

bool arguments::validate( arg_parser& ap )
{
  if ( !bells ) {
    ap.error( "Must specify the number of bells" );
    return false;
  }

  if ( bells < 3 || bells >= int(bell::MAX_BELLS) ) {
    ap.error( make_string() << "The number of bells must be between 3 and " 
	      << bell::MAX_BELLS-1 << " (inclusive)" );
    return false;
  }

  try { 
    meth = method( meth_str, bells );
  } catch ( const exception& e ) {
    ap.error( make_string() << "Invalid method place notation: " 
	      << e.what() );
    return false;
  }

  if ( meth.empty() ) {
    ap.error( "Must specify a method" );
    return false;
  }

  if ( principle & whole_courses ) {
    ap.error( "Searching for principles in whole courses is not supported" );
    return false;
  }

  generate_pends();
  // TODO: Warning if partends generate the extent? 
  // Or anything equally silly?

  return true;
}

class state
{
public:
  enum 
  {
    in_course_only  = 0x01,
    tenors_together = 0x02,
    whole_courses   = 0x04,
    principle       = 0x08
  };

  state( const method& m, int flags, const group& pgrp );

  void set_beta(double b) { beta = b; }
  bool perturb(); // returns true if perturbation was kept

  int score() const {
    return sc * mt->group_size() * ( flags & whole_courses ? 7 : 1 ); 
  } 

  class const_iterator
  {
  public:
    typedef row value_type;
    typedef ptrdiff_t difference_type;
    typedef forward_iterator_tag iterator_category;
    typedef row const& reference;
    typedef row const* pointer;

    const_iterator& operator++() {
      ++idx; fixup();
    }

    const_iterator operator++(int) {
      const_iterator tmp(*this); ++*this; return tmp;
    }

    row operator*() const {
      assert( (*v)[idx] );
      return mt->find( multtab::row_t::from_index(idx) );
    }
    
    details::operator_arrow_proxy<row> operator->() const {
      return details::operator_arrow_proxy<row>( **this );
    }
    
    bool operator==( const_iterator const& other ) const {
      return idx == other.idx;
    }

    bool operator!=( const_iterator const& other ) const {
      return idx != other.idx;
    }
      
  private:
    void fixup() {
      while ( idx < v->size() && !(*v)[idx] ) ++idx;
    }

    friend class state;
    const_iterator( int idx, vector<bool> const* v, multtab const *mt ) 
      : idx(idx), v(v), mt(mt)
    {
      fixup();
    }

    // Data members
    int idx;
    vector<bool> const* const v;
    multtab const* const mt;
  };

  const_iterator begin_rows() const { 
    return const_iterator(0, &leads, mt.get()); 
  }

  const_iterator end_rows() const { 
    return const_iterator(leads.size(), &leads, mt.get());
  }

  bool check() const;
  
  void clear();

private:
  static inline bool random_bool( double ptrue = 0.5 ) {
    assert( ptrue <= 1 );
    return rand() < RAND_MAX * ptrue;
  }
  
  inline bool should_keep( int delta ) {
    return delta > 0 || random_bool( exp(delta * beta) );
  }
  
  typedef multtab::row_t      row_t;
  typedef multtab::post_col_t fch_t;
  
  const int bells;
  double beta;
  int sc;
  int flags;
  int nw, nh;
  scoped_pointer<multtab> mt;
  vector<fch_t> fchs;
  vector<bool> leads;
};

state::state( const method& m, int flags, const group& pgrp )
  : bells(bells), beta(0), flags(flags)
{
  nh = flags & principle ? 0 : 1;

  // Init nw
  if ( flags & whole_courses )
    nw = flags & tenors_together ? 5 : m.bells() - 1 - nh;
  else
    nw = m.bells() - nh;
  

  status_out( "Generating multiplication table ..." );

  // Init mt
  if ( flags & in_course_only )
    mt.reset( new multtab( incourse_extent_iterator(nw, nh, m.bells()),
			   incourse_extent_iterator(), pgrp ) );
  else
    mt.reset( new multtab( extent_iterator(nw, nh, m.bells()),
			   extent_iterator(), pgrp ) );
  
  // Init fchs
  if ( flags & whole_courses )
    {
      false_courses ft
	(m, ( flags & tenors_together ? false_courses::tenors_together : 0 )
	 |  ( flags & in_course_only  ? false_courses::in_course_only  : 0 ) );
      
      fchs.reserve( ft.size() );
      
      int n(0);
      for ( false_courses::const_iterator i( ft.begin() ), e( ft.end() );
	    i != e; ++i )
	if ( !i->isrounds() ) {
	  status_out( make_string() << "Calculating false course table "
		      "[" << ++n << "/" << ft.size() <<  "] ..." );

	  multtab::post_col_t f( mt->compute_post_mult(*i) );
	  if ( (multtab::row_t() * f).isrounds() )
	    throw runtime_error
	      ( "Error: the falseness conflicts with the part-end group" );
	  fchs.push_back( f );
	}
    }
  else 
    {
      assert( !( flags & tenors_together ) );
      
      status_out( "Calculating false lead table ..." );

      falseness_table ft
	( m, ( flags & in_course_only ? falseness_table::in_course_only : 0  ) 
	  |  ( flags & principle      ? falseness_table::no_fixed_treble : 0 ));

      fchs.reserve( ft.size() );
      
      int n(0);
      for ( falseness_table::const_iterator i( ft.begin() ), e( ft.end() );
	    i != e; ++i )
	if ( !i->isrounds() ) {
	  status_out( make_string() << "Calculating false lead table "
		      "[" << ++n << "/" << ft.size() <<  "] ..." );

	  multtab::post_col_t f( mt->compute_post_mult(*i) );
	  if ( (multtab::row_t() * f).isrounds() )
	    throw runtime_error
	      ( "Error: the falseness conflicts with the part-end group" );
	  fchs.push_back( f );
	}
    }

  clear_status();

  clear();
}

void state::clear()
{
  sc = 0;
  vector<bool>( mt->size(), false ).swap( leads );
}


bool state::check() const
{
  int realsc = 0;
  for ( int i=0; i<leads.size(); ++i )
    if ( leads[i] ) ++realsc;

  if ( realsc != sc ) {
    cerr << "ERROR: Score mismatch" << endl;
    return false;
  }

  if ( sc != distance( begin_rows(), end_rows() ) ) {
    cerr << "ERROR: Size mismatch" << endl;
    return false;
  }

  for ( vector<fch_t>::const_iterator fi( fchs.begin() ), fe( fchs.end() );
	fi != fe; ++fi ) 
    for ( multtab::row_iterator 
	    ri( mt->begin_rows() ), re( mt->end_rows() );
	  ri != re; ++ri ) 
      if ( leads[ ri->index() ] && leads[ (*ri * *fi).index() ] ) {
	cerr << "ERROR: Falseness" << endl;
	return false;
      }

  return true;
}

bool state::perturb()
{
  const double p_remove( 0.5 );

  int ri = random_int( leads.size() );

  if ( leads[ri] ) { // Remove it
    if ( should_keep( -1 ) ) {
      assert(leads[ri]);
      leads[ri] = false;
      --sc;
      //assert( check() );
      return true;
    }

    return false;
  }
  else { // Add it
    vector< row_t > f; // false leads

    for ( vector<fch_t>::const_iterator fi(fchs.begin()), fe(fchs.end());
	  fi != fe; ++fi )
      {
	row_t r( row_t::from_index(ri) * *fi );
	if ( r.index() == ri )
	  return false;
	if ( leads[r.index()] ) 
	  if ( find( f.begin(), f.end(), r ) == f.end() )
	    f.push_back(r);
      }

    if ( should_keep( 1 - f.size() ) ) {
      for ( vector<row_t>::const_iterator i(f.begin()), e(f.end());
	    i != e; ++i ) {
	assert(leads[i->index()]);
	leads[i->index()] = false;
      }
      assert( !leads[ri] );
      leads[ri] = true;
      sc += 1 - f.size();
      //assert( check() );
      return true;
    }

    return false;
  }
}

row select_lead_head( row r ) 
{
  const row lh( row::pblh(r.bells()) );
  while ( r[r.bells() - 1] != r.bells() - 1 )
    r = lh * r;
  return r;
}

int main( int argc, char* argv[] )
{
  try {
    arguments args( argc, argv );

    scoped_pointer< state > s;

    try {
      int stflags(0);
      if ( args.in_course )       stflags |= state::in_course_only;
      if ( args.whole_courses )   stflags |= state::whole_courses;
      if ( args.tenors_together ) stflags |= state::tenors_together;
      if ( args.principle )       stflags |= state::principle;
      
      s.reset( new state( args.meth, stflags, args.pends ) );
      clear_status();
    }
    catch ( exception const& ex ) {
      clear_status();
      cerr << "Error initialising state: " << ex.what() << endl;
      exit(1);
    }

    int mx = 0;
    
    if ( args.seed == -1 )
      srand( args.seed = time(NULL) );
    else if ( args.seed )
      srand( args.seed );
    
    if ( args.seed && !args.quiet )
      cout << "Started with seed " << int(args.seed) << endl;
    
  
    for ( int i=0; args.loop == -1 || i < args.loop; ++i ) {
      
      const double beta_init  = 3;
      const double beta_final = 25;

      
      const double beta_mult
	= pow( beta_final / beta_init, 1/double(args.num_steps) );
      
      int nw = 4;
      
      for ( double beta = beta_init ; beta < beta_final; beta *= beta_mult ) {
	s->set_beta(beta);
	s->perturb();
      }

      if (!s->check()) { 
	cerr << "ERROR!!!" << endl;
	transform( s->begin_rows(), s->end_rows(), 
		   ostream_iterator<row>( cerr, "\n" ),
		   select_lead_head );
	exit(1);
      }

      if ( s->score() > mx ) mx = s->score();
      
      if ( !args.quiet ) {
	cout << s->score();
	if ( args.loop != 1 )
	  cout << " [highest = " << mx << "]";
	cout << endl;
      }

      if ( args.print_leads && s->score() >= args.min_leads ) {
	transform( s->begin_rows(), s->end_rows(), 
		   ostream_iterator<row>( cout, "\n" ),
		   select_lead_head );
	cout << "\n\n\n" << endl;
      }

      s->clear();
    }

    if ( args.quiet )
      cout << mx << endl;

    return 0;
  }
  catch (exception const& ex) {
    cerr << ex.what() << endl;
    exit(1);
  }
}
