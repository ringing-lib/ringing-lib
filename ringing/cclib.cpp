// cclib.cpp - Read and write the Central Council Method libraries
// Copyright (C) 2001 Mark Banner <mark@standard8.co.uk>

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
#include <ringing/cclib.h>
#include <ringing/method.h>
#include <string>

RINGING_START_NAMESPACE

newlib<cclib> cclib::type;

// This function is for creating lower case strings.
void lowercase(char &c)
{
  c = tolower(c);
}

// Load a method from a Central Council Method library
method *cclib::load(const char *name)
{
  string methname(name);

  // These *must* be initialised as -1 for later
  int meth_name_starts = -1;
  int meth_name_ends = -1;
  int meth_hl = -1;
  int meth_le = -1;
  int meth_lh = -1;

  f.seekg(0,ios::beg);		// Go to the beginning of the file
  while(f.good()) {
    
    // first, read in a line
    string linebuf;
    getline(f, linebuf);
    if (linebuf.length() > 1)
      {
	// The second check for No. is used as an extra insurance check...
	if ((linebuf.find("Name") != -1) && (linebuf.find("No.") != -1))
	  {
	    // we now have start and end position.
	    meth_name_starts = linebuf.find("Name");
	    meth_name_ends = linebuf.find("Notation");
	    meth_hl = linebuf.find("hl");
	    if (meth_hl != -1)
	      {
		meth_le = linebuf.find("le");
		meth_lh = linebuf.find("lh");
	      }
	    else
	      {
		meth_le = -1;
		meth_lh = linebuf.find("lh");
	      }
	  }
	else if (meth_name_starts != meth_name_ends)
	  {
	    if (linebuf.length() > meth_name_ends)
	      {
		string wordbuf(linebuf, meth_name_starts, meth_name_ends - meth_name_starts);
		for_each(wordbuf.begin(), wordbuf.end(), lowercase);
		for_each(methname.begin(), methname.end(), lowercase);

		// FIXME: This will match "Plain" to "Plain Bob"
		if (wordbuf.compare(methname, 0, methname.length()) == 0)
		  {
		    // we have found the method.

		    // now get the rest of the details
		    method *m = NULL;
		    string pn;
		    // Get place notation
		    if (meth_hl != -1)
		      {
			// We have a reflection
			pn.append("&");
			// Add place notation
			pn.append(linebuf.substr(meth_name_ends, meth_hl - meth_name_ends));
			// Add half lead notation
			pn.append(linebuf.substr(meth_hl, meth_le - meth_hl));
			m = new method(pn, b, name);
			m->push_back(change(b, linebuf.substr(meth_le, meth_lh - meth_le)));
		      }
		    else
		      {
			// Add place notation
			pn.append(linebuf.substr(meth_name_ends, meth_lh - meth_name_ends));
			m = new method(pn, b, name);
		      }
		    return m;
		  }
	      }
	  }

      }
    // else ignore  
  }
  return NULL;			// Couldn't find it
}

RINGING_END_NAMESPACE

