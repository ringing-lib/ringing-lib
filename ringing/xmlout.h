// -*- C++ -*- xmlout.h - Output XML
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

#ifndef RINGING_XMLOUT_H
#define RINGING_XMLOUT_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/library.h>
#include <ringing/pointers.h>
#include <string>

RINGING_START_NAMESPACE

RINGING_USING_STD

class xmlout {
public:
  xmlout( const string& filename );

  class iterator;

  iterator begin();
  iterator end();
  
  void flush();

private:
  xmlout( const xmlout& ); // Unimplemented
  xmlout& operator=( const xmlout& ); // Unimplemented

  friend class iterator;

  // Data members
  class impl;
  scoped_pointer<impl> pimpl;
};

class xmlout::iterator {
public:
  iterator() : pimpl(NULL) {}
  
  iterator& operator++() { return *this; }
  iterator operator++(int) { return *this; }
  iterator& operator*() { return *this; }

  void operator=( library_entry const& entry );
    
private:
  friend class xmlout;
  iterator( xmlout::impl* p ) : pimpl(p) {}

  // Data members
  impl* pimpl;
};

inline xmlout::iterator xmlout::begin() {
  return iterator( pimpl.get() );
}

inline xmlout::iterator xmlout::end() {
  return iterator();
}

RINGING_END_NAMESPACE

#endif
