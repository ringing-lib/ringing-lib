// -*- C++ -*- proof.h - Proving Stuff
// Copyright (C) 2001 Mark Banner <mark@standard8.co.uk>

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

#ifndef RINGING_PROOF_H
#define RINGING_PROOF_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <list.h>
#include <map.h>
#include <algorithm.h>
#else
#include <iostream>
#include <list>
#include <map>
#include <algorithm>
#endif
#include <ringing/row.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

// Our hash function
int our_hash(const row& r);           // Default hash function

/******************************************
  Proof Class
 *****************************************/

// Struct to store falseness details
struct linedetail {
  row _row;
  list<int> _lines;

#if RINGING_PREMATURE_MEMBER_INSTANTIATION
  bool operator==(const linedetail &) const;
  bool operator!=(const linedetail &) const;
  bool operator<(const linedetail &) const;
  bool operator>(const linedetail &) const;
#endif
};

template <class RowIterator>
class proof {
public:
  typedef list<linedetail> failinfo;
  typedef multimap<int, row> mmap;
  typedef map<int, int> nomap;
  typedef int (*hash_function)(const row& r);

  proof();   // Default constructor. Use prove to provide rows to prove.

  // In these and the prove functions we assume we are given only the rows
  // to prove. i.e. check all rows from first to last.
  proof(RowIterator first, RowIterator last, bool qp = false);
  proof(RowIterator first, RowIterator last, const int max, 
	bool qp = false, hash_function f = &our_hash);
  proof(RowIterator true_first,
	RowIterator true_last,
	RowIterator unknown_first,
	RowIterator unknown_last,
	bool qp = false);
  proof(RowIterator true_first,
	RowIterator true_last,
	RowIterator unknown_first,
	RowIterator unknown_last,
	const int max,
	bool qp = false,
	hash_function f = &our_hash);

  inline bool prove(RowIterator first, RowIterator last, bool qp = false);
  inline bool prove(RowIterator first, RowIterator last, const int max, 
		    bool qp = false, hash_function f = &our_hash);

  inline bool prove(RowIterator true_first,
		    RowIterator true_last,
		    RowIterator unknown_first,
		    RowIterator unknown_last,
		    bool qp = false);
  inline bool prove(RowIterator true_first,
		    RowIterator true_last,
		    RowIterator unknown_first,
		    RowIterator unknown_last,
		    const int max,
		    bool qp = false,
		    hash_function f = &our_hash);

  const failinfo& failed() const // Return rows where it failed.
    { return where; }
  operator int() const;          // Is touch true or false?
  int operator!() const;

private:
  inline void add_to_failmap(RowIterator r,
			     const int &i,
			     const int &j,
			     const int &changed);
  bool return_true(const void*) { return true; };
  void resetfailinfo();
  bool istrue;                   // Well, is it true or not?
  failinfo where;                // Details of where it failed.
}; 

/******************************************
  Definitions for the proof class.
 *****************************************/

// Default constructor
template <class RowIterator>
proof<RowIterator>::proof()
{
  // Set istrue to false as we haven't been given anything to prove yet.
  istrue = false;
  resetfailinfo();
}

// Constructor - 1 extent only.
template <class RowIterator>
proof<RowIterator>::proof(RowIterator first,
			  RowIterator last,
			  bool qp)
{
  // Prove it
  istrue = prove(first, last, qp);
}

// Constructor - multiple extents.
template <class RowIterator>
proof<RowIterator>::proof(RowIterator first,
			  RowIterator last,
			  const int max, 
			  bool qp,
			  proof::hash_function f)
{
  // Prove it
  istrue = prove(first, last, max, qp, f);
}

template <class RowIterator>
proof<RowIterator>::proof(RowIterator true_first,
			  RowIterator true_last,
			  RowIterator unknown_first,
			  RowIterator unknown_last,
			  bool qp)
{
  istrue = prove(true_first,
		 true_last,
		 unknown_first,
		 unknown_last,
		 qp);
}

