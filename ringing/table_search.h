// -*- C++ -*- table_search.h - A search class using a multiplication table
// Copyright (C) 2002, 2010 Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_TABLE_SEARCH_H
#define RINGING_TABLE_SEARCH_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/search_base.h>
#include <ringing/group.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

#if RINGING_AS_DLL
RINGING_EXPLICIT_TEMPLATE 
struct RINGING_API RINGING_PREFIX_STD pair<size_t, size_t>;
#endif

class RINGING_API table_search : public search_base
{
public:
  enum flags {
    no_flags = 0x0,

    // Prune rotations from the search
    ignore_rotations = 0x01,  // == true

    // In multipart searches, allow blocks that do not all join together
    // (e.g. WHx2 as 6-part of PB6)
    mutually_true_parts = 0x02,

    // For reasons of historical compatbility, the length argument
    // is ordinarily in leads.  This option says the lengths are in changes.
    length_in_changes = 0x04,

    // Allow non-round blocks.  Not compatible with ignore_rotations
    non_round_blocks = 0x08
  };

  // Constructors
  table_search( const method &meth, const vector<change> &calls,
                const group& partends, flags = no_flags);

  table_search( const method &meth, const vector<change> &calls,
                const group& partends, pair< size_t, size_t > lenrange, 
		flags = no_flags );

  // Older constructors, partly left for backwards compatibility
  table_search( const method &meth, const vector<change> &calls,
                bool set_ignore_rotations = false);

  table_search( const method &meth, const vector<change> &calls,
                pair< size_t, size_t > lenrange, 
                bool set_ignore_rotations = false);

private:
  // The implementation
  class context;
  friend class context;
  virtual context_base *new_context() const;

  // Data members
  method meth;
  vector<change> calls;
  group partends;
  pair< size_t, size_t > lenrange; // The minimum and maximum number of leads
  flags f;
};


RINGING_END_NAMESPACE

#endif // RINGING_TABLE_SEARCH_H
