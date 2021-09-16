// -*- C++ -*- libfacet.h - Library extensibility mechanism
// Copyright (C) 2004, 2010, 2021 Richard Smith <richard@ex-parrot.com>.

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

#ifndef RINGING_LIBFACET_H
#define RINGING_LIBFACET_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class RINGING_API library_facet_id {
public:
  library_facet_id();
  
  library_facet_id( const library_facet_id& o ) { *this = o; }
  library_facet_id &operator=( const library_facet_id & );

private:
  static int assign_id();
  friend bool operator==( const library_facet_id &, const library_facet_id & );
  friend bool operator<( const library_facet_id &, const library_facet_id & );
  mutable int id; 
};

class RINGING_API library_facet_base {
public:
  virtual ~library_facet_base() {}
  virtual library_facet_id const& get_id() = 0;
};

RINGING_API
bool operator==( const library_facet_id &a, const library_facet_id &b );

RINGING_API
bool operator<( const library_facet_id &a, const library_facet_id &b );

inline RINGING_API
bool operator!=( const library_facet_id &a, const library_facet_id &b )
{ return !( a == b ); }

inline RINGING_API
bool operator>( const library_facet_id &a, const library_facet_id &b )
{ return b < a; }

inline RINGING_API
bool operator<=( const library_facet_id &a, const library_facet_id &b )
{ return !( b < a ); }

inline RINGING_API
bool operator>=( const library_facet_id &a, const library_facet_id &b )
{ return !( a < b ); }


// Macro to assist in declaring facets.
#define RINGING_DECLARE_LIBRARY_FACET( FACET_NAME, TYPE )		\
  class RINGING_API FACET_NAME : public library_facet_base		\
  {									\
  public:								\
    typedef TYPE type;							\
    FACET_NAME( const type &t ) : t(t) {}				\
    operator const type &() const { return t; }				\
    static const library_facet_id id;					\
    virtual library_facet_id const& get_id() { return id; }             \
  private:								\
    static bool do_force_init();					\
    static const bool force_init;					\
    type t;								\
  }

// Corresponding out-of-line definitions
#define RINGING_DEFINE_LIBRARY_FACET( FACET_NAME )			\
  bool FACET_NAME::do_force_init() { return true;			\
    /* Fixes some problems with DLL exporting */			\
    library_entry().get_facet( (FACET_NAME const*)0 );			\
  }									\
  const library_facet_id FACET_NAME::id;				\
  const bool FACET_NAME::force_init = FACET_NAME::do_force_init()	\

RINGING_END_NAMESPACE

#endif
