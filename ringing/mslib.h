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
#include <stdexcept.h>
#else
#include <cctype>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#endif
#include <ringing/library.h>
#include <string>

RINGING_START_NAMESPACE

RINGING_USING_STD

// mslib : Implement MicroSIRIL libraries
class mslib : public library_base {
private:
  fstream f;                    // The iostream we're using
  int b;                        // Number of bells for files in this lib
  int wr;                       // Is it open for writing?

public:
  static newlib<mslib> type;    // Provide a handle to this library type

  mslib(const char *name) : wr(0) {
    f.open(name, ios::in | ios::out);
    if(f.good()) wr = 1; else f.open(name, ios::in);
    const char *s;
    // Get the number off the end of the file name
    for(s = name + strlen(name) - 1; s > name && isdigit(s[-1]); s--);
    b = atoi(s);
    if(b == 0) f.close();
  }
  ~mslib() {
    f.close();
  }

  // Is this file in the right format?
  static int canread(const char* const name)
    {
      ifstream ifs(name);
      if (ifs.good())
	{
	  int valid = 0;
	  int notvalid = 0;
	  ifs.seekg(0, ios::beg);
	  while (ifs.good() && (notvalid != 1))
	    {
	      string linebuf;
	      getline(ifs, linebuf);
	      if (linebuf.length() > 1)
		{
		  if ((linebuf.find("Name") != -1) && (linebuf.find("No.") != -1))
		    {
		      notvalid = 1;
		    }
		  else if (linebuf[0] != '*')
		    {
		      valid = (count(linebuf.begin(), linebuf.end(), ' ') == 2 ? 1 : 0);
		    }
		}
	    }
	  ifs.close();
	  return (notvalid == 1 ? 0 : valid);
	}
      return 0;
    }

  int good(void) const          // Is the library in a usable state?
    { return !!f; }

  int writeable(void) const     // Is this library writeable?
    { return wr; }

  method load(const char *name);     // Load a method
//int save(method& name);       // Save a method

#if RINGING_USE_EXCEPTIONS
  struct invalid_name : public invalid_argument {
    invalid_name();
  };
#endif

};

RINGING_END_NAMESPACE

#endif
