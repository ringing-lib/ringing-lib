// -*- C++ -*- library.h - Things for method libraries
// Copyright (C) 2001, 2002, 2004, 2009, 2017 
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

#ifndef RINGING_LIBRARY_H
#define RINGING_LIBRARY_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <list.h>
#include <stdexcept.h>
#include <iterator.h>
#else
#include <list>
#include <stdexcept>
#include <iterator>
#endif
#include <string>
#include <ringing/method.h>
#include <ringing/pointers.h>
#include <ringing/libbase.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class library;
class library_base;
class library_entry;

// library_base : A base class for method libraries
class RINGING_API library_base : public virtual libbase::interface {
public:
  virtual ~library_base() {}		// Got to have a virtual destructor

  // Reading the library.
  virtual method load(const string& s, int stage) const; // Load a method
  virtual library_entry find(const method& pn) const; // Load by pn
  virtual int dir(list<string>& result) const;
  virtual int mdir(list<method>& result) const;

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
  // Writing to the library
  virtual bool save(const method& m)	// Save a method
    { return false; }
  virtual bool rename_method(const string& name1, const string& name2)
    { return false; }
  virtual bool remove(const string& name)
    { return false; }
#endif

  // Library status
  virtual bool good (void) const = 0;	// Is it in a usable state?
  virtual bool writeable(void) const    // Is it writeable?
    { return false; }

  // The new style library interface uses iterators
  class const_iterator;
  virtual const_iterator begin() const = 0;
  const_iterator end() const;

#if RINGING_USE_EXCEPTIONS
  struct invalid_name : public invalid_argument {
    invalid_name();
  };
#endif
};

#if RINGING_AS_DLL
RINGING_EXPLICIT_STL_TEMPLATE list< library_base *(*)( const string & ) >;
#endif

RINGING_START_DETAILS_NAMESPACE
struct call_readentry {
  static inline bool fn( library_entry& le, library_base &lb ) {
    return le.pimpl->readentry(lb);
  }
};
RINGING_END_DETAILS_NAMESPACE
 

class RINGING_API library_base::const_iterator 
  : public RINGING_STD_CONST_ITERATOR( input_iterator_tag, library_entry )
{
public:
  // Standard iterator typedefs
  typedef library_entry value_type;
  typedef input_iterator_tag iterator_category;
  typedef ptrdiff_t difference_type;
  typedef const value_type &reference;
  typedef const value_type *pointer;

  // Construction
  const_iterator() 
    : lb(NULL), ok(false)
  {}
  const_iterator( library_base *lb, library_entry::impl *val ) 
    : lb(lb), val(val), 
      ok( lb && val && 
          RINGING_DETAILS_PREFIX call_readentry::fn(this->val, *lb) ) 
  {}

  // Equality Comparable requirements
  bool operator==( const const_iterator &i ) const
    { return ok ? (i.ok && lb == i.lb) : !i.ok; }
  bool operator!=( const const_iterator &i ) const
    { return !operator==( i ); }

  // Trivial Iterator requirements
  value_type operator*() const { return val; }
  pointer operator->() const { return &val; }

  // Input Iterator requirements
  const_iterator &operator++() 
    { ok = RINGING_DETAILS_PREFIX call_readentry::fn(val, *lb); return *this; }
  const_iterator operator++(int) 
    { const_iterator tmp(*this); ++*this; return tmp; }

private:
  // Data members
  library_base *lb;
  library_entry val;
  bool ok;
};

class RINGING_API library : public virtual libbase {
public:
  // Construction
  library() {}
  library(const string& filename);
  library(library_base* i) { this->set_impl(i); }

  // Reading the library.
  method load(const string& name, int stage=0) const
    { return lb()->load(name, stage); }
  library_entry find(const method& pn) const { return lb()->find(pn); }
  int dir(list<string>& result) const { return lb()->dir(result); }
  int mdir(list<method>& result ) const { return lb()->mdir(result); }

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
  // Writing to the library
  bool save(const method& m) { return lb()->save(m); }
  bool rename_method(const string& name1, const string& name2) 
    { return lb()->rename_method(name1, name2); }
  bool remove(const string name) { return lb()->remove(name); }
#endif

  // Library status
  bool good() { return bool(lb()) && lb()->good(); }
  bool writeable() { return bool(lb()) && lb()->writeable(); }

  // New style, iterator based interface
  typedef library_base::const_iterator const_iterator;
  const_iterator begin() const 
    { return lb() ? lb()->begin() : const_iterator(); }
  const_iterator end() const 
    { return lb() ? lb()->end() : const_iterator(); }

  bool empty() const { return begin() == end(); }

  // Register a new type of library
  typedef library_base *(*init_function)( const string & );
  static void addtype(init_function lt);

  // Set the path (a colon delimited string) in which we look for libraries
  static void setpath(string const& p);

private:
  library_base* lb()
    { return this->libbase::get_impl<library_base>(); }
  library_base const* lb() const 
    { return this->libbase::get_impl<library_base>(); }

  bool try_load_lib( string const& filename );

  static list<init_function> libtypes;
  static string libpath;
};



// cc_collection_id is a general purpose ID for a number in printed 
// method collections.  
RINGING_DECLARE_LIBRARY_FACET( cc_collection_id, string );
// For licensing reasons, we want this out of cclib.h (which is GPL'd)


RINGING_END_NAMESPACE

#endif
