// library.h - Things for method libraries

#ifndef METHLIB_LIBRARY_H
#define METHLIB_LIBRARY_H

#ifdef __GNUG__
#pragma interface
#endif

#include <iostream.h>
#include <fstream.h>
#include <list.h>
#include "method.h"
#include "stuff.h"

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

#endif
