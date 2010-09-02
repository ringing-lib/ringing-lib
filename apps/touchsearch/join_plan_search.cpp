// -*- C++ -*- join_plan_search.cpp - Search ways to join up a plan
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

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <bvector.h>
#else
#include <vector>
#endif
#include <ringing/search_base.h>
#include <ringing/falseness.h>
#include <ringing/multtab.h>
#include <ringing/extent.h>
#include <ringing/touch.h>
#include "join_plan_search.h"

#define DEBUG_LEVEL 0

#if DEBUG_LEVEL
#include <iostream>
#define IF_DEBUG( expr ) (void)( expr )
#else 
#define IF_DEBUG( expr ) (void)(false)
#endif
#define DEBUG( expr ) IF_DEBUG((cout << expr) << endl)

RINGING_START_NAMESPACE

RINGING_USING_STD

join_plan_search::join_plan_search( unsigned int bells,
                                    map<row, method> const& plan, 
                                    vector<change> const& calls,
                                    join_plan_search::flags f )
  : plan(plan), calls(calls), lenrange( size_t(0), size_t(-1) ), 
    f(f), bells(bells)
{}

join_plan_search::join_plan_search( unsigned int bells, 
                                    map<row, method> const& plan, 
                                    vector<change> const& calls,
                                    pair<size_t, size_t> lenrange,
                                    join_plan_search::flags f )
  : plan(plan), calls(calls), lenrange(lenrange), 
    f(f), bells(bells)
{}

class join_plan_search::context : public search_base::context_base
{
public:
  context( const join_plan_search* s ) 
    : lenrange( s->lenrange ),
      table( make_table(s) )
  {
    DEBUG( "Constructing context: table size " << table.size() );

    init_touch_meths( s->plan );

    for ( size_t i=0; i < s->calls.size(); ++i )
      init_call( s->calls[i] );

    t.push_back( tl = new touch_child_list );
    t.set_head( tl );
  }

private:
  typedef multtab::post_col_t post_col_t;
  typedef multtab::row_t row_t;

  static bool is_in_course( const join_plan_search *s )
  {
    int const psign 
      = s->plan.size() ? s->plan.begin()->second.back().sign() : 0;

    for ( map<row,method>::const_iterator i=s->plan.begin(), e=s->plan.end();
            i != e; ++i )
      if ( i->second.lh().sign() == -1 ) {
        DEBUG( "Lead head of method " << i->second.format() << " (at " 
               << i->first << ") is out-of-course" );
        return false;
      } 
      else if ( i->second.back().sign() != psign ) {
        DEBUG( "Methods have different parity lead end changes" );
        return false;
      }
        

    for ( size_t i=0; i < s->calls.size(); ++i )
      if ( s->calls[i].sign() != psign ) {
        DEBUG( "Call #" << i << " is parity-altering" );
        return false;
      }

    DEBUG( "Lead heads are all in-course" );
    return true;
  }

  static multtab make_table( const join_plan_search *s )
  {
    if ( is_in_course(s) ) {
      return multtab( incourse_extent_iterator( s->bells - 1, 1),
                      incourse_extent_iterator() );
    }
    else {
      return multtab( extent_iterator( s->bells - 1, 1 ),
                      extent_iterator() );
    }
  }

  void init_touch_meths( map<row,method> const& p )
  {
    map<method, int> meths;
    vector<int>( table.size(), -1 ).swap( plan );

    for ( map<row,method>::const_iterator i=p.begin(), e=p.end(); i != e; ++i )
    { 
      method m( i->second );  m.pop_back();
      if ( meths.find(m) == meths.end() ) {
        meths[m] = les.size();
        DEBUG( "Found method #" << les.size() << ": " << m.format() );
        les.push_back( table.compute_post_mult( m.lh() ) );
        lens.push_back( m.size() + 1 );
        t.push_back( new touch_changes( m.begin(), m.end() ) ); 
      }
        
      plan[ table.find( i->first ).index() ] = meths[m];
    }
  }

  void init_call( change const& ch )
  {
    call_les.push_back( table.compute_post_mult( row() * ch ) );
    DEBUG( "Computing call " << ch );

    touch_changes *c; // the lead end change
    t.push_back( c = new touch_changes() );
    c->push_back( ch );

    for ( int m=0, n=les.size(); m<n; ++m ) {
      touch_child_list *cl; // the whole lead
      t.push_back( cl = new touch_child_list );
      cl->push_back( 1, t.get_node(m) );
      cl->push_back( 1, c );
    }
  }

  virtual void run( outputer &output ) 
  {
    force_halt = false;
    nodes = 0ul;
    lead_vector_t( table.size(), false ).swap( leads );
    run_recursive( output, row_t(), 0 );
  }

  void output_touch( outputer& output )
  {
    list< touch_child_list::entry > &ch = tl->children();
    ch.clear();

    for ( size_t i=0, n=comp.size(); i < n; ++i )
      tl->push_back( 1, t.get_node(comp[i]) );

    DEBUG( "Have touch" );
    force_halt = output( t );
  }

  // The main loop of the algorithm   
  void run_recursive( outputer &output, const row_t &lh, size_t depth )
  {
#if DEBUG_LEVEL > 1
    IF_DEBUG( copy( comp.begin(), comp.end(), ostream_iterator<int>(cout) ));
    DEBUG( " at depth " << depth );
#endif 
      
    IF_DEBUG( (++nodes % 1000000 == 0) && (cout << "Node: " << nodes << "\n") );

    int meth_n = plan[ lh.index() ];
    if ( meth_n == -1 ) return;  // We're outside of the plan.

    row_t const le( lh * les[meth_n] );

    // Is it going to repeat?
    if ( leads[lh.index()] || leads[le.index()] ) 
      {
        // Has it come round, and is it in it's canonical form?
        if ( depth >= lenrange.first && lh.isrounds() )
          output_touch( output );
      }
    else if ( depth < lenrange.second )
      {
        const int num_meths = les.size();

        leads[lh.index()] = true;
        leads[le.index()] = true;
        comp.push_back( num_meths + meth_n + 1 );

        for ( int i = 0, n = call_les.size(); !force_halt && i < n; ++i )
          {
            run_recursive( output, le * call_les[i], depth + lens[meth_n] );
            comp.back() += num_meths + 1;
          }

        comp.pop_back();
        leads[le.index()] = false;
        leads[lh.index()] = false;
      }
  }

  // Data members
  pair< size_t, size_t > lenrange;      // The min & max lengths (in leads)
  bool force_halt;                      // Are we terminating the search?
  multtab table;                        // A precomputed multiplication table

  touch t;                              // The current touch
  touch_child_list *tl;
  vector< post_col_t > les;             // l.e. row for each method
  vector< size_t > lens;                // lengths for each method
  vector< int > plan;                   // Map multtab::row_t => index into les
  vector< post_col_t > call_les;

  RINGING_ULLONG nodes;                 // Node count

  // Logically this is a vector<bool>, but the C++ standard mandates 
  // that that should be a packed structure.  Changing to vector<char>
  // makes a small but significant speed improvement.
  typedef vector<char> lead_vector_t;
  lead_vector_t leads;                  // The leads had so far

  vector< size_t > comp;                // The calls we've had so far
};

search_base::context_base *join_plan_search::new_context() const
{
  return new context(this);
}

RINGING_END_NAMESPACE
