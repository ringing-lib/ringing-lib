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

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999. 
//  Copyright (c) 2001, 2002 Peter Dimov

//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// $Id$

#ifndef RINGING_POINTERS_H
#define RINGING_POINTERS_H

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <algo.h>
#else
#include <algorithm>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_DETAILS_NAMESPACE

// A class that helps provide a safe conversion to bool, inspired by the 
// safe_bool operator in boost's function_base.hpp [ http://www.boost.org ].
class RINGING_API safe_bool
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

template <class T> 
struct delete_helper {
  static void fn( T *&ptr ) { delete ptr; ptr = NULL; }
};

// A smart pointer that calls a clone member function to perform a deep copy
// This class has value semantics and has separate const and non-const 
// accessors to propogates constness to its held pointer.
template <class T>
class cloning_pointer : private RINGING_DETAILS_PREFIX safe_bool
{
public:
  // Standard auto pointer typedefs
  typedef T element_type;
    
  // Accessors
  const element_type *operator->() const { return ptr; }
  const element_type &operator*() const { return *ptr; }
  const element_type *get() const { return ptr; }

  // Non-const accessors
  element_type *operator->() { return ptr; }
  element_type &operator*() { return *ptr; }
  element_type *get() { return ptr; }
    
  // Construction and destruction
  explicit cloning_pointer( T *src = 0 ) : ptr(src) {}
 ~cloning_pointer() { delete ptr; }

  // Swapping, assignment and copying
  void swap( cloning_pointer &o ) 
    { RINGING_PREFIX_STD swap( ptr, o.ptr ); }
  cloning_pointer( const cloning_pointer &o ) 
    : ptr( o.ptr ? o.ptr->clone() : 0 ) {}
  cloning_pointer &operator=( const cloning_pointer &o )
    { cloning_pointer<T>( o ).swap(*this); return *this; }

  // Reset the pointer
  void reset( T *x = 0 ) { delete ptr; ptr = x; } 

  // Safe boolean conversions
  operator safe_bool_t() const { return make_safe_bool( ptr ); }
  bool operator!() const { return !bool( *this ); }

  // Comparison operators.  
  // Needed if we want to put them in a STL container in MSVC-5.
  bool operator< ( const cloning_pointer<T> &o ) const { return ptr <  o.ptr; }
  bool operator> ( const cloning_pointer<T> &o ) const { return ptr >  o.ptr; }
  bool operator<=( const cloning_pointer<T> &o ) const { return ptr <= o.ptr; }
  bool operator>=( const cloning_pointer<T> &o ) const { return ptr >= o.ptr; }
  bool operator==( const cloning_pointer<T> &o ) const { return ptr == o.ptr; }
  bool operator!=( const cloning_pointer<T> &o ) const { return ptr != o.ptr; }

private:
  T *ptr;
};

