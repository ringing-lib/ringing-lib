// -*- C++ -*- peal.h - Details of a peal
// Copyright (C) 2004 Richard Smith <richard@ex-parrot.com>.

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// $Id$

#ifndef RINGING_PEAL_H
#define RINGING_PEAL_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <string>
#include <ringing/libfacet.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class peal {
public:
  struct date {
    date() : day(0), month(0), year(0) {}
    date( int day, int month, int year )
      : day(day), month(month), year(year) {}

    int day, month, year;
  };

  peal() {}
  peal( const date& d, const string& l ) : d(d), l(l) {}

  date const& when() const { return d; }
  string const& where() const { return l; }

private:
  date d;
  string l;
};

RINGING_DECLARE_LIBRARY_FACET( first_tower_peal, peal );
RINGING_DECLARE_LIBRARY_FACET( first_hand_peal,  peal );

RINGING_DECLARE_LIBRARY_FACET( rw_ref, string );

RINGING_END_NAMESPACE

#endif