template <class RowIterator>
proof<RowIterator>::proof(RowIterator true_first,
			  RowIterator true_last,
			  RowIterator unknown_first,
			  RowIterator unknown_last,
			  const int max,
			  bool qp,
			  proof::hash_function f)
{
  istrue = prove(true_first,
		 true_last,
		 unknown_first,
		 unknown_last,
		 max,
		 qp,
		 f);
}

// Prove function for rows up to one extent.
template <class RowIterator>
bool proof<RowIterator>::prove (RowIterator first,
				RowIterator last,
				bool qp)
{
  RowIterator i;
  RowIterator j;
  istrue = true;
  int changed_line;
  int count_i = 0;
  int count_j = 0;

  // First reset out failed information list
  resetfailinfo();

  // The basic algorithm here is to look at the first line and
  // compare it to the rest, then to look at the second and 
  // compare it to the rest (starting from the third, going to the end)
  // and so forth. If it's false the details get stored for later
  // retrival if required.
  i = first;
  while (i != last)
    {
      count_i++;
      changed_line = 1;
      count_j = count_i;
      j = i;
      j++;
      while (j != last)
	{
	  count_j++;
	  if (*j == *i)
	    {
	      if (!qp)
		{
		  istrue = false;
		  this->add_to_failmap(i, count_i, count_j, changed_line);
		  changed_line = 0;
		}
	      else
		// We know this has to be false.
		return false;
	    }
	  j++;
	}
      i++;
    }

  return istrue;
}

// Prove function for rows over one extent.
template <class RowIterator>
bool proof<RowIterator>::prove(RowIterator first,
			       RowIterator last,
			       const int max,
			       bool qp,
			       hash_function f)
{
  RowIterator i;
  int count_i = 0;
  mmap m;
  nomap n;
  istrue = true;

  // First reset out failed information list
  resetfailinfo();

  // This time we go through the list once, checking that we haven't
  // exceeded max, the limit for the number of rows. If we do, then
  // we add the item onto a list for later reference.
  i = first;
  while (i != last)
    {
      count_i++;
      if ((int) m.count(f(*i)) >= max)
	{
	  istrue = false;
	  if (!qp)
	    {
	      this->add_to_failmap(i, (*n.find(f(*i))).second, count_i, 0);
	    }
	  else
	    {
	      // If we are here, then it is false
	      return false;
	    }
	}
      // Insert the pair into the multimap
      m.insert(make_pair(f(*i), *i));
      n.insert(make_pair(f(*i), count_i));
      i++;
    }

  return istrue;
}

// Proof function for where a block is known to be true, and
// an additional block has been added on to it.
template <class RowIterator>
bool proof<RowIterator>::prove(RowIterator true_first,
			       RowIterator true_last,
			       RowIterator unknown_first,
			       RowIterator unknown_last,
			       bool qp)
{
  RowIterator i;
  RowIterator j;
  istrue = true;
  int changed_line;
  int count_i = 0;
  int count_j = 0;
  
  // First reset out failed information list
  resetfailinfo();

  // use i to iterate through just the unknown list.
  i = unknown_first;
  while (i != unknown_last)
    {
      count_i++;
      changed_line = 1;
      count_j = 0;
      // use j to iterate through both true and unknown.
      j = true_first;
      while (j != unknown_last)
	{
	  count_j++;
	  // Only compare if j != i, i.e if this isn't the same row.
	  if (j != i)
	    {
	      // Do we have a match?
	      if (*j == *i)
		{
		  istrue = false;
		  if (!qp)
		    {
		      this->add_to_failmap(i, 
					   count_i + (true_last - true_first),
					   count_j,
					   changed_line);
		      changed_line = 0;
		    }
		  else
		    {
		      return false;
		    }
		}
	    }

	  // if j has reached the end of the true list,
	  // set it to the start of the unknown list.
	  j++;
	  if (j == true_last)
	    {
	      j = unknown_first;
	    }
	}
      i++;
    }

  return istrue;
}