// A smart pointer that reference counts its pointee and provides handle
// (reference) semantics. 
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
  explicit shared_pointer( T *src = 0 ) 
    : ptr( src )
  { 
# if RINGING_USE_EXCEPTIONS
    try { rc = new int(1); } 
    catch (...) { delete ptr; throw; } 
# else
    rc = new int(1);
# endif
  }
 ~shared_pointer() { if ( !--*rc ) { delete ptr; delete rc; } }
  
  // Swapping, assignment and copying
  void swap( shared_pointer<T> &o )
    { RINGING_PREFIX_STD swap( ptr, o.ptr ); 
      RINGING_PREFIX_STD swap( rc, o.rc ); }
  shared_pointer( const shared_pointer<T> &o )
    : ptr( o.ptr ) { ++*( rc = o.rc ); }
  shared_pointer &operator=( const shared_pointer<T> &o )
    { shared_pointer<T>( o ).swap(*this); return *this; }

  // Reset the pointer
  void reset( T *src = 0 ) 
  { 
    if ( ptr == src ) return;
    else if ( !--*rc ) { delete ptr; }
# if RINGING_USE_EXCEPTIONS
    else try { rc = new int(0); } 
    catch (...) { ++*rc; delete src; throw; } 
# else
    else rc = new int(0); 
# endif
    *rc = 1;
    ptr = src;
  }

  // Safe boolean conversions
  operator safe_bool_t() const { return make_safe_bool( ptr ); }
  bool operator!() const { return !bool( *this ); }

  // Comparison operators.  
  // Needed if we want to put them in a STL container in MSVC-5.
  bool operator< ( const shared_pointer<T> &o ) const { return ptr <  o.ptr; }
  bool operator> ( const shared_pointer<T> &o ) const { return ptr >  o.ptr; }
  bool operator<=( const shared_pointer<T> &o ) const { return ptr <= o.ptr; }
  bool operator>=( const shared_pointer<T> &o ) const { return ptr >= o.ptr; }
  bool operator==( const shared_pointer<T> &o ) const { return ptr == o.ptr; }
  bool operator!=( const shared_pointer<T> &o ) const { return ptr != o.ptr; }

private:
  T *ptr;
  mutable int *rc;
};


RINGING_START_DETAILS_NAMESPACE

// Never use this directly
template <class T> 
struct auto_delete_helper {
  static void fn( T *&ptr ) { delete ptr; ptr = NULL; }
};

RINGING_END_DETAILS_NAMESPACE

// A smart pointer that prohibits copying, but ensures the destructor
// is correctly called.  Based on boost's scoped_ptr.
template <class T>
class scoped_pointer : private RINGING_DETAILS_PREFIX safe_bool
{
public:
  // Standard auto pointer typedefs
  typedef T element_type;
  typedef void (*deletor_type)( element_type *& );

  // Accessors
  element_type *operator->() const { return ptr; }
  element_type &operator*() const { return *ptr; }
  element_type *get() const { return ptr; }

  // Construction and destruction
  // 
  // Note: there is some magic going on here to get it all working
  // correctly in MSVC 5.  Normally, you can completely ignore the
  // second argument to these constructors, however, in code that 
  // needs to compile in MSVC 5, if the class is used on an incomplete 
  // type (e.g. in the pimpl idiom), pass delete_helper<T>::fn
  // as the second argument and ignore all the warnings of the 
  // 
  explicit scoped_pointer( T *src, deletor_type d )
    : ptr( src ), deletor(d) 
  {}
  explicit scoped_pointer( T *src = 0 )
    : ptr( src ), 
      deletor( RINGING_DETAILS_PREFIX auto_delete_helper<T>::fn )
  {}

 ~scoped_pointer() { deletor(ptr); }

  // Reset the pointer
  void reset( T *x = 0 ) { deletor(ptr); ptr = x; }

  // Release the pointer.  Deleting it is now the caller's responsibility.
  element_type *release() { T *tmp = ptr; ptr = 0; return tmp; }

  // Safe boolean conversions
  operator safe_bool_t() const { return make_safe_bool( ptr ); }
  bool operator!() const { return !bool( *this ); }

  // Comparison operators.  
  // Needed if we want to put them in a STL container in MSVC-5.
  bool operator< ( const scoped_pointer<T> &o ) const { return ptr <  o.ptr; }
  bool operator> ( const scoped_pointer<T> &o ) const { return ptr >  o.ptr; }
  bool operator<=( const scoped_pointer<T> &o ) const { return ptr <= o.ptr; }
  bool operator>=( const scoped_pointer<T> &o ) const { return ptr >= o.ptr; }
  bool operator==( const scoped_pointer<T> &o ) const { return ptr == o.ptr; }
  bool operator!=( const scoped_pointer<T> &o ) const { return ptr != o.ptr; }

private:
  // Assignment and copying disabled
  scoped_pointer( const scoped_pointer<T> &o );
  void operator=( const scoped_pointer<T> &o );

private:
  T *ptr;
  void (*deletor)( element_type *&ptr );
};



