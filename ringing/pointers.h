// -*- C++ -*- pointers.h - A few smart pointer classes
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

// Parts of this file are taken from the boost smart_ptr library [see
// http://www.boost.org for details], and are under the following copyright:

// (C) Copyright Greg Colvin and Beman Dawes 1998, 1999. Permission to copy,
// use, modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided "as is"
// without express or implied warranty, and with no claim as to its
// suitability for any purpose.

// $Id$

#ifndef RINGING_POINTERS_H
#define RINGING_POINTERS_H

#include <ringing/common.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_DETAILS_NAMESPACE

// A class that helps provide a safe conversion to bool, inspired by the 
// safe_bool operator in boost's function_base.hpp [ http://www.boost.org ].
class safe_bool
{
private:
  struct dummy
  {
    void nonnull() {} 
  };

protected:
  // Making the safe_bool type a member-pointer disables almost
  // all built-in operators.  The only remaining ones are disabled below.
  typedef void (dummy::*safe_bool_t)();
  
  static safe_bool_t make_safe_bool( bool b ) 
  {
    return b ? &dummy::nonnull : 0; 
  }
  
  private:
  // Private operator==, != to disable comparison via operator safe_bool.
  // ( The return type is void because MSVC5 does not respect access specifiers
  // on member operators. )
  void operator==( const safe_bool & ) {}
  void operator!=( const safe_bool & ) {}
};

RINGING_END_DETAILS_NAMESPACE

// A smart pointer that calls a clone member function to perform the copy
template <class T>
class cloning_pointer : private RINGING_DETAILS_PREFIX safe_bool
{
public:
  // Standard auto pointer typedefs
  typedef T element_type;
    
  // Accessors 
  element_type *operator->() const { return ptr; }
  element_type &operator*() const { return *ptr; }
  element_type *get() const { return ptr; }
    
  // Construction and destruction
  cloning_pointer( T *src = 0 ) : ptr(src) {}
 ~cloning_pointer() { delete ptr; }

  // Swapping, assignment and copying
  void swap( cloning_pointer &o ) 
    { RINGING_PREFIX_STD swap( ptr, o.ptr ); }
  cloning_pointer( const cloning_pointer &o ) 
    : ptr( o.ptr ? o.ptr->clone() : 0 ) {}
  cloning_pointer &operator=( const cloning_pointer &o )
    { cloning_pointer( o ).swap(*this); return *this; }

  // Reset the pointer
  void reset( T *x = 0 ) { delete ptr; ptr = x; } 

  // Safe boolean conversions
  operator safe_bool_t() const { return make_safe_bool( ptr ); }
  bool operator!() const { return !bool( *this ); }

private:
  T *ptr;
};

// A smart pointer that reference counts its pointee 
// Based on boost's shared_ptr, but simplified to compile under MSVC-5.
template <class T>
class shared_pointer : private RINGING_DETAILS_PREFIX safe_bool
{
public:
  // Standard auto pointer typedefs
  typedef T element_type;

  // Accessors
  element_type *operator->() const { return ptr; }
  element_type &operator*() const { return *ptr; }
  element_type *get() const { return ptr; }

  // Construction and destruction
  shared_pointer( T *src = 0 ) 
    : ptr( src )
  { 
    try { rc = new int(1); } 
    catch (...) { delete ptr; throw; } 
  }
 ~shared_pointer() { if ( !--*rc ) { delete ptr; delete rc; } }
  
  // Swapping, assignment and copying
  void swap( shared_pointer &o )
    { RINGING_PREFIX_STD swap( ptr, o.ptr ); 
      RINGING_PREFIX_STD swap( rc, o.rc ); }
  shared_pointer( const shared_pointer &o )
    : ptr( o.ptr ) { ++*( rc = o.rc ); }
  shared_pointer &operator=( const shared_pointer &o )
    { shared_pointer( o ).swap(*this); return *this; }

  // Reset the pointer
  void reset( T *src = 0 ) 
  { 
    if ( ptr == src ) return;
    else if ( !--*rc ) { delete ptr; }
    else try { rc = new int; } 
    catch (...) { ++*rc; delete src; throw; } 
    *rc = 1;
    ptr = src;
  }

  // Safe boolean conversions
  operator safe_bool_t() const { return make_safe_bool( ptr ); }
  bool operator!() const { return !bool( *this ); }

private:
  T *ptr;
  mutable int *rc;
};

// An STL functor to delete pointers
struct delete_pointers
{
  template <class T>
  void operator()( T *&ptr ) const { delete ptr; ptr = NULL; }
};


RINGING_END_NAMESPACE

#endif
