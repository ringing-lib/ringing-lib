// -*- C++ -*- search_base.h - Base class for searching
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

#ifndef RINGING_SEARCH_BASE_H
#define RINGING_SEARCH_BASE_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

RINGING_START_NAMESPACE

// FIXME!!
class search_common;
class touch;

class RINGING_API search_base
{
public:
  virtual ~search_base() {}

  class outputer
  {
  public:
    // Returns true if the search should halt
    virtual bool operator()( const touch &t ) = 0;
  };

  void run( outputer &o ) const;

RINGING_PROTECTED_IMPL:
  class RINGING_API context_base
  {
  public:
    virtual void run( outputer & ) = 0;
    virtual ~context_base() {}
  };

  friend class search_common;
private:
  virtual context_base *new_context() const = 0;
};


RINGING_START_DETAILS_NAMESPACE

template < class OutputIterator >
class search_output : public search_base::outputer 
{
public:
  search_output( const OutputIterator &iter )
    : iter(iter)
  {}

  virtual bool operator()( const touch &t )
  {
    *iter++ = t;
    return false;
  }

private:
  OutputIterator iter;
};

template < class OutputIterator, class UnaryPredicate >
class search_output_until : public search_base::outputer 
{
public:
  search_output_until( const OutputIterator &iter,
		       const UnaryPredicate &until ) 
    : iter(iter), until(until)
  {}
  
  virtual bool operator()( const touch &t )
  {
    *iter++ = t;
    return until( t );
  }
  
private:
  OutputIterator iter;
  UnaryPredicate until;
};

RINGING_END_DETAILS_NAMESPACE


template < class OutputIterator > 
void touch_search( const search_base &searcher, 
		   const OutputIterator &iter )
{
  RINGING_USING_DETAILS
  search_output< OutputIterator > o( iter );
  searcher.run( o );
}

  
template < class OutputIterator, class UnaryPredicate > 
void touch_search( const search_base &searcher, 
		   const OutputIterator &iter,
		   const UnaryPredicate &terminate )
{
  RINGING_USING_DETAILS
  search_output_until< OutputIterator, UnaryPredicate > o( iter, terminate );
  searcher.run( o );
}

RINGING_END_NAMESPACE

#endif // RINGING_SEARCH_BASE_H

