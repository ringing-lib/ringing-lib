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
method cclib::load(const char *name)
{
  string methname(name);

  // These *must* be initialised as string::npos for later
  string::size_type meth_name_starts = string::npos;
  string::size_type meth_name_ends = string::npos;
  string::size_type meth_hl = string::npos;
  string::size_type meth_le = string::npos;
  string::size_type meth_lh = string::npos;

  ifstream f(filename.c_str());

  while(f.good()) {
    // first, read in a line
    string linebuf;
    getline(f, linebuf);
    if (linebuf.length() > 1)
      {
	// The second check for No. is used as an extra insurance check...
	if ((linebuf.find("Name") != string::npos) && (linebuf.find("No.") != string::npos))
	  {
	    // This is a header line - use it to get the start and end position
	    // of fields
	    meth_name_starts = linebuf.find("Name");
	    meth_name_ends = linebuf.find("Notation");
	    meth_hl = linebuf.find("hl");

	    // If we have a half lead then take note accordingly.
	    if (meth_hl != string::npos)
	      {
		meth_le = linebuf.find("le");
		meth_lh = linebuf.find("lh");
	      }
	    else
	      {
		meth_le = string::npos;
		meth_lh = linebuf.find("lh");
	      }
	  }
	// This if checks that we have found at least one header line.
	else if (meth_name_starts != meth_name_ends)
	  {
	    // This could be a line detailing a method.
	    if (linebuf.length() > meth_name_ends)
	      {
		// Extract the method name section
		string wordbuf(linebuf, meth_name_starts, meth_name_ends - meth_name_starts);
		// Remove whitespace from end of method name
		string::const_iterator i = wordbuf.end();
		string::const_iterator j = wordbuf.end();
		do {
		  j = i;
		  i--;
		} while (isspace(*i));

		wordbuf = wordbuf.substr(0, j - wordbuf.begin());

		// Copy wordbuf to preserve for later
		string methodname(wordbuf);

		// Make all letters lower case
		for_each(wordbuf.begin(), wordbuf.end(), lowercase);
		for_each(methname.begin(), methname.end(), lowercase);

		// Do we have the correct line for the method?
                // (The overloads of compare provided by glibstdc++-2.x are
                // non-standard extentions, and have been changed in v3.0.
		// I think the following does the same.  RAS)
		if (wordbuf.find(methname) == 0)
		  {
		    // we have found the method.
		    // now get the rest of the details
		    string pn;
		    // Get place notation
		    if ((meth_hl != string::npos) && (meth_le != string::npos))
		      {
			// We have a reflection
			pn.append("&");
			// Add place notation
			pn.append(linebuf.substr(meth_name_ends, meth_hl - meth_name_ends));
			// Add half lead notation
			pn.append(linebuf.substr(meth_hl, meth_le - meth_hl));

			// Now create the method
			method m(pn, b, methodname);

			// Strip any whitespace
			string ch(linebuf, meth_le, meth_lh - meth_le);
			string::const_iterator i = ch.begin();
			while(!isspace(*i)) i++;
			
			// Create the change
			ch = ch.substr(0, i - ch.begin());
			change c(b, ch);
			m.push_back(c);

			// We've finished now.
			return m;
		      }
		    else if (meth_hl != string::npos)
		      {
			// This is for methods like Grandsire which the CC
			// have entered in an awkward way.

			// Make this a reflection temporarily
			pn.append("&");
		        pn.append(linebuf.substr(meth_name_ends, meth_lh - meth_name_ends));

			// Create the method
			method m(pn, b, methodname);

			// Now remove the last change
			m.pop_back();
			return m;
		      }
		    else
		      {
			// This is for the non-reflecting irregular methods.

			// Add place notation
			pn.append(linebuf.substr(meth_name_ends, meth_lh - meth_name_ends));

			// Create the method and return it.
			method m(pn, b, methodname);
			return m;
		      }
		  }
	      }
	  }

      }
    // else ignore the line (length < 1)
  }
  f.close();
  // If we are here we couldn't find the method.
#if RINGING_USE_EXCEPTIONS
  // If we are using exceptions, throw one to notify it couldn't be found.
  throw invalid_name();
#else
  // Otherwise we have to return something to avoid warning and errors, so
  // make up something strange. Give it a name so it can always be checked
  // against.
  method m(1,2, "Not Found");
  return m;
#endif
}

#if RINGING_USE_EXCEPTIONS
cclib::invalid_name::invalid_name() 
  : invalid_argument("The method name supplied could not be found in the library file") {}
#endif

RINGING_END_NAMESPACE
