// -*- C++ -*- fexent.cpp - search of maximal sets of mutually true leads
// Copyright (C) 2004, 2005 Richard Smith <richard@ex-parrot.com>

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
#include <iomanip>
#include <cassert>

#include "args.h"
#include "init_val.h"

#if 0
#define DEBUG( expr ) (void)((cout << expr) << endl)
#else 
#define DEBUG( expr ) (void)(false)
#endif

// The checks controlled by this #define are quite expensive
// but probably worth leaving in to catch any false touches
// if and when they are generated.
#define ENABLE_CHECKS 1

using namespace std;
using namespace ringing;

class arg_parser;

template <class T>
static int sign( T i )
{
  if ( i > 0 ) return +1;
  else if ( i < 0 ) return -1;
  else return 0;
}

static bool is_tt( row const& r, int nw = 6 )
{
  for ( int b=nw, n=r.bells(); b<n; ++b )
    if ( r[b] != b ) 
      return false;
  return true;
}

static bool are_tenors_over( row const& r )
{
  const int b = r.bells();
  return r[b-1] == b-1 && r[b-3] == b-2;
}

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

struct weighting 
{
  weighting();

  double base;
  double linked_course;

  double in_course;
  double out_of_course;
  double tenors_together; // together in usual orientation
  double tenors_over;
};