// A smart pointer somewhere between cloning_pointer and shared_pointer. 
// Used correctly it provides value semantics, but, unlike cloning_pointer
// it provides the copy-on-write optimisation.  
//
// Note:  Because the language provides no way to differentiate calls to
// const and non-const member functions of the pointee, the class is only
// effective when the cow_pointer has the same constness as the member 
// function to be called.  This is typically the case when cow_pointers are 
// used to implement the pimpl idiom with lightweight forwarder functions
// to the implementation class.
template <class T> 
class cow_pointer : private RINGING_DETAILS_PREFIX safe_bool
{
public:
  // Standard auto pointer typedefs
  typedef T element_type;
    
  // Const accessors 
  const element_type *operator->() const { return ptr; }
  const element_type &operator*() const { return *ptr; }
  const element_type *get() const { return ptr; }

  // Non-const accessors.  Trigger a deep copy.
  element_type *operator->() { clone(); return ptr; }
  element_type &operator*() { clone(); return *ptr; }
  element_type *get() { clone(); return ptr; }

  // Construction and destruction
  explicit cow_pointer( T *src = 0 ) 
    : ptr(src) 
  {
# if RINGING_USE_EXCEPTIONS
    try { rc = new int(1); } 
    catch (...) { delete ptr; throw; } 
# else
    rc = new int(1);
# endif
  }
 ~cow_pointer() { if ( !--*rc ) { delete ptr; delete rc; } }

  // Swapping, assignment and copying
  void swap( cow_pointer<T> &o )
    { std::swap( ptr, o.ptr ); 
      std::swap( rc, o.rc ); }
  cow_pointer( const cow_pointer<T> &o )
    : ptr( o.ptr ) { ++*( rc = o.rc ); }
  cow_pointer &operator=( const cow_pointer<T> &o )
    { cow_pointer<T>( o ).swap(*this); return *this; }

  // Reset the pointer
  void reset( T *src = 0 ) 
  { 
    if ( ptr == src ) return;
    else if ( !--*rc ) { delete ptr; }
# if RINGING_USE_EXCEPTIONS
    else try { rc = new int(0); } 
    catch (...) { ++*rc; delete src; throw; } 
# else
    else rc = new int(0); 
# endif
    *rc = 1;
    ptr = src;
  }

  // Force a deep copy to happen if the refcount isn't 1.
  void clone() { if ( *rc > 1 && ptr ) reset( ptr->clone() ); }

  // Safe boolean conversions
  operator safe_bool_t() const { return make_safe_bool( ptr ); }
  bool operator!() const { return !bool( *this ); }

  // Comparison operators.  
  // Needed if we want to put them in a STL container in MSVC-5.
  bool operator< ( const cow_pointer<T> &o ) const { return ptr <  o.ptr; }
  bool operator> ( const cow_pointer<T> &o ) const { return ptr >  o.ptr; }
  bool operator<=( const cow_pointer<T> &o ) const { return ptr <= o.ptr; }
  bool operator>=( const cow_pointer<T> &o ) const { return ptr >= o.ptr; }
  bool operator==( const cow_pointer<T> &o ) const { return ptr == o.ptr; }
  bool operator!=( const cow_pointer<T> &o ) const { return ptr != o.ptr; }

private:
  T *ptr;
  mutable int *rc;
};


RINGING_START_DETAILS_NAMESPACE

template <class T>
class operator_arrow_proxy 
{
private:
  typedef T proxy_type;

public:
  operator_arrow_proxy( const proxy_type &c ) : c(c) {}
  const proxy_type *operator->() const { return &c; }
  operator const proxy_type *() const { return &c; }

private:
  proxy_type c;
};

RINGING_END_DETAILS_NAMESPACE

RINGING_END_NAMESPACE

#endif
