// mslib.cc - Read and write MicroSIRIL libraries
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#if RINGING_OLD_C_INCLUDES
#include <string.h>
#else
#include <cstring>
#endif
#include <ringing/mslib.h>

RINGING_START_NAMESPACE

newlib<mslib> mslib::type;

// Load a method from a MicroSIRIL library
method *mslib::load(char *name)
{
  char *s;

  f.seekg(0,ios::beg);		// Go to the beginning of the file
  while(f) {
    s = name;
    // See whether the name matches
    while(*s && tolower(f.get()) == tolower(*s++));
    if(*s == '\0' && isspace(f.get())) { // Found it
      char lh[16];		       // Get the lead head code
      f.get(lh,16,' ');
      buffer linebuf(256);
      f.get(linebuf, linebuf.size()); // Read in the rest of the line
      s = strtok(linebuf," \t"); // Strip blanks
      method *m = NULL;
      m = new method(s,b,name);
      if(*lh) {
	s = lh + strlen(lh) - 1;
	if(*s == 'z') {
	  *s = '\0';
	  m->push_back(change(b, lh));
	} else {
	  if((lh[0] >= 'a' && lh[0] <= 'f')
	     || (lh[0] >= 'p' && lh[0] <= 'q'))
	    m->push_back(change(b, "12"));
	  else
	    m->push_back(change(b, "1"));
	}
    }

      return m;
    }
    while(!f.eof() && f.get() != '\n');	// Skip to the next line
  }
  return NULL;			// Couldn't find it
}

#if 0
// Write a method to a MicroSIRIL library
//
// This isn't ideal, because MicroSIRIL libraries are far
// from ideal.  If the name is more than one word, we run them
// all together and make sure the first letter is upper case, and
// the rest lower case.  If the method isn't already in the library,
// put it in so that everything stays in alphabetical order.
int mslib::save(method& m)
{
  char *buffer = new char[160];
  char *buffer2 = new char[160];
  char name[32];

  // First sort the name out.
  char *s = m.name(), *t;
  if(*s == '\0') return -1;
  *t++ = toupper(*s++);
  while(*s != '\0') {
    if(isspace(*s))      s++;
    else
      *t++ = tolower(*s++);
  }
  *t = '\0';

  streampos rpos, wpos;

  // Now let's see whether the method is already in the library
  f.seekg(0, ios::beg);
  while(f) {
    wpos = f.tellg();
    s = name;
    // See whether the name matches
    while(*s && tolower(f.get()) == tolower(*s++));
    if(*s == '\0' && isspace(f.get()) == ' ') // Found it
      break;
    while(!f.eof() && f.get() != '\n');	// Skip to the next line
  }

  if(!f.eof()) {		// Found it
    // Skip to the next line
    while(!f.eof() && f.get() != '\n');
    rpos = f.tellg();
  } else  {			// Didn't find it
    f.seekg(0, ios::beg);
    while(f) {
      char c;
      wpos = rpos = f.tellg();
      s = name;
      while(*s && tolower((c = f.get())) == tolower(*s++));
      // See whether we should be before this in alphabetical order
      if(*s == '\0' || tolower(c) > tolower(s[-1]))
	break;
    }
    if(f.eof()) {		// Need to add at the end of the file
      f.put('\n');
      wpos = rpos = f.tellp();
    }
  }

  // Now read in the first buffer-ful after the insertion point, so
  // we don't lose it.
  f.seekg(rpos);
  f.get(buffer, sizeof buffer);
  rpos = f.tellg();

  // Write a line containing the method to be saved

  // Oh sod it, this is too complicated and nasty.
  // Who wants to be able to write to MicroSIRIL libraries
  // anyway?  They're designed to be written by humans
  // and read by computers.
}

#endif

RINGING_END_NAMESPACE

