// -*- C++ -*- output.h - generic classes to handle output of methods
// Copyright (C) 2002, 2003, 2004 Richard Smith <richard@ex-parrot.com>

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

#ifndef METHSEARCH_OUTPUT_INCLUDED
#define METHSEARCH_OUTPUT_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <string>
#include <ringing/library.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

class method_properties : public library_entry
{
public:
  method_properties() {}
  explicit method_properties( const method& m );
  explicit method_properties( const library_entry& e );

  string get_property( int num_opt, const string& name ) const;

private:
  class impl2;
};


#endif // METHSEARCH_OUTPUT_INCLUDED
