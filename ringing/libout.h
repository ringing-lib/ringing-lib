// -*- C++ -*- libout.h - general classes for output of a method library
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

#ifndef RINGING_LIBOUT_H
#define RINGING_LIBOUT_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class library_entry;

// TODO: Merge this with library
class libout {
public:
  libout() {}

  void flush() { pimpl->flush(); }
  void append( library_entry const& entry ) { pimpl->append(entry); }

  class iterator;

  iterator begin();
  iterator end();

RINGING_PROTECTED_IMPL:
  class interface {
  public:
    virtual ~interface() {}
    virtual void append( library_entry const& entry ) = 0;
    virtual void flush() {}
  };

  explicit libout( interface* p ) : pimpl(p) {}

  template <class ImplType> ImplType* get_impl( ImplType* = 0 ) {
    return static_cast<ImplType*>(pimpl.get()); 
  }

  template <class ImplType> ImplType const* get_impl( ImplType* = 0 ) const { 
    return static_cast<ImplType const*>(pimpl.get()); 
  }

private:
  libout( const libout& ); // Unimplemented
  libout& operator=( const libout& ); // Unimplemented

  scoped_pointer<interface> pimpl;
};

class libout::iterator {
public:
  iterator() : pimpl(NULL) {}
  
  iterator& operator++() { return *this; }
  iterator operator++(int) { return *this; }
  iterator& operator*() { return *this; }
  
  void operator=( library_entry const& entry ) { pimpl->append(entry); }
  
private:
  friend class libout;
  iterator( interface* p ) : pimpl(p) {}
  
  // Data members
  interface* pimpl;
};

inline libout::iterator libout::begin() { 
  return iterator( pimpl.get() ); 
}

inline libout::iterator libout::end() { 
  return iterator(); 
}


// An adaptor to output simulatenously in multiple ways
class multilibout : public libout {
public:
  multilibout();
  
  void add( libout* outputer ); // Takes ownership
  bool empty() const;

private:
  class impl;
};


RINGING_END_NAMESPACE

#endif // RINGING_LIBOUT_H
