// -*- C++ -*- basic_search.h - A simple touch search class
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

#ifndef RINGING_BASIC_SEARCH_H
#define RINGING_BASIC_SEARCH_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/search_base.h>

RINGING_START_NAMESPACE

RINGING_USING_STD


class basic_search : public search_base
{
public:
  // Constructors
  basic_search( const method &meth, const vector<change> &calls,
		bool ignore_rotations = false );

  basic_search( const method &meth, const vector<change> &calls,
		pair< size_t, size_t > lenrange, 
		bool ignore_rotations = false );

private:
  // The implementation
  class context;
  friend class context;
  virtual context_base *new_context() const;

  // Data members
  method meth;
  vector<change> calls;
  pair< size_t, size_t > lenrange; // The minimum and maximum number of leads
  bool ignore_rotations;
};


RINGING_END_NAMESPACE

#endif // RINGING_BASIC_SEARCH_H
