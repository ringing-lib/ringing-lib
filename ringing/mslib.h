// -*- C++ -*- mslib.h - MicroSIRIL libraries
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

#ifndef RINGING_MSLIB_H
#define RINGING_MSLIB_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#else
#include <cctype>
#endif
#include <ringing/library.h>
RINGING_USING_STD

RINGING_START_NAMESPACE

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
    if(f.good()) wr = 1; else f.open(name, ios::in);
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
};

RINGING_END_NAMESPACE

#endif
