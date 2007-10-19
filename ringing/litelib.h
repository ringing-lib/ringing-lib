// -*- C++ -*- litelib.h - Lightweight library format
// Copyright (C) 2007 Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_LITELIB_H
#define RINGING_LITELIB_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/library.h>
#include <ringing/libfacet.h>
#include <string>

RINGING_START_NAMESPACE

RINGING_USING_STD

// This provides an extremely lightweight library format.
// It's designed to easy to create and parse (by hand or machine).
// The library has no header, and each line starts with the p.n.;
// any text after the first whitespace on a line is ignored.
// It is particularly intended as a convenient way of piping data
// between stdout and stdin of consecutive programs in a pipeline.
class RINGING_API litelib : public library
{
public:
  RINGING_DECLARE_LIBRARY_FACET( payload, string );

  litelib() {}

  explicit litelib( int b, const string& filename );
  explicit litelib( int b, istream& is );

  //static void registerlib(void);

private:
  class impl;
};

RINGING_END_NAMESPACE

#endif

