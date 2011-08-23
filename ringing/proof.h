// -*- C++ -*- proof.h - Proving Stuff
// Copyright (C) 2001, 2002, 2006, 2008, 2011 
// Mark Banner <mark@standard8.co.uk>
// and Richard Smith <richard@ex-parrot.com>

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


#ifndef RINGING_PROOF_H
#define RINGING_PROOF_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <list.h>
#include <multimap.h>
#include <algo.h>
#else
#include <iostream>
#include <list>
#include <map>
#include <algorithm>
#endif
#include <ringing/row.h>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

/******************************************
  Proof Class
 *****************************************/

// Struct to store falseness details
struct linedetail {
  row _row;
  list<int> _lines;

  linedetail() {} // Compiler-generated constructor triggers bug in gcc 4.0.1

  RINGING_FAKE_COMPARATORS( linedetail )
};

class RINGING_API prover
{
public:
  typedef list<linedetail> failinfo;

  // max_occurs is the number of times a row is permitted to occur
  // in the touch before it is considered false.
  explicit prover( int max_occurs = 1 )
    : max_occurs(max_occurs), lineno(0), falsec(0u), dups(0u), fi(NULL)
  {}

  // fi is a structure into which information about duplicate lines 
  // are inserted.
  explicit prover( failinfo &fi, int max_occurs = 1 )
    : max_occurs(max_occurs), lineno(0), falsec(0u), dups(0u), fi(&fi)
  {}

  // Adds a row to the touch, and returns true if the touch (so far) 
  // contains no rows more than max_occurs times.  Inserts rows present 
  // more than max_occurs times into the failinfo structure (if one was 
  // supplied).
  bool add_row( const row &r );
  void remove_row( const row& r );

  // Returns the number of instances of 'r' in the touch.
  size_t count_row( const row& r ) const;

  // The length of the touch
  size_t size() const { return m.size(); }

  size_t duplicates() const { return dups; }

  bool truth() const { return falsec == 0; }

  void disable_proving() { max_occurs = -1; }

  // Create a prover referencing all the rows in its argument.  It is 
  // undefined behaviour if chained prover (the argument) is modified
  // whilst the returned prover is in use.
  // Effectively an efficient way of copying a prover.
  static shared_pointer<prover> 
  create_branch( shared_pointer<prover> const& chain );

private:
  shared_pointer<prover> chain;
  typedef multimap<row, int> mmap;
  int max_occurs;
  int lineno;
  size_t falsec, dups;
  mmap m;
  failinfo *fi;
};



/********************************************************************
 * Description     :
 *    This class provides functions for proving a set of rows
 * unique. The constructor normally does the main proving bit, it
 * takes first and last iterators for the set of rows, and possibly
 * a number specifing the maximum number of times a row can be
 * repeated (for touches spanning multiple extents).
 *
 * It will check ALL rows given, therefore if rounds are at the start
 * and end, then one of them should be removed before passing to the
 * object.
 *
 * This object only stores the results, it does not make a copy of
 * the actual rows. However, pointers to false rows will be stored,
 * so don't destroy your rows before getting all the proof results.
 *
 * The default hash function for multiple extents just turns the row
 * directly into a number, this ensures uniqueness for any given
 * row/key pair.
 ********************************************************************/

// The way we prove things is as follows:
// 
// We have a big map.  The keys are ints, and the values are
// lists of rows, with a count for each row.  Each row we get,
// we first call the hash function; this returns us an int,
// given a row.  We then use this int as a key into our big
// map, and see whether the row is in the list associated with
// that int; if it is, then increase its count and see whether
// it is bigger than the maximum number we are allowed for
// any one row.  If it is, we have failed.  If it isn't, we add
// that row to the list and carry on.
//
// In the normal case, we will only be looking for one occurrence
// of each row, so it seems unnecessary to keep track of the count
// for each row, and it's a waste of space, so don't bother.

template <class RowIterator>
class proof {
public:
  typedef list<linedetail> failinfo;
  typedef multimap<row, int> mmap;

  // Default constructor. Use prove to provide rows to prove.
  // (Set istrue to false as we haven't been given anything to prove yet.)
  proof() : istrue(false) {}

  // In these and the prove functions we assume we are given only the rows
  // to prove. i.e. check all rows from first to last.
  // qp = quick prove


  // Constructor - 1 extent only.
  proof(RowIterator first, RowIterator last, bool qp = false) 
    : istrue(prove(first, last, qp)) 
  {}

  // Constructor - multiple extents.
  proof(RowIterator first, RowIterator last, const int max, 
	bool qp = false)
    : istrue(prove(first, last, max, qp)) 
  {}

  proof(RowIterator true_first,
	RowIterator true_last,
	RowIterator unknown_first,
	RowIterator unknown_last,
	bool qp = false)
    : istrue(prove(true_first, true_last, unknown_first, unknown_last, qp))
  {}

  proof(RowIterator true_first,
	RowIterator true_last,
	RowIterator unknown_first,
	RowIterator unknown_last,
	const int max,
	bool qp = false)
    : istrue(prove(true_first, true_last, unknown_first, unknown_last, max, qp))
  {}

  bool prove(RowIterator first, RowIterator last, 
	     bool qp = false) {
    return prove(first, first, first, last, 1, qp);
  }

  bool prove(RowIterator first, RowIterator last, 
	     const int max, bool qp = false) {
    return prove(first, first, first, last, max, qp);
  }

  // Proof function for where a block is known to be true, and
  // an additional block has been added on to it.
  bool prove(RowIterator true_first, RowIterator true_last,
	     RowIterator unknown_first, RowIterator unknown_last,
	     bool qp = false) {
    return prove(true_first, true_last, unknown_first, unknown_last, 1, qp);
  }

  bool prove(RowIterator true_first, RowIterator true_last,
	     RowIterator unknown_first, RowIterator unknown_last,
	     const int max, bool qp = false);
 
  const failinfo& failed() const // Return rows where it failed.
    { return where; }
  operator int() const { return istrue; }   // Is touch true or false?
  int operator!() const { return !istrue; }

private:
  bool istrue;                   // Well, is it true or not?
  failinfo where;                // Details of where it failed.
}; 

/******************************************
  Definitions for the proof class.
 *****************************************/

// Proof function for where a block is known to be true, and
// an additional block has been added on to it.
template <class RowIterator>
bool proof<RowIterator>::prove(RowIterator true_first,
			       RowIterator true_last,
			       RowIterator unknown_first,
			       RowIterator unknown_last,
			       const int max,
			       bool qp)
{
  scoped_pointer<prover> p( qp ? new prover(max) : new prover(where, max) );
  istrue = true;
  for ( ; true_first != true_last && (!qp || istrue) ; ++true_first )
    istrue = istrue && p->add_row( *true_first );
  for ( ; unknown_first != unknown_last && (!qp || istrue) ; ++unknown_first )
    istrue = istrue && p->add_row( *unknown_first );
  return istrue;
}

RINGING_START_DETAILS_NAMESPACE

RINGING_API void print_failinfo( ostream& os, bool istrue, 
				 prover::failinfo const& fi );

RINGING_END_DETAILS_NAMESPACE


// Output function for falseness, This is mainly for logging.
template <class RowIterator>
ostream& operator<<(ostream &o, const proof<RowIterator> &p)
{
  RINGING_DETAILS_PREFIX print_failinfo( o, bool(p), p.failed() ); 
  return o;
}


RINGING_END_NAMESPACE

#endif