// Proof function for where a block is known to be true, and
// an additional block has been added on to it.
template <class RowIterator>
bool proof<RowIterator>::prove(RowIterator true_first,
			       RowIterator true_last,
			       RowIterator unknown_first,
			       RowIterator unknown_last,
			       const int max,
			       bool qp,
			       proof::hash_function f)
{
  RowIterator i;
  int count_i = 0;
  mmap m;
  nomap n;
  istrue = true;

  // First reset out failed information list
  resetfailinfo();

  // First add all the true rows into the failmap - we don't need to check
  // them.
  for (i = true_first; i != true_last; i++)
    {
      count_i++;
      m.insert(make_pair(f(*i), *i));
      n.insert(make_pair(f(*i), count_i));
    }

  // No go through the fail list, checking that we haven't
  // exceeded max, the limit for the number of rows. If we do, then
  // we add the item onto a list for later reference.
  i = unknown_first;
  while (i != unknown_last)
    {
      count_i++;
      if ((int) m.count(f(*i)) >= max)
	{
	  istrue = false;
	  if (!qp)
	    {
	      this->add_to_failmap(i, (*n.find(f(*i))).second, count_i, 0);
	    }
	  else
	    {
	      // If we are here, then it is false
	      return false;
	    }
	}
      // Insert the pair into the multimap
      m.insert(make_pair(f(*i), *i));
      n.insert(make_pair(f(*i), count_i));
      i++;
    }

  return istrue;
}

template <class RowIterator>
void proof<RowIterator>::add_to_failmap(RowIterator r,
					const int &i,
					const int &j,
					const int &changed)
{
  // add the false rows into the failinfo
  // does it exist already?
  int exists = 0;
  failinfo::iterator fi = where.begin();
  // Search only if there is something to search
  if (where.size() > 0)
    {
      fi = where.begin();
      while ((fi != where.end()) && (exists == 0))
	{
	  if ((*fi)._row == *r)
	    {
	      exists = 1;
	    }
	  else
	    {
	      fi++;
	    }
	}
    }
  if (exists == 0)
    {
      // no, so start a new item.
      linedetail l;
      l._row = *r;
      // sort out the order
      if (j > i)
	{
	  l._lines.push_back(i);
	  l._lines.push_back(j);
	}
      else
	{
	  l._lines.push_back(j);
	  l._lines.push_back(i);
	}
      where.push_back(l);
    }
  else
    {
      // yes it does, but only record the first time a
      // change is entered
      if (changed == 0)
	{
	  (*fi)._lines.push_back(j);
	}
    }
}


// Function for finding out whether it proved true or not.
template <class RowIterator>
proof<RowIterator>::operator int () const
{
  return istrue;
}

template <class RowIterator>
int proof<RowIterator>::operator!() const
{
  return !istrue;
}

// Output function for falseness, This is mainly for logging.
template <class RowIterator>
ostream& operator<<(ostream &o, const proof<RowIterator> &p)
{
  if (p)
    {
      o << "True\n";
    }
  else
    {
      o << "False\n";
      
      // go through the list outputting details in turn.
      proof<RowIterator>::failinfo faili = p.failed();
      proof<RowIterator>::failinfo::iterator fi;
      for (fi = faili.begin(); fi != faili.end(); fi++)
	{
	  o << "Row " << (*fi)._row << " is repeated on lines";
	  list<int>::iterator i;
	  for (i = (*fi)._lines.begin(); i != (*fi)._lines.end(); i++)
	    {
	      o << " " << *i;
	    }
	  o << endl;
	}
    }
  return o;
}

template <class RowIterator>
void proof<RowIterator>::resetfailinfo()
{
  where.erase(where.begin(), where.end());
}

RINGING_END_NAMESPACE

#endif
