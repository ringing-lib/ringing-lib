// -*- C++ -*- xmllib.h - Access to the online XML method library
// Copyright (C) 2003 Richard Smith <richard@ex-parrot.com>.

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// $Id$

#ifndef RINGING_XMLLIB_H
#define RINGING_XMLLIB_H

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/common.h>
#include <ringing/library.h>
#include <string>

RINGING_START_NAMESPACE

RINGING_USING_STD

class RINGING_API xmllib : public library 
{
public:
  enum file_arg_type { filename, url, default_url };

  xmllib() {}

  // - If the first argument is filename, this reads a local file
  //   just as if it had been read using the registerlib mechanism.
  // - If the first argument is url, this reads a URL to fetch and 
  //   read.  This requires Xerces to have been built with net access.
  // - If the first argument is default_url, the string is the CGI
  //   parameters (excluding initial '?') to be passed to 
  //   http://methods.ringing.org/cgi-bin/simple.pl
  explicit xmllib( file_arg_type, const string&);

  static void registerlib(void) {
    library::addtype(&canread);
  }

private:
  // Is this file in the right format?
  static library_base *canread(const string& filename);

  class impl;
};

RINGING_END_NAMESPACE

#endif
