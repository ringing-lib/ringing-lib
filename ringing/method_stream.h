// -*- C++ -*- method_stream.h - lightweight output of methods
// Copyright (C) 2021 Richard Smith <richard@ex-parrot.com>.

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

#ifndef RINGING_METHOD_STREAM_H
#define RINGING_METHOD_STREAM_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/libout.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class RINGING_API method_stream : public libout {
public:
  method_stream();

private:
  class impl;
};



RINGING_END_NAMESPACE

#endif // RINGING_METHOD_STREAM_H
