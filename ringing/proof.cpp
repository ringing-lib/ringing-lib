// proof.cpp - Proving Stuff
// Copyright (C) 2001, 2002, 2006, 2008, 2010, 2011, 2021
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


#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <stdexcept>

#include <ringing/proof.h>
#include <ringing/iteratorutils.h>

RINGING_USING_STD

RINGING_START_NAMESPACE

size_t prover::count_row( const row& r ) const
{
  size_t n(0);

  for ( prover const* p = this; p; p = p->chain.get() ) {
    pair< mmap::const_iterator, mmap::const_iterator > rng 
      = p->m.equal_range(r);
    n += distance( rng.first, rng.second );
  }

  return n;
}

size_t prover::size() const {
  size_t n(0);
  for ( prover const* p = this; p; p = p->chain.get() )
    n += p->m.size();
  return n;
}

// Returns false if the touch is false
bool prover::add_row( const row &r )
{
  // This function is quite complicated to avoid doing more than one
  // O( ln N ) operation on each multimap.  The equal_range function call
  // will be O( ln N ); the insert function ought to be O(1) because a 
  // sensible hint is supplied (although the C++ standard doesn't require
  // the hint to be used).  The number of identical elements is also 
  // calculated from the range, rather than doing a O( ln N ) call to 
  // multimap::count.

  typedef pair< mmap::iterator, mmap::iterator > range;
  list<range> ranges;

  for ( prover* p = this; p; p = p->chain.get() ) 
    ranges.push_front( p->m.equal_range(r) );

  // effecively m.count(r)
  size_t n(1);
  for ( list<range>::const_iterator ri = ranges.begin(), re = ranges.end(); 
	ri != re; ++ri ) 
    n += distance( ri->first, ri->second );

  range const& rng = ranges.back();

  mmap::iterator i( m.insert( rng.first == m.begin() ? m.begin() 
			                             : prior( rng.first ),
			      mmap::value_type( r, ++lineno ) ) );

  if ( n > 1 )
    ++dups; 

  if ( max_occurs != -1 && (int) n > max_occurs )
    {
      falsec++;
      if ( fi )
	{

	  for ( failinfo::iterator j = fi->begin(), e = fi->end(); j != e; ++j)
	    if ( j->_row == r )
	      {
		j->_lines.push_back( i->second );
		return false;
	      }

	  linedetail l;

	  // The C++ standard doesn't make any guarantee about where i
	  // is in relation to rng.first and rng.second -- it may fall in
	  // the range [rng.first, rng.second), but equally, it might get 
	  // inserted immediately before rng.first.  If we don't detect i 
	  // in this range, we explicitly add it by hand afterwards.
	  l._row = r;
	  bool added_i = false;
	  {
	    for ( mmap::iterator j = rng.first; j != rng.second; ++j )
	      {
		if ( j == i ) added_i = true;
		l._lines.push_back( j->second );
	      }
	  }
	  if (!added_i) 
	    l._lines.push_back( i->second );

	  fi->push_back( l );
	  return false;
	}
    }

  return truth();
}

void prover::remove_row( const row& r )
{
  // As above.  TODO:  Refactor
  typedef pair< mmap::iterator, mmap::iterator > range;
  list<range> ranges;

  for ( prover* p = this; p; p = p->chain.get() ) 
    ranges.push_front( p->m.equal_range(r) );

  // effecively m.count(r)
  size_t n(0);  // Note this is not 1 as in add_row
  for ( list<range>::const_iterator ri = ranges.begin(), re = ranges.end(); 
	ri != re; ++ri ) 
    n += distance( ri->first, ri->second );

  range const& rng = ranges.back();
  if ( n == 0 )
    throw logic_error( "Row does not exist to be removed" );
  if ( rng.first == rng.second ) 
    throw logic_error( "Row does not exist at proof head to be removed" );

  --lineno;
  m.erase( prior( rng.second ) );

  if ( n > 1 )
    --dups;

  if ( max_occurs != -1 && (int) n > max_occurs )
    {
      falsec--;
      if (fi)
        {
	  for ( failinfo::iterator j = fi->begin(), e = fi->end(); j != e; ++j)
	    if ( j->_row == r )
	      {
		j->_lines.pop_back(); 
                if ( j->_lines.empty() ) fi->erase(j);
                break;
	      }
        }
    }
}

shared_pointer<prover> 
prover::create_branch( shared_pointer<prover> const& chain )
{
  shared_pointer<prover> p( new prover );
  p->chain      = chain;
  p->max_occurs = chain->max_occurs;
  p->lineno     = chain->lineno;
  p->dups       = chain->dups;
  p->fi         = chain->fi;
  // NB do not copy chain->m.
  return p;
}

RINGING_START_DETAILS_NAMESPACE

void print_failinfo( ostream& o, bool istrue, prover::failinfo const& faili )
{
  if (istrue)
    {
      o << "True\n";
    }
  else
    {
      o << "False\n";
      
      // go through the list outputting details in turn.
      
      for (prover::failinfo::const_iterator fi = faili.begin(); 
	   fi != faili.end(); ++fi)
	{
	  o << "Row " << fi->_row << " is repeated on lines";
	  
	  for (list<int>::const_iterator i = fi->_lines.begin(); 
	       i != fi->_lines.end(); ++i)
	    {
	      o << " " << *i;
	    }
	  o << endl;
	}
    }
}

RINGING_END_DETAILS_NAMESPACE

RINGING_END_NAMESPACE
