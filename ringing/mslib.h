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
#include <algorithm.h>
#include <fstream.h>
#else
#include <cctype>
#include <algorithm>
#include <fstream>
#endif
#include <ringing/library.h>
#include <string>

RINGING_START_NAMESPACE

RINGING_USING_STD

// mslib : Implement MicroSIRIL libraries
class mslib : public library_base {
private:
  fstream f;                    // The file stream we're using
  int b;                        // Number of bells for files in this lib
  int wr;                       // Is it open for writing?
  int _good;
  static newlib<mslib> type;    // Provide a handle to this library type

public:
  static RINGING_API void registerlib(void) {
    library::addtype(&type);
  }

  mslib(const string& name);
  ~mslib() { if (_good == 1) f.close(); }

private:
  friend class newlib<mslib>;
  
  // Is this file in the right format?
  static int canread(ifstream& ifs);

  // Return a list of items
  int dir(list<string>& result);

  int good(void) const          // Is the library in a usable state?
    { return _good; }

  int writeable(void) const     // Is this library writeable?
    { return wr; }

  method load(const string& name);     // Load a method
//int save(method& name);       // Save a method
};

RINGING_END_NAMESPACE

#endif
