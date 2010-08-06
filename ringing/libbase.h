// -*- C++ -*- libbase.h - base class for both library and libout
// Copyright (C) 2001, 2002, 2004, 2009, 2010 
// Martin Bright <martin@boojum.org.uk>
// and Richard Smith <richard@ex-parrot.com>.

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

#ifndef RINGING_LIBBASE_H
#define RINGING_LIBBASE_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <string>
#include <ringing/pointers.h>
#include <ringing/libfacet.h>
#include <ringing/method.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class library_entry;
class library_base;

RINGING_START_DETAILS_NAMESPACE
struct call_readentry;
RINGING_END_DETAILS_NAMESPACE
 
class libbase {
public:
  typedef library_entry value_type;
  typedef library_entry const& const_reference;

  class interface {
  public:
    virtual ~interface() {}
  };

RINGING_PROTECTED_IMPL:
  libbase() {}

  void set_impl( interface* p ) { if (pimpl.get() != p) pimpl.reset(p); }

  template <class ImplType> ImplType* get_impl( ImplType* = 0 ) {
    return dynamic_cast<ImplType*>(pimpl.get()); 
  }

  template <class ImplType> ImplType const* get_impl( ImplType* = 0 ) const { 
    return dynamic_cast<ImplType const*>(pimpl.get()); 
  }

private:
  shared_pointer<interface> pimpl;
};

// library_entry : A base class for entries from libraries
class RINGING_API library_entry {
public:
  library_entry() {}

  // Library implementations should subclass this
  class RINGING_API impl {
  public:
    virtual ~impl() {}
    virtual impl *clone() const = 0;

    virtual string name() const = 0;
    virtual string base_name() const = 0;
    virtual string pn() const = 0;
    virtual int bells() const = 0;
    virtual method meth() const;
    
    virtual bool readentry( library_base &lb ) = 0;

    virtual bool has_facet( const library_facet_id& id ) const;

    virtual shared_pointer< library_facet_base > 
      get_facet( const library_facet_id& id ) const;

  };

  // Public accessor functions
  string name() const      { return pimpl->name(); }
  string base_name() const { return pimpl->base_name(); }
  string pn() const        { return pimpl->pn(); }
  int bells() const        { return pimpl->bells(); }
  method meth() const      { return pimpl->meth(); }

  // Get an extended property of the method 
  template <class Facet>
  typename Facet::type get_facet( Facet const* = NULL ) const 
  {
    shared_pointer< library_facet_base > f( pimpl->get_facet( Facet::id ) );
    if ( !f.get() ) throw runtime_error( "No such facet" );
	typedef typename Facet::type facet_type;
    return facet_type( static_cast< const Facet& >( *f ) );
  }

  template <class Facet>
  bool has_facet( Facet const* = NULL ) const 
  {
    return pimpl->has_facet( Facet::id );
  }

  // As above, but taking an id instead of a template parameter
  shared_pointer< library_facet_base > 
      get_facet( const library_facet_id& id ) const
  { return pimpl->get_facet( id ); }

  bool has_facet( const library_facet_id& id ) const
  { return pimpl->has_facet( id ); }



  library_entry( impl *pimpl ) : pimpl(pimpl) {}

  // As we have a default constructor that creates null library_entry objects,
  // we ought to be able to test for such objects.
  bool null() const { return !pimpl; }

protected:
  template <class ImplType> ImplType* get_impl( ImplType* = 0 ) {
    return static_cast<ImplType*>(pimpl.get()); 
  }

  template <class ImplType> ImplType const* get_impl( ImplType* = 0 ) const { 
    return static_cast<ImplType const*>(pimpl.get()); 
  }

private:
  friend struct RINGING_DETAILS_PREFIX call_readentry;

  cow_pointer< impl > pimpl;
};

 
RINGING_END_NAMESPACE

#endif // RINGING_LIBBASE_H
