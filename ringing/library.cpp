// library.cpp : Libraryish things
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
#include <ringing/library.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

list<libtype*> library::libtypes;

#if RINGING_USE_EXCEPTIONS
library_base::invalid_name::invalid_name() 
  : invalid_argument("The method name supplied could not be found in the library file") {}
#endif

library::library(const string& filename)
{
  lb = NULL;
  if (filename.length() > 0)
    {
      ifstream ifs(filename.c_str(), ios::in);
      if (ifs.good())
	{
	  list<libtype*>::const_iterator i = libtypes.begin();
	  while ((lb == NULL) && (i != libtypes.end()))
	    {
	      lb = (*i)->open(ifs, filename);
	      i++;
	    }
	  ifs.close();
	}
      // Don't worry about else condition - user should check for good()
    }
}

RINGING_END_NAMESPACE
