// mslib.h - MicroSIRIL libraries

#ifndef METHLIB_MSLIB_H
#define METHLIB_MSLIB_H

#ifdef __GNUG__
#pragma interface
#endif

#include <fstream.h>
#include <ctype.h>
#include "library.h"

// mslib : Implement MicroSIRIL libraries
class mslib : public library {
private:
  fstream f;                    // The iostream we're using
  int b;                        // Number of bells for files in this lib
  int wr;                       // Is it open for writing?

public:
  static newlib<mslib> type;    // Provide a handle to this library type

  mslib(char *name) : wr(0) {
    f.open(name, ios::in | ios::out);
    if(f) wr = 1; else f.open(name, ios::in);
    char *s;
    // Get the number off the end of the file name
    for(s = name + strlen(name) - 1; s > name && isdigit(s[-1]); s--);
    b = atoi(s);
    if(b == 0) f.close();
  }
  ~mslib() {}

  static int canread(ifstream& ifs) // Is this file in the right format?
    { return 1; }

  int good(void) const          // Is the library in a usable state?
    { return !!f; }

  int writeable(void) const     // Is this library writeable?
    { return wr; }

  method *load(char *name);     // Load a method
//int save(method& name);       // Save a method

  static int expand_pn(char *in, char *out, char *lh); // Expand place notation
};

#endif
