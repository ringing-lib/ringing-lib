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
#include <ringing/stuff.h>
RINGING_USING_STD

RINGING_START_NAMESPACE

class library;

// libtype : A type of library
class libtype {
public:
  virtual library *open(ifstream& f, char *n) const  // Try to open this file.
    { return NULL; }			  // Return NULL if it's not the
					  // right sort of library.
};

// newlib : Each new type of library should declare one of these
template <class mylibrary>
class newlib : public libtype {
public:
  library *open(ifstream& f, char *name) const {
    if(mylibrary::canread(f)) {
      f.close();
      return new mylibrary(name);
    } else
      return NULL;
  }
};

// library : A base class for method libraries
class library {
private:
  static libtype *libtypes[];	// List of all library types
public:
  virtual ~library() {}		// Got to have a virtual destructor
  virtual method *load(char *name) // Load a method
    { return NULL; }
  virtual int save(method& m)	// Save a method
    { return 0; }
  virtual int rename(char *name1, char *name2)
    { return 0; }
  virtual int remove(char *name)
    { return 0; }
  virtual int dir(list<string>& result)
    { return 0; }
  virtual int good (void) const	// Is it in a usable state?
    { return 0; }
  virtual int writeable(void) const // Is it writeable?
    { return 0; }
  static library *open(char *name) // Open a library
  {
    ifstream f(name);
    if(!f) return NULL;
    int i;
    library *l;
    for(i = 0; libtypes[i] != NULL; i++) {
      if((l = libtypes[i]->open(f, name)) != NULL) return l;
    }
    return NULL;
  }
};

RINGING_END_NAMESPACE

#endif