weighting::weighting()
  : base(1.0),
    linked_course(1.0),
    in_course(0.0),
    out_of_course(0.0),
    tenors_together(0.0),
    tenors_over(0.0)
{
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

  vector<string>          required_strs;
  vector<row>             required;

  init_val<bool,false>    linkage;
  vector<string>          call_strs;
  vector<change>          calls;
 
  weighting               wprof;

  arguments( int argc, char** argv );

private:
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
  bool generate_pends( arg_parser& ap );
  bool generate_required( arg_parser& ap );
  bool generate_calls( arg_parser& ap );
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

bool arguments::generate_pends( arg_parser& ap )
{
  vector<row> gens; 
  gens.reserve( pend_strs.size() );

  for ( vector<string>::const_iterator
	  i( pend_strs.begin() ), e( pend_strs.end() ); i != e; ++i )  
    {
      row const g( row(bells) * row(*i) );

      // This is hopelessly broken.  Let's just admit it and bail out.
      if ( linkage && whole_courses && g[bells-1] != bells-1 ) {
	ap.error( "Part end groups affecting the tenor are not supported " 
		  "for linked touches in whole courses" );
	return false;
      }

      gens.push_back(g);
    }
  
  if ( gens.size() )
    pends = group( gens );
  else
    pends = group( row(bells) );

  // TODO: Warning if partends generate the extent? 
  // Or anything equally silly?

  return true;
}


bool arguments::generate_required( arg_parser& ap )
{
  for ( vector<string>::const_iterator
	  i( required_strs.begin() ), e( required_strs.end() ); i != e; ++i ) {
    row const r( row(bells) * row(*i) );
    if ( in_course && r.sign() == -1 ) {
      ap.error( make_string() << "Out-of-course lead '"
		<< r << "' required in in-course composition" );
      return false;
    }
    required.push_back(r);
  }
  return true;
}

bool arguments::generate_calls( arg_parser& ap )
{
  // These are only warnings -- so don't return false
  if ( call_strs.empty() && linkage ) {
    call_strs.push_back( "14" );
    ap.error( "No calls specified -- assuming fourths place bobs");
  }
  else if ( !call_strs.empty() && !linkage ) {
    call_strs.clear();
    ap.error( "Calls ignored when linkage is not required" );
  }
  
  for ( vector<string>::const_iterator 
	  i( call_strs.begin() ), e( call_strs.end() ); i != e; ++i ) 
    try {
      change const ch( bells, *i );
      if (ch.sign() != meth.back().sign() && in_course) {
	ap.error( make_string() << "Out-of-course call '"
		  << ch << "' specified for in-course composition" );
	return false;
      }
      calls.push_back( ch );
    } 
    catch ( exception const& ex ) {
      ap.error( make_string() << "Unable to parse change '" 
		<< *i << "': " << ex.what() );
      return false;
    }

  return true;
}

class weighting_opt : public option {
public:
  weighting_opt( char c, const string& l, const string& d, 
		 const string& a, weighting& w )
    : option(c, l, d, a),  w(w) 
  {}

private:
  virtual bool process( const string &, const arg_parser & ) const;
  weighting& w;
};

bool weighting_opt::process( const string& arg, const arg_parser& ap ) const
{
  string::size_type pos( arg.find('=') );
  if ( pos == string::npos ) {
    ap.error( "Weighting must be specified as OPTION=WEIGHT" );
    return false;
  }

  double v;
  try {
    v = lexical_cast<double>( arg.substr( pos + 1 ) );
  }
  catch ( bad_lexical_cast const& ) {
    ap.error( make_string() << "Invalid weighting: '" << arg.substr(pos+1) << "'" );
    return false;
  }

  string opt( arg, 0, pos );

  if      ( opt == "i" || opt == "in-course" )
    w.in_course = v;
  else if ( opt == "o" || opt == "out-of-course" )
    w.out_of_course = v;
  else if ( opt == "t" || opt == "tenors-together" )
    w.tenors_together = v;
  else if ( opt == "f" || opt == "tenors-over" )
    w.tenors_over = v;
  else if ( opt == "k" || opt == "linked" )
    w.linked_course = v;
  else if ( opt == "b" || opt == "base" )
    w.base = v;
  else {
    ap.error( make_string() << "Unknown option: '" << opt << "'" );
    return false;
  }

  return true;
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

  p.add( new strings_opt
	 ( 'r', "required",
	   "Require certain leads (or courses)",  "ROW",
	   required_strs ) );

  p.add( new boolean_opt
	 ( 'k', "linkage",
	   "Require linkage between leads (or courses)",
	   linkage ) );

  p.add( new strings_opt
	 ( 'C', "call",
	   "Specify a call",  "CHANGE",
	   call_strs ) );

  p.add( new integer_opt
	 ( '\0', "seed",
	   "Seed the random number generator",  "NUM",
	   seed ) );

  p.add( new boolean_opt
	 ( 'q', "quiet",
	   "Supress all output other than the maximum score",
	   quiet ) );

  p.add( new boolean_opt
	 ( 'u', "quiet",
	   "Display the current status",
	   status ) );

  p.add( new boolean_opt
	 ( '\0', "print-leads",
	   "Print the matching leads (or courses)",
	   print_leads ) );

  p.add( new integer_opt
	 ( '\0', "min-leads",
	   "Require at least this number of leads", "NUM",
	   min_leads ) );

  p.add( new weighting_opt
	 ( 'W', "weighting",
	   "Specify a weighting", "OPTION=WEIGHT",
	   wprof ) );
}

bool arguments::validate( arg_parser& ap )
{
  if ( !bells ) {
    ap.error( "Must specify the number of bells" );
    return false;
  }

  if ( bells < 4 || bells >= int(bell::MAX_BELLS) ) {
    ap.error( make_string() << "The number of bells must be between 4 and " 
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

  if ( !generate_pends( ap ) ) 
    return false;

  if ( !generate_required( ap ) )
    return false;

  if ( !generate_calls( ap ) )
    return false;

  if ( linkage )
    ap.error( "Warning:  Linkage handling is experimental." );

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

  state( const method& m, int flags, const group& pgrp,
	 vector<row> const& required_rows, vector<change> const& calls,
	 weighting const& wprof );

  void set_beta(double b) { beta = b; }
  bool perturb(); // returns true if perturbation was kept

  void prune_unlinked();

  int length() const {
    return len * mt->group_size() * ( flags & whole_courses ? courselen : 1 );
  } 

  bool fully_linked() const { 
    return links == len;
  }

  double percent_linked() const {
    return 100 * (double)links / len;
  }
 
  void dump( ostream& os ) const;

private:
  enum lead_state {
    disallowed = -1,
    absent = false,
    present, 
    required
  };

public:
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
      assert( is_present( (*v)[idx] ) );
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
      while ( idx < v->size() && is_absent( (*v)[idx] ) ) ++idx;
    }

    friend class state;
    const_iterator( int idx, vector<lead_state> const* v, multtab const *mt ) 
      : idx(idx), v(v), mt(mt)
    {
      fixup();
    }

    // Data members
    int idx;
    vector<lead_state> const* const v;
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
  void init_mt( method const& m, group const& pgrp, weighting const& wprof );
  void init_fchs( method const& m );
  void init_flhs( method const& m );
  void init_req( method const& m, vector<row> const& required_rows );
  void init_qsets( method const& m, vector<change> const& calls );
  void init_lhs( method const& m, vector<change> const& calls );

  struct perturbation;
  
  inline bool should_keep( double delta ) {
    return delta > 0 || random_bool( exp(delta * beta) );
  }

  typedef multtab::row_t      row_t;
  typedef multtab::post_col_t fch_t;

  static bool is_present( lead_state s ) { return s == present || s == required; }
  static bool is_absent( lead_state s ) { return s == disallowed || s == absent; }

  bool is_present( row_t r ) const { return is_present( leads[r.index()] ); }
  bool is_absent( row_t r ) const { return is_absent( leads[r.index()] ); }

  size_t check_qsets( row_t r ) const;
  size_t check_lhs( row_t r ) const;
 
 
  const int bells, courselen;
  const double link_weight;

  double beta;
  double sc;
  int len, links;
  int flags;
  int nw, nh;
  scoped_pointer<multtab> mt;
  vector<double> weight;
  vector<fch_t> fchs;
  vector<row_t> req_rows;
  vector< pair< vector< pair< fch_t, 
			      size_t /*inverse qset index*/> >, 
                size_t /*change_index*/> > qsets;
  vector< pair< fch_t, fch_t > > lhs; // forward & reverse
  vector<lead_state> leads;
  vector<size_t> linkage; // if qsets.size(), indices into the qsets vector, or size_t(-1)
                          // else if lhs.size(), indices into the lhs vector
};

void state::dump( ostream& os ) const
{
  for ( int i=0; i<leads.size(); ++i )
    {
      row_t const r( row_t::from_index(i) );
      if ( is_present(r) )
	{
	  row const rr( mt->find(r) );
	  os << setw(3) << i << ": " << rr
	     << ( rr.sign() > 0 ? '+' : '-' ) << ' ';

	  if ( qsets.size() ) 
	    for ( size_t qidx(0); qidx != qsets.size(); ++qidx )
	      {
		size_t count(0);
		make_string desc;
		desc << " Q" << qidx << " (" << i;

		for ( size_t i(0), n(qsets[qidx].first.size()); 
		      i != n && count == i; ++i )
		  {
		    row_t q = r * qsets[qidx].first[i].first;
		    if (q != r && is_present(q) ) {
		      ++count;
		      desc << " " << q.index();
		    }
		  }
		desc << ")";

		if ( count == qsets[qidx].first.size() ) 
		  os << string(desc);
	      }
	  else 
	    for ( size_t i(0); i != lhs.size(); ++i )
	      {
		row_t p = r * lhs[i].first;
		if (p != r && is_present(p) )
		  os << " " << p.index();
	      }

	  os << "\n";
	}
    }
}

void state::init_mt( method const& m, group const& pgrp, 
		     weighting const& wprof )
{
  status_out( "Generating multiplication table ..." );

  group postgroup;
  if ( flags & whole_courses )
    postgroup = group( m.lh() );

  if ( flags & in_course_only )
    mt.reset( new multtab( incourse_extent_iterator(nw, nh, bells),
			   incourse_extent_iterator(), pgrp, postgroup ) );
  else
    mt.reset( new multtab( extent_iterator(nw, nh, bells),
			   extent_iterator(), pgrp, postgroup ) );

  assert( mt->size() * pgrp.size() 
	  == factorial(nw) / ( (flags & in_course_only) ? 2 : 1 ) );

  vector<double>( mt->size(), wprof.base ).swap( weight );

  for ( int i=0; i<mt->size(); ++i )
    {
      row const r( mt->find( row_t::from_index(i) ) );
      
      if ( r.sign() == +1     ) weight[i] += wprof.in_course;
      if ( r.sign() == -1     ) weight[i] += wprof.out_of_course;
      if ( is_tt(r)           ) weight[i] += wprof.tenors_together;
      if ( are_tenors_over(r) ) weight[i] += wprof.tenors_over;
    }

  clear_status();
}

void state::init_fchs( method const& m )
{
  assert( flags & whole_courses );

  status_out( "Calculating false course table ..." );

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
      if ( (multtab::row_t() * f).isrounds() ) {
	throw runtime_error
	  ( "Error: the falseness conflicts with the part-end group" );
      }
      fchs.push_back( f );
    }

  clear_status();
}

void state::init_flhs( method const& m )
{
  assert( !( flags & whole_courses ) );
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

  clear_status();
}

void state::init_req( const method& m, vector<row> const& required_rows )
{
  int n(0);
  for ( vector<row>::const_iterator 
	  i(required_rows.begin()), e(required_rows.end());
	i != e; ++i ) 
    {
      status_out( make_string() << "Calculating required and forbidden "
		  << ( flags & whole_courses ? "courses " : "rows " )
		  << "[" << ++n << "/" << required_rows.size() <<  "] ..." );

      row_t r;
      {
	row const lh( m.lh() );
	row r2(*i);
	while ( r2[bells - 1] != bells - 1 )
	  r2 = lh * r2;
	r = mt->find(r2);
      }

      if ( find( req_rows.begin(), req_rows.end(), r ) != req_rows.end() )
	throw runtime_error
	  ( "Error: the required rows conflict with the part-end group" );
      
      for ( vector<fch_t>::const_iterator fi( fchs.begin() ), fe( fchs.end() );
	    fi != fe; ++fi ) {
	row_t f( r * *fi );
	if ( find( req_rows.begin(), req_rows.end(), f ) != req_rows.end() )
	  throw runtime_error
	    ( "Error: the required rows conflict with the falseness" );
      }

      req_rows.push_back(r);
    }

  clear_status();
}

void state::init_lhs( method const& m, vector<change> const& calls )
{
  assert( !( flags & whole_courses ) );

  lhs.push_back( make_pair( mt->compute_post_mult( m.lh() ),
			    mt->compute_post_mult( m.lh().inverse() ) ) );

  row const le( m.lh() * m.back() );

  for ( size_t ci(0); ci != calls.size(); ++ci ) 
    {
      row const lh( le * calls[ci] );
      lhs.push_back( make_pair( mt->compute_post_mult( lh ),
				mt->compute_post_mult( lh.inverse() ) ) );
    }
}

void state::init_qsets( method const& m, vector<change> const& calls )
{
  assert( flags & whole_courses );

  group const lhg( m.lh() );
  vector< vector<fch_t> > qinverses;

  int n(0);
  for ( size_t ci(0); ci != calls.size(); ++ci )
    {
      status_out( make_string() << "Calculating Q-sets "
		  "[" << ++n << "/" << calls.size() <<  "] ..." );

      group const qproto( row() * m.back() * calls[ci] );

      group::const_iterator const le( lhg.end() );
      for ( group::const_iterator li( lhg.begin() ); li != le; ++li ) 
	{
	  vector< pair<fch_t, size_t> > qset;
	  vector<fch_t> qinverse;
	  
	  make_string desc;
	  desc << "Q" << qsets.size() << " (call " << calls[ci] << "): ";

	  // Generate the Q-set's course heads
	  //
	  // This is subtle because the course-heads corresponding
	  // to a Q-set affecting the tenor are no longer a group 
	  // conjugate to the Q-set (or even a group at all).
	  for ( group::const_iterator qi( qproto.begin() ), qe( qproto.end() );
		qi != qe; ++qi )
	    {
	      if ( qi->isrounds() ) 
		continue;

	      for ( group::const_iterator lj( lhg.begin() ); lj != le; ++lj )
		{
		  const row q( *li * *qi * *lj );
		  
		  if ( q[bells-1] != bells-1 ) continue;
		  if ( (flags & tenors_together) && !is_tt(q) ) continue;

		  // We'll calculate the inverse at some point anyway
		  // so this does not add to the size of mt.
		  qinverse.push_back( mt->compute_post_mult( q.inverse() ) );

		  qset.push_back
		    ( make_pair( mt->compute_post_mult(q), size_t(-1) ) );

		  desc << " " << q;

		  break;
		}
	    }

	  bool ok( qset.size() == qproto.size()-1 );

	  // Check that the Q-set does not intersect the FCHs.
	  for ( vector< pair<fch_t, size_t> >::const_iterator 
		  qi( qset.begin() ), qe( qset.end() );
		ok && qi != qe; ++qi )
	    for ( vector<fch_t>::const_iterator 
		    fi( fchs.begin() ), fe( fchs.end() );
		  ok && fi != fe; ++fi )
	      if ( *fi == qi->first )
		ok = false;

	  if (ok) {
	    cout << string(desc) << endl;
	    qsets.push_back( make_pair( qset, ci ) );
	    qinverses.push_back( qinverse );
	  }
	}
    }

  {
    // Set up the inverse Q-set links 
    //
    size_t const n( qsets.size() );
    assert( qinverses.size() == n );

    for ( size_t i(0); i != n; ++i ) 
      {
	group const qproto( row() * m.back() * calls[qsets[i].second] );

	size_t const m( qproto.size() - 1 );
	assert( m == qsets[i].first.size() );

	for ( size_t j(0); j != m; ++j )
	  {
	    // Locate the inverse element in the Q-set
	    size_t jinv;
	    {
	      group::const_iterator gi( qproto.begin() ); 
	      advance( gi, j+1 );
	      gi = find( qproto.begin(), qproto.end(), gi->inverse() );
	      jinv = distance( qproto.begin(), gi ) - 1;
	    }

	    bool found(false);

	    for ( size_t k(0); !found && k != n; ++k )
	      if ( qsets[i].second == qsets[k].second ) 
		{
		  assert( qsets[k].first.size() == m );
		  if ( qsets[i].first[j].first == qinverses[k][jinv] )
		    {  
		      qsets[i].first[j].second = k;
		      found = true;
		    }
		}
	    assert(found);
	  }
      }
  }

  clear_status();

  if ( qsets.empty() )
    throw runtime_error
      ( "Error: the given calls conflict with the falseness" );
}

state::state( const method& m, int flags, const group& pgrp, 
	      const vector<row>& required_rows, const vector<change>& calls,
	      const weighting& wprof )
  : bells(m.bells()), courselen(m.leads()), 
    link_weight( wprof.linked_course ),
    beta(0), flags(flags)
{
  nh = flags & principle ? 0 : 1;

  if ( flags & whole_courses )
    nw = flags & tenors_together ? 5 : m.bells() - 1 - nh;
  else
    nw = m.bells() - nh;
  
  init_mt( m, pgrp, wprof );
 
  if ( flags & whole_courses )
    init_fchs( m );
  else
    init_flhs( m );

  if ( required_rows.size() )
    init_req( m, required_rows );

  if ( calls.size() )
    {
      if ( flags & whole_courses )
	init_qsets( m, calls );
      else
	init_lhs( m, calls );
    }

  clear();
}

size_t state::check_lhs( row_t r ) const 
{
  for ( size_t i(0); i != lhs.size(); ++i )
    {
      row_t const p( r * lhs[i].first );

      if ( r != p && is_present(p) )
	return i;
    }

  return size_t(-1);
}

size_t state::check_qsets( row_t r ) const
{
  for ( size_t qi(0); qi != qsets.size(); ++qi )
    {
      size_t count(0), n(qsets[qi].first.size());

      for ( size_t i(0); i != n && count == i; ++i )
	{
	  row_t const q( r * qsets[qi].first[i].first );

	  // Handle Q-sets that are partially self-Q-sets
	  // That is, Q-sets where some, but not all, of the elements 
	  // correspond to the same course (modulo the part end group).
	  // This can happen with Q-sets that affect the tenors.
	  bool self( q == r );
	  for ( size_t j(0); !self && j != i; ++j )
	    if ( r * qsets[qi].first[j].first == q )
	      self = true;

	  if (self) 
	    ; // exclude "self" Q-sets
	  else if ( is_present(q) )
	    ++count;
	}

      if ( count == n ) 
	return qi;
    }
  return size_t(-1);
}



bool state::check() const
{
  double realsc = 0;
  int linkage = 0;
  for ( int i=0; i<leads.size(); ++i ) 
    if ( is_present( leads[i] ) ) {
      realsc += weight[i];
      if ( qsets.size() && check_qsets( row_t::from_index(i) ) != size_t(-1) ||
	   lhs.size()   && check_lhs  ( row_t::from_index(i) ) != size_t(-1) )
	realsc += link_weight, ++linkage;
    }

  if ( linkage != links ) {
    cerr << "ERROR: Linkage mismatch: actual " << linkage << "; expected " << links << endl;
    return false;
  }

  if ( realsc != sc ) {
    cerr << "ERROR: Score mismatch: actual " << realsc << "; expected " << sc << endl;
    return false;
  }

  if ( distance( begin_rows(), end_rows() ) != len ) {
    cerr << "ERROR: Length mismatch" << endl;
    return false;
  }
  

  for ( vector<fch_t>::const_iterator fi( fchs.begin() ), fe( fchs.end() );
	fi != fe; ++fi ) 
    for ( multtab::row_iterator 
	    ri( mt->begin_rows() ), re( mt->end_rows() );
	  ri != re; ++ri ) 
      {
	row_t f( *ri * *fi );
	if ( *ri == f && is_present(f) ) {
	  cerr << "ERROR: Internally false lead" << endl;
	  return false;
	}
	else if ( is_present(*ri) && is_present( *ri * *fi ) ) {
	  cerr << "ERROR: Falseness" << endl;
	  return false;
	}
      }

  if ( fully_linked() && linkage != len ) {
    cerr << "ERROR: Linkage count mismatch" << endl;
    return false;
  }

  return true;
}

class state::perturbation
{
public:
  explicit perturbation( state const& s );

  bool add_row   ( row_t const& r );
  bool remove_row( row_t const& r, lead_state new_state = absent );

  // Returns true if any pruning occurred
  bool prune_unlinked( row_t const& r );  

  double delta() const { 
#if ENABLE_CHECKS
    assert(check_delta()); 
#endif
    return d; 
  }

  void commit( state& );
  void commit_permanently( state& );
  

private:
  void commit2( state& s, lead_state add, lead_state rm );

  bool check_delta() const;

  //
  // Row status
  //
  bool is_added( row_t const& r ) const
    { map<row_t, double>::const_iterator i = rdiff.find(r);
      return i != rdiff.end() && i->second > 0; }
  bool is_removed( row_t const& r ) const 
    { map<row_t, double>::const_iterator i = rdiff.find(r);
      return i != rdiff.end() && i->second < 0; }

  // TODO:  It is bad to assume that  (double(w) - double(w) == 0.0)

  void do_add_row( row_t const& r )
    { double const w( s.weight[ r.index() ] ); 
      d += w; if ( (rdiff[r] += w) == 0 ) rdiff.erase(r); }
  void do_rm_row( row_t const& r )
    { double const w( s.weight[ r.index() ] ); 
      d -= w; if ( (rdiff[r] -= w) == 0 ) rdiff.erase(r); }

  //
  // General linkage
  //
  size_t get_linkage( row_t const& r ) const;
  void do_add_link( row_t const& r, size_t link );
  void do_rm_link( row_t const& r );

  //
  // Q-set linkage
  //
  void remove_qset( row_t const& r );
  void try_add_qset( row_t const& r );

  //
  // Lead linkage
  //
  void remove_lh( row_t const& r );
  void try_add_lh( row_t const& r );
  
  //
  // Data members
  //
  state const& s;

  map<row_t, double, row_t::cmp> rdiff; // +1 to add, -1 to remove
  map<row_t, pair<int, size_t>, row_t::cmp> ldiff;
  double d;
};

inline size_t state::perturbation::get_linkage( row_t const& r ) const
{
  map<row_t, pair<int,size_t> >::const_iterator i = ldiff.find(r);
  return i == ldiff.end() ? s.linkage[r.index()] : i->second.second; 
}

inline void state::perturbation::do_add_link( row_t const& r, size_t link )
{ 
  pair<int, size_t>& x = ldiff[r];

  DEBUG( "Actually adding link " << r.index() << ": " << link );

  d += s.link_weight; 
  x.first++; 
  x.second = link; 
}

inline void state::perturbation::do_rm_link( row_t const& r )
{ 
  pair<int, size_t>& x = ldiff[r];

  DEBUG( "Actually removing link " << r.index() << ": " << get_linkage(r) );

  d -= s.link_weight; 
  if ( --x.first == 0 ) 
    ldiff.erase(r); 
  else 
    x.second = size_t(-1); 
}


state::perturbation::perturbation( state const& s )
  : s(s), d(0)
{
  DEBUG( "" );
  DEBUG( "New perturbation -- initial score " << s.sc << "; " 
	 << "links " << s.links );
}

void state::clear()
{
  sc = len = links = 0;
  vector<lead_state>( mt->size(), absent ).swap( leads );

  if ( qsets.size() || lhs.size() )
    vector<size_t>( mt->size(), size_t(-1) ).swap( linkage );

  perturbation init(*this);

  // Find leads that are false against themselves (perhaps in different parts)
  for ( vector<fch_t>::const_iterator fi( fchs.begin() ), fe( fchs.end() );
	fi != fe; ++fi ) 
    for ( multtab::row_iterator 
	    ri( mt->begin_rows() ), re( mt->end_rows() );
	  ri != re; ++ri ) 
      {
	row_t f( *ri * *fi );
	if ( *ri == f ) {
	  DEBUG( "Internally false row " << f.index() );
	  bool const rv = init.remove_row(f, disallowed);
	  assert(rv); (void)(rv); // avoid warning with _NDEBUG
	}
      }

  // Locate required rows and rows false against them
  for ( vector<row_t>::const_iterator i(req_rows.begin()), e(req_rows.end());
	i != e; ++i )
    if ( !init.add_row(*i) ) 
      throw runtime_error( "Error: the required rows are mutually false" );
  
  init.commit_permanently(*this);
}

bool state::perturbation::check_delta() const
{
  double real_delta(0);

  for ( map<row_t, double>::const_iterator 
	  i(rdiff.begin()), e(rdiff.end());  i != e;  ++i )
    real_delta += i->second;

  for ( map< row_t, pair<int, size_t> >::const_iterator 
	  i(ldiff.begin()), e(ldiff.end());  i != e;  ++i )
    real_delta += i->second.first * s.link_weight;

  return real_delta == d;
}

void state::perturbation::try_add_qset( row_t const& r )
{
  if ( ( s.is_present(r) && !is_removed(r) ||
	 s.is_absent (r) &&  is_added  (r) ) &&
       get_linkage(r) == size_t(-1) )
    {
      for ( size_t qi(0); qi != s.qsets.size(); ++qi )
	{
	  size_t count(0), n(s.qsets[qi].first.size());

	  for ( size_t i(0); i != n && count == i; ++i )
	    {
	      row_t const q( r * s.qsets[qi].first[i].first );
	      
	      bool self( q == r );
	      for ( size_t j(0); !self && j != i; ++j )
		if ( r * s.qsets[qi].first[j].first == q )
		  self = true;

	      if (self) 
		; // exclude "self" Q-sets
	      else if ( s.is_present(q) && !is_removed(q) ||
			s.is_absent(q) && is_added(q) )
		++count;
	    }

	  if ( count == n )
	    {
	      DEBUG( "Found link to add to " << r.index() << ": " << qi );

	      if ( get_linkage(r) == size_t(-1) )
		do_add_link(r, qi);

	      for ( size_t i(0); i != n; ++i ) 
		{
		  row_t const q = r * s.qsets[qi].first[i].first;

		  if ( get_linkage(q) == size_t(-1) )
		    do_add_link( q, s.qsets[qi].first[i].second );
		}
	    }
	}
    }
}

void state::perturbation::remove_qset( row_t const& r )
{
  if ( get_linkage(r) != size_t(-1) )
    {
      DEBUG( "Remove Q-set link " << r.index() );

      do_rm_link(r);

      // Remove links elsewhere
      for ( size_t i=0, n=s.qsets.size(); i != n; ++i ) 
	for ( size_t j=0, m=s.qsets[i].first.size(); j != m; ++j )
	  {
	    row_t const q( r * s.qsets[i].first[j].first );

	    if ( get_linkage(q) == s.qsets[i].first[j].second )
	      do_rm_link(q);
	  }

      // Fix up new links elsewhere in the Q-set if possible
      for ( size_t i=0, n=s.qsets.size(); i != n; ++i ) 
	for ( size_t j=0, m=s.qsets[i].first.size(); j != m; ++j )
	  try_add_qset( r * s.qsets[i].first[j].first );
    }
}

void state::perturbation::remove_lh( row_t const& r )
{
  DEBUG( "Remove link " << r.index() );

  if ( get_linkage(r) != size_t(-1) )
    do_rm_link(r);

  // Remove links elsewhere
  for ( size_t i=0, n=s.lhs.size(); i != n; ++i )
    {
      row_t const p( r * s.lhs[i].second );
      
      if ( get_linkage(p) == i ) {
	do_rm_link(p);
	try_add_lh(p);
      }
    }
}

void state::perturbation::try_add_lh( row_t const& r )
{
  if ( ( s.is_present(r) && !is_removed(r) ||
	 s.is_absent (r) &&  is_added  (r) ) &&
       get_linkage(r) == size_t(-1) )
    {
      DEBUG( "Attempting to add linkage from " << r.index() );

      for ( size_t i=0, n=s.lhs.size(); i != n; ++i )
	{
	  row_t const p( r * s.lhs[i].first );

	  if ( p == r )
	    ;
	  else if ( s.is_present(p) && !is_removed(p) ||
		    s.is_absent(p) && is_added(p) )
	    {
	      do_add_link(r, i);
	      break;
	    }
	}
    }
  DEBUG( "Attempting to add linkage to " << r.index() );

  // preceeding
  // TODO -- suboptimal.  Only needs to be done if called from add_row,
  // not if called from remove_link
  for ( size_t i=0, n=s.lhs.size(); i != n; ++i )
    {
      row_t const p( r * s.lhs[i].second );

      if ( p == r )
	;
      else if ( ( s.is_present(p) && !is_removed(p) ||
		  s.is_absent (p) &&  is_added  (p) ) &&
		get_linkage(p) == size_t(-1) )
	do_add_link(p, i);
    }
}

bool state::perturbation::remove_row( row_t const& r, lead_state new_state )
{
  if ( s.is_present(r) && !is_removed(r) ||
       s.is_absent (r) &&  is_added  (r) ||
       new_state == disallowed )
    {
      DEBUG( "Remove row " << r.index() << ", score " << s.weight[r.index()] );

      if ( s.leads[r.index()] == required )
	return false;

      do_rm_row(r);
  
      if ( s.qsets.size() )
	remove_qset(r);
      else if ( s.lhs.size() ) 
	remove_lh(r);
    }
  return true;
}

bool state::perturbation::add_row( row_t const& r )
{
  if ( s.leads[r.index()] == disallowed )
    return false;

  if ( s.is_absent(r)  && !is_added(r) ||
       s.is_present(r) &&  is_removed(r) )
    {
      DEBUG( "Add row " << r.index() << ", "
	     "score " << s.weight[r.index()] << ", "
	     "lead_state " << (int)s.leads[r.index()] );

      // Remove false rows
      for ( vector<fch_t>::const_iterator fi(s.fchs.begin()), fe(s.fchs.end());
	    fi != fe; ++fi )
	if ( !remove_row(r * *fi) )
	  return false;

      do_add_row(r);

      if ( s.qsets.size() )
	try_add_qset(r);
      else if ( s.lhs.size() )
	try_add_lh(r);
    }
  return true;
}

void state::perturbation::commit2( state& s, lead_state add, lead_state rm )
{
  DEBUG( "Committing -- delta " << d );

  for ( map<row_t, double>::const_iterator 
	  i(rdiff.begin()), e(rdiff.end());  i != e;  ++i )
    {
      DEBUG( "Commit row: " << i->first.index() << ", score " << i->second );
      assert( i->second != 0 );

      if ( i->second > 0 && s.is_absent(i->first) ||
	   i->second < 0 && s.is_present(i->first) )
	{
	  // TODO Gah! More assertions on floating equalities.  Bad, bad, bad.
	  assert( s.weight[ i->first.index() ] == i->second * sign(i->second) );
	  s.sc += i->second;
	  s.len += sign(i->second);
	}

      s.leads[ i->first.index() ] = ( (i->second > 0) ? add : rm );
    }

  for ( map< row_t, pair<int, size_t> >::const_iterator 
	  i(ldiff.begin()), e(ldiff.end());  i != e;  ++i )
    {
      DEBUG( "Commit link: " << i->first.index() << ", score " << i->second.first );

      s.linkage[ i->first.index() ] = i->second.second;
      s.sc += i->second.first * s.link_weight;
      s.links += i->second.first;
    }

#if ENABLE_CHECKS
  assert( s.check() );
#endif
}

void state::perturbation::commit( state& s )
{
  commit2( s, present, absent );
}

void state::perturbation::commit_permanently( state& s )
{
  commit2( s, required, disallowed );
}

bool state::perturbation::prune_unlinked( row_t const& r )
{
  if ( ( s.is_present(r) && !is_removed(r) ||
	 s.is_absent (r) &&  is_added  (r) ) &&
       get_linkage(r) == size_t(-1) )
    {
      do_rm_row(r);

      if ( s.qsets.size() )
	remove_qset(r);
      else if ( s.lhs.size() ) 
	remove_lh(r);

      return true;
    }

  return false;
}

void state::prune_unlinked()
{
  while (true)
    {
      bool done_anything = false;
      perturbation p( *this );
      
      for ( int i=0; i != leads.size(); ++i )
	// NB avoid short circuit
	done_anything = p.prune_unlinked( row_t::from_index(i) ) 
	  || done_anything;
      
      if ( done_anything ) 
	p.commit( *this );
      else
	{
	  assert( fully_linked() );
	  return;
	}
    }
}

bool state::perturb()
{
  perturbation p( *this );

  int ri = random_int( leads.size() );
  bool valid(false);

  if ( leads[ri] == present )
    valid = p.remove_row( row_t::from_index(ri) );

  else if ( leads[ri] == absent )
    valid = p.add_row( row_t::from_index(ri) );

  else 
    return false;

  if ( valid && should_keep( p.delta() ) ) 
    {
      p.commit(*this);
      return true;
    }

  return false;
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
      
      s.reset( new state( args.meth, stflags, args.pends, args.required, 
			  args.calls, args.wprof  ) );
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
    
    if ( !args.quiet )
      cout << "Using part-end group of order " << args.pends.size() << endl;
  
    for ( int i=0; args.loop == -1 || i < args.loop; ++i ) {
      
      const double beta_init  = 3;
      const double beta_final = 25;

      
      const double beta_mult
	= pow( beta_final / beta_init, 1/double(args.num_steps) );
      
      int n = 0;
      for ( double beta = beta_init ; beta < beta_final; beta *= beta_mult ) {
	s->set_beta(beta);
	s->perturb();

	if ( args.status ) {
	  if ( n++% 1000 == 0 )
	    status_out( make_string() << "Currently " 
			<< floor(double(n)/args.num_steps * 1000)/10. 
			<< "% done" );
	}
      }

      if ( args.linkage && !s->fully_linked() )
	s->prune_unlinked();

      clear_status();

#if ENABLE_CHECKS
      if (!s->check()) { 
	cerr << "ERROR!!!" << endl;
	s->dump( cerr );
	exit(1);
      }
#endif

      if ( s->length() > mx && ( !args.linkage || s->fully_linked() ) ) 
	mx = s->length();
      
      if ( !args.quiet ) {
	cout << s->length() << " leads " 
	     << "(" << s->length() * args.meth.size() << ")";
	if ( args.linkage && !s->fully_linked() )
	  cout << " not fully linked (" << setw(2) << s->percent_linked() << "%)";
	if ( args.loop != 1 )
	  cout << " [highest = " << mx << " leads "
	       << "(" << mx * args.meth.size() << ")]";
	cout << endl;
      }

      if ( args.print_leads && s->length() >= args.min_leads 
	   && ( !args.linkage || s->fully_linked() ) ) {
	s->dump( cout );
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
