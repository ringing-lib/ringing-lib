// -*- C++ -*- library.h - Things for method libraries
// Copyright (C) 2001 Martin Bright <M.Bright@dpmms.cam.ac.uk>

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

#ifndef RINGING_LIBRARY_H
#define RINGING_LIBRARY_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <fstream.h>
#include <list.h>
#include <stdexcept.h>
#else
#include <iostream>
#include <fstream>
#include <list>
#include <stdexcept>
#endif
#include <string>
#include <ringing/method.h>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class library;
class library_base;

// libtype : A type of library
class libtype {
protected:
  virtual library_base *open(ifstream& ifs, const string &n) const  // Try to open this file.
  { return NULL; }			  // Return NULL if it's not the
					  // right sort of library.
  friend class library;
  };

// newlib : Each new type of library should declare one of these
template <class mylibrary>
class newlib : public libtype {
protected:
  library_base *open(ifstream& ifs, const string &name) const {
    if(mylibrary::canread(ifs)) {
      return new mylibrary(name);
    } else
      return NULL;
  }
  friend class library;
};

// library_base : A base class for method libraries
class RINGING_API library_base {
public:
  virtual ~library_base() {}		// Got to have a virtual destructor
  virtual method load(const string& s) = 0; // Load a method
  virtual int save(const method& m)	// Save a method
    { return 0; }
  virtual int rename_method(const string& name1, const string& name2)
    { return 0; }
  virtual int remove(const string& name)
    { return 0; }
  virtual int dir(list<string>& result) // Return a list of items
    { return 0; }
  virtual int good (void) const	// Is it in a usable state?
    { return 0; }
  virtual int writeable(void) const // Is it writeable?
    { return 0; }

#if RINGING_USE_EXCEPTIONS
  struct invalid_name : public invalid_argument {
    invalid_name();
  };
#endif
};

#if RINGING_AS_DLL
RINGING_EXPLICIT_STL_TEMPLATE list<libtype*>;
#endif

class RINGING_API library {
private:
  shared_pointer<library_base> lb;
  static list<libtype*> libtypes;

protected:
  library(library_base* lb) : lb(lb) {}
public:
  library(const string& filename = "");
  method load(const string& name) { return lb->load(name); }
  method load(const char* name) { return lb->load(string(name)); }
  int save(const method& m) { return lb->save(m); }
  int rename_method(const string name1, const string name2) 
    { return lb->rename_method(name1, name2); }
  int remove(const string name) { return lb->remove(name); }
  int dir(list<string>& result) { return lb->dir(result); }
  bool good() { return bool(lb) && lb->good(); }
  int writeable() { return bool(lb) && lb->writeable(); }

  static void addtype(libtype* lt) { libtypes.push_back(lt); }
};

RINGING_END_NAMESPACE

#endif
