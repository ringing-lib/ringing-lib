// -*- C++ -*- cclib.h - Central Council Method libraries
// Copyright (C) 2001 Mark Baner <mark@standard8.co.uk>

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

#ifndef RINGING_CCLIB_H
#define RINGING_CCLIB_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#else
#include <cctype>
#endif
#if RINGING_OLD_INCLUDES
#include <algo.h>
#include <fstream.h>
#else
#include <algorithm>
#include <fstream>
#endif
#include <ringing/library.h>
#include <string>

RINGING_START_NAMESPACE

#undef SEPERATE_FILES

RINGING_USING_STD

// cclib : Implement Central Council Method libraries
class cclib : public library_base {
private:
  ifstream f;                   // The file stream we're using
  int b;                        // Number of bells for files in this lib
  int wr;                       // Is it open for writing?
  int _good;                    // If we have a good filename or not.

  // Extracts the number of bells from the filename.
  static int extractNumber(const string&);

public:
  static RINGING_API void registerlib(void) {
    library::addtype(&canread);
  }
#if defined(SEPERATE_FILES)
  static int seperatefiles(const string&);
#endif

private:
  // Construction handled by library class
  cclib(const string& filename);
 ~cclib() { if (_good == 1) f.close(); }

  // Is this file in the right format?
  static RINGING_API library_base *canread(const string& filename);

  // Iterators into the library
  class entry_type;
  friend class entry_type;
  virtual const_iterator begin() const;

  // Is the library in a usable state?
  virtual bool good(void) const { return _good; }

  // Is this library writeable?
  virtual bool writeable(void) const { return wr; }
};

RINGING_END_NAMESPACE

#endif
