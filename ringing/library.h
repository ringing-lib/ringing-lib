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
#else
#include <iostream>
#include <fstream>
#include <list>
#endif
#include <ringing/method.h>

#if _MSC_VER
// Something deep within the STL in Visual Studio decides to 
// turn this warning back on.  
#pragma warning(disable: 4231)
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class library;
class library_base;

// libtype : A type of library
class RINGING_API libtype {
protected:
  virtual library_base *open(const char *n) const  // Try to open this file.
  { return NULL; }			  // Return NULL if it's not the
					  // right sort of library.
  friend class library;
};

#if RINGING_AS_DLL
RINGING_EXPLICIT_STL_TEMPLATE list<libtype *>;
#endif

// newlib : Each new type of library should declare one of these
template <class mylibrary>
class newlib : public libtype {
protected:
  library_base *open(const char *name) const {
    if(mylibrary::canread(name)) {
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
  virtual method load(const char* name) = 0; // Load a method
  virtual int save(const method& m)	// Save a method
    { return 0; }
  virtual int rename(const string name1, const string name2)
    { return 0; }
  virtual int remove(const string name)
    { return 0; }
  virtual int dir(list<string>& result)
    { return 0; }
  virtual int good (void) const	// Is it in a usable state?
    { return 0; }
  virtual int writeable(void) const // Is it writeable?
    { return 0; }
};

class RINGING_API library {
private:
  library_base* lb;
  static list<libtype*> libtypes;

public:
  library(const char* filename);
  ~library() { if(lb) delete lb; }
  method load(const char* name) { return lb->load(name); }
  int save(const method& m) { return lb->save(m); }
  int rename(const string name1, const string name2) 
    { return lb->rename(name1, name2); }
  int remove(const string name) { return lb->remove(name); }
  int dir(list<string>& result) { return lb->dir(result); }
  bool good() { return lb && lb->good(); }
  int writeable() { return lb && lb->writeable(); }

  static void addtype(libtype* lt) { libtypes.push_back(lt); }
};

RINGING_END_NAMESPACE

#endif
