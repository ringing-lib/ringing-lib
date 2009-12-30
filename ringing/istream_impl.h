// -*- C++ -*- istream_impl.h - Classes to help implement istream extractors
// Copyright (C) 2009 Richard Smith <richard@ex-parrot.com>

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


#ifndef RINGING_ISTREAM_IMPL_H
#define RINGING_ISTREAM_IMPL_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_OLD_IOSTREAMS
#include <istream.h>
#else 
#include <istream>
#endif

RINGING_USING_STD

RINGING_START_NAMESPACE

class istream_flag_sentry : public istream::sentry {
public:
  istream_flag_sentry( istream& i, bool noskipws = false ) 
    : istream::sentry(i, noskipws), i(i), oldf( i.flags() )
  {}

 ~istream_flag_sentry() { i.setf( oldf ); }

private:
  ios_base& i;
  ios_base::fmtflags oldf;
};

RINGING_END_NAMESPACE

#endif // RINGING_ISTREAM_IMPL_H
