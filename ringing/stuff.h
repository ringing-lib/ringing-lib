// -*- C++ -*- stuff.h - Random useful things
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

#ifndef RINGING_STUFF_H
#define RINGING_STUFF_H

#include <ringing/common.h>

RINGING_START_NAMESPACE

// A buffer which allocates and frees itself
class buffer {
private:
  char *p;
  int s;

public:
  buffer(int siz) : s(siz) { p = new char[s]; }
  ~buffer() { delete[] p; }
  
  int size() { return s; }
  operator char*() { return p; }
  operator const char *() const { return p; }
  char& operator[](int i) { return p[i]; }
  const char& operator[](int i) const { return p[i]; }
};

RINGING_END_NAMESPACE

#endif  


