// -*- C++ -*- join_plan_search.h - Search ways to join up a plan
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

#ifndef RINGING_JOIN_PLAN_SEARCH_H
#define RINGING_JOIN_PLAN_SEARCH_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <map.h>
#else
#include <vector>
#include <map>
#endif
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/search_base.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class RINGING_API join_plan_search : public search_base 
{
public:
  // Let's try to keep this compatbile with table_search::flags...
  enum flags {
    no_flags = 0x0,

    // Assert that the plan has no internal falseness
    no_internal_falseness = 0x10
  };

  join_plan_search( unsigned int bells, map<row, method> const& meths, 
                    vector<change> const& calls, flags = no_flags );

  join_plan_search( unsigned int bells, map<row, method> const& meths, 
                    vector<change> const& calls, pair<size_t, size_t> lenrange,
                    flags = no_flags );

private:
  // The implementation
  class context;
  friend class context;
  virtual context_base *new_context() const;
 
  // Data members
  map<row, method>       plan;
  vector<change>         calls;
  pair< size_t, size_t > lenrange;
  flags                  f;
  unsigned               bells;
};

RINGING_END_NAMESPACE

#endif // RINGING_JOIN_PLAN_SEARCH_H
