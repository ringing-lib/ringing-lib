// -*- C++ -*- library.h - Things for method libraries
// Copyright (C) 2001, 2002 Martin Bright <martin@boojum.org.uk>
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

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <iosfwd.h>
#include <list.h>
#include <stdexcept.h>
#include <iterator.h>
#else
#include <iosfwd>
#include <list>
#include <stdexcept>
#include <iterator>
#endif
#include <string>
#include <ringing/method.h>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class library;
class library_base;
class library_entry;

// library_base : A base class for method libraries
class RINGING_API library_base {
public:
  virtual ~library_base() {}		// Got to have a virtual destructor

  // Reading the library.
  virtual method load(const string& s, int stage) const; // Load a method
  virtual int dir(list<string>& result) const;
  virtual int mdir(list<method>& result) const;

  // Writing to the library
  virtual bool save(const method& m)	// Save a method
    { return false; }
  virtual bool rename_method(const string& name1, const string& name2)
    { return false; }
  virtual bool remove(const string& name)
    { return false; }

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
RINGING_EXPLICIT_STL_TEMPLATE list< library_base *(*)( ifstream &, 
						       const string & ) >;
RINGING_EXPLICIT_RINGING_TEMPLATE shared_pointer<library_base>;
RINGING_EXPLICIT_RINGING_TEMPLATE cloning_pointer<library_entry>;
#endif

// library_entry : A base class for entries from libraries
class RINGING_API library_entry {
public:
  library_entry() {}

  // Library implementations should subclass this
  class RINGING_API impl
  {
  public:
    virtual ~impl() {}
    virtual impl *clone() const = 0;

    virtual string name() const = 0;
    virtual string base_name() const = 0;
    virtual string pn() const = 0;
    virtual int bells() const = 0;
    
    virtual bool readentry( library_base &lb ) = 0;
  };

  // Public accessor functions
  string name() const      { return pimpl->name(); }
  string base_name() const { return pimpl->base_name(); }
  string pn() const        { return pimpl->pn(); }
  int bells() const        { return pimpl->bells(); }
  method meth() const      { return method( pn(), bells(), base_name() ); }

private:
  friend class library_base::const_iterator;
  library_entry( impl *pimpl ) : pimpl(pimpl) {}

  cow_pointer< impl > pimpl;
};

class RINGING_API library_base::const_iterator
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
    : lb(lb), val(val), ok( lb && val && val->readentry(*lb) ) 
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
    { ok = val.pimpl->readentry(*lb); return *this; }
  const_iterator operator++(int) 
    { const_iterator tmp(*this); ++*this; return tmp; }

private:
  // Data members
  library_base *lb;
  library_entry val;
  bool ok;
};

class RINGING_API library {
public:
  // Construction
  library() {}
  library(const string& filename);
  library(library_base* lb) : lb(lb) {}

  // Reading the library.
  method load(const string& name, int stage=0) const
    { return lb->load(name, stage); }
  int dir(list<string>& result) const { return lb->dir(result); }
  int mdir(list<method>& result ) const { return lb->mdir(result); }

  // Writing to the library
  bool save(const method& m) { return lb->save(m); }
  bool rename_method(const string& name1, const string& name2) 
    { return lb->rename_method(name1, name2); }
  bool remove(const string name) { return lb->remove(name); }

  // Library status
  bool good() { return bool(lb) && lb->good(); }
  bool writeable() { return bool(lb) && lb->writeable(); }

  // New style, iterator based interface
  typedef library_base::const_iterator const_iterator;
  const_iterator begin() const { return lb ? lb->begin() : const_iterator(); }
  const_iterator end() const { return lb ? lb->end() : const_iterator(); }

  // Register a new type of library
  typedef library_base *(*init_function)( const string & );
  static void addtype(init_function lt) { libtypes.push_back(lt); }

private:
  shared_pointer<library_base> lb;
  static list<init_function> libtypes;
};

RINGING_END_NAMESPACE

#endif
