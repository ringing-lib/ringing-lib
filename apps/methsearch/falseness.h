// -*- C++ -*- falseness.h - things to analyse falseness
// Copyright (C) 2002 Richard Smith <richard@ex-parrot.com>

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

#ifndef METHSEARCH_FALSENESS_INCLUDED
#define METHSEARCH_FALSENESS_INCLUDED

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include <ringing/pointers.h>
#include <string>

// Forward declare ringing::method
RINGING_START_NAMESPACE
class method;
class row;
RINGING_END_NAMESPACE

RINGING_USING_NAMESPACE
RINGING_USING_STD

class falseness_group_table
{
public:
  enum type { none, regular, bnw };
  static void init( type t );
  static string codes( const method &m );

  // Public to avoid MSVC compilation errors
 ~falseness_group_table();

private:
  struct impl;
  scoped_pointer< impl > pimpl;

  static falseness_group_table &instance();

  falseness_group_table();
};


bool might_support_extent( const method &m );

bool has_particular_fch( const row &fch, const method &m );

bool is_cps( const method &m );

#endif // METHSEARCH_FALSENESS_INCLUDED
