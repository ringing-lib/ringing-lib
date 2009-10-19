// -*- C++ -*- methodset.h - A set of methods with input and output interfaces
// Copyright (C) 2009 Richard Smith <richard@ex-parrot.com>

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
  
#ifndef RINGING_METHODSET_H
#define RINGING_METHODSET_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/library.h>
#include <ringing/libout.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

// This class provides a fast way of accessing methods.  Unlike the cclib,
// xmllib and mslib classes, it is not backed by a file and so does not 
// require any file I/O to look up methods.  It also provides an interface
// for both input and output, thus allowing bidirectional iterators 
// on the interface.

class RINGING_API methodset : public libout, public library {
public:
  methodset();
 
  typedef library::const_iterator const_iterator;
  typedef library::const_iterator iterator; // To avoid problems with libout's
  using library::begin;
  using library::end;

  void clear();

private:
  class impl;
};

RINGING_END_NAMESPACE

#endif // RINGING_METHODSET_H