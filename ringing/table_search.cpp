// -*- C++ -*- table_search.cpp - A search class using a multiplication table
// Copyright (C) 2001, 2002 Richard Smith <richard@ex-parrot.com>

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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <bvector.h>
#else
#include <vector>
#endif
#include <ringing/search_base.h>
#include <ringing/table_search.h>
#include <ringing/falseness.h>
#include <ringing/multtab.h>
#include <ringing/extent.h>
#include <ringing/touch.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

table_search::table_search( const method &meth, const vector<change> &calls,
			    bool ignore_rotations )
  : meth( meth ), calls( calls ), 
    lenrange( make_pair( size_t(0), size_t(-1) ) ),
    ignore_rotations( ignore_rotations )
{}

table_search::table_search( const method &meth, const vector<change> &calls,
			    pair< size_t, size_t > lenrange, 
			    bool ignore_rotations )
  : meth( meth ), calls( calls ), 
    lenrange( lenrange ),
    ignore_rotations( ignore_rotations )
{}


class table_search::context : public search_base::context_base
{
public:
  context( const table_search *s ) 
    : lenrange( s->lenrange ), 
      table( extent_iterator( s->meth.bells()-1, 1 ),
	     extent_iterator() ),
      ignore_rotations( s->ignore_rotations )
  {
    row le; 
    for_each( s->meth.begin(), s->meth.end()-1, permute(le) );
      
    t.push_back( new touch_changes( s->meth.begin(), s->meth.end()-1 ) );

    // The plain lead is a call too.      
    init_call( le, s->meth.back() );
    
    for ( size_t i=0; i < s->calls.size(); ++i )
      init_call( le, s->calls[i] );
    
    t.push_back( tl = new touch_child_list );    
    t.set_head( tl );

    init_falseness( s->meth );
  }

private:
  typedef multtab::post_col_t post_col_t;
  typedef multtab::row_t row_t;

  void init_falseness( const method &meth )
  {
    falseness_table ft( meth );
    
    for ( falseness_table::const_iterator i( ft.begin() ); i != ft.end(); ++i )
      {
	falsenesses.push_back( table.compute_post_mult( *i ) );
      }
  }

  void init_call( const row &le, const change &ch )
  {
    touch_changes *c;
    touch_child_list *cl;
    
    t.push_back( c = new touch_changes() );
    t.push_back( cl = new touch_child_list );
    cl->push_back( 1, t.get_node(0) );
    cl->push_back( 1, c );
    c->push_back( ch );
    
    call_lhs.push_back( table.compute_post_mult( le * ch ) );
  }
  
  // Keep looking for touches, pushing them down the outputer.
  virtual void run( outputer &output ) 
  {
    force_halt = false;
    vector<bool>( table.size() ).swap( leads );
    run_recursive( output, row_t(), 0, 0 );
  }

  // Is the row false against a row that we've already had?
  bool is_row_false( const row_t &r )
  {
    for ( vector< post_col_t >::const_iterator i( falsenesses.begin() ); 
	  i != falsenesses.end(); ++i )
      if ( leads[ (r * *i).index() ] )
	return true;

    return false;
  }

  // Output the current touch and any rotations of it.
  void output_touches( outputer &output, size_t cur )
  {
    size_t len( calls.size() );
    size_t parts( len % cur ? cur : len / cur );
    list< touch_child_list::entry > &ch = tl->children();

    ch.clear();
    
    for ( size_t i=0; i < len; ++i )
      tl->push_back( 1, t.get_node( 2 * (1 + calls[i]) ) );

    force_halt = output( t );

    if ( ! ignore_rotations )
      for ( size_t start = 1; !force_halt && start < len / parts; ++start )
	{
	  // Try all of it's distinguishable rotations.
	  ch.splice( ch.end(), ch, ch.begin() );
	  force_halt = output( t );
	}
  }

  // A touch, T, is in canonical form if there exists no rotation of T
  // that is lexicographically less than T.

  // This function returns true if the current fragment could possibly be
  // at the start of a canonical touch.
  bool is_possibly_canonical( size_t &cur )
  {
    if ( calls.empty() )
      return true;

    if ( cur == 0 || calls.back() > calls[ calls.size() - 1 - cur ] )
      cur = calls.size();

    else if ( calls.back() < calls[ calls.size() - 1 - cur ] )
      return false;

    return true;
  }

  // ... and this function checks that a touch fragment for which 
  // is_possibly_canonical() returns true is a canonical complete touch.
  bool is_really_canonical()
  {
    for ( vector< size_t >::const_iterator i( calls.begin() ); 
	  i != calls.end(); ++i )
      if ( *i != calls.front() )
	return calls.back() != calls.front();
    return true;
  }

  // The main loop of the algorithm   
  void run_recursive( outputer &output, const row_t &r, size_t depth, size_t cur )
  {
    // Is the touch lexicographically no greater than any of it's rotations?
    if ( !is_possibly_canonical( cur ) )
      return;

    // Is the going to repeat?
    else if ( is_row_false( r ) )
      {
	// Has it come round, and is it in it's canonical form?
	if ( depth >= lenrange.first && r.isrounds() && is_really_canonical() )
	  output_touches( output, cur );
      }
    else if ( depth < lenrange.second )
      {
	leads[r.index()] = true;
	calls.push_back( 0 );
	
	for ( ; !force_halt && calls.back() < call_lhs.size(); ++calls.back() )
	  {
	    run_recursive( output, r * call_lhs[ calls.back() ], 
			   depth + 1, cur );
	  }
	
	calls.pop_back();
	leads[r.index()] = false;
      }
  }
  
private:
  // Data members
  pair< size_t, size_t > lenrange;	// The min & max lengths (in leads)
  bool force_halt;			// Are we terminating the search?
  vector< size_t > calls;		// The calls we've had so far
  touch t;				// The current touch
  touch_child_list *tl;
  bool ignore_rotations;		// Are we to ignore rotations?

  vector< bool > leads;			// The leads had so far
  vector< post_col_t > call_lhs;	// The effect of each call (inc. Pl.)
  vector< post_col_t > falsenesses;	// The falsenesses of the method
  multtab table;			// A precomputed multiplication table
};

search_base::context_base *table_search::new_context() const 
{ 
  return new context( this );
}

RINGING_END_NAMESPACE
