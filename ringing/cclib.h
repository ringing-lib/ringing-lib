// -*- C++ -*- mslib.h - Central Council Method libraries
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

// cclib : Implement Central Council Method libraries
class cclib : public library {
private:
  fstream f;                    // The iostream we're using
  int b;                        // Number of bells for files in this lib
  int wr;                       // Is it open for writing?

public:
  static newlib<cclib> type;    // Provide a handle to this library type

  cclib(const char *name) : wr(0) {
    f.open(name, ios::in | ios::out);
    if(f.good()) wr = 1; else f.open(name, ios::in);
    const char *s;
    // Get the number off the end of the file name
    // Is there a '.'? e.g. '.txt', if so account for it
    const char* last = find(name, name + strlen(name) - 1, '.');
    // now start to reverse from last.
    for(s = last - 1; s > name && isdigit(s[-1]); s--);
    b = atoi(s);
    if(b == 0) f.close();
  }
  ~cclib() {}

  static int canread(ifstream& ifs) // Is this file in the right format?
  {
    int valid = 0;
    int temp = -1;
    while ((ifs.good()) && (valid < 2))
      {
	string linebuf;
	getline(ifs, linebuf);
	if (linebuf.length() > 1)
	  {
	    // The second check for No. is used as an extra insurance check...
	    if ((linebuf.find("Name") != -1) && (linebuf.find("No.") != -1))
	      {
		temp = linebuf.find("Name") - 1;
		valid++;
	      }
	    else if ((temp != -1) && (atoi(linebuf.substr(0, temp).c_str()) != 0))
	      {
		valid++;
	      }
	  }
      }
    // if valid is 2 both the checks have been successful
    return (valid == 2 ? 1 : 0);
  }

  int good(void) const          // Is the library in a usable state?
    { return !!f; }

  int writeable(void) const     // Is this library writeable?
    { return wr; }

  method *load(const char *name);     // Load a method
//int save(method& name);       // Save a method - not defined for cclib
};

RINGING_END_NAMESPACE

#endif
