// stl.cpp - Placeholder file for explicit instantiations of STL classes
// Copyright (C) 2001-2 Richard Smith <richard@ex-parrot.com>

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

#if RINGING_AS_DLL

#define RINGING_DO_STL_INSTANTIATIONS

#if defined(_MSC_VER)
#pragma warning( disable: 4660 )
#endif

// include all headers that contain explicit template instantiations here...
#include <ringing/row.h>
#include <ringing/library.h>
#include <ringing/touch.h>
#include <ringing/basic_search.h>
#include <ringing/table_search.h>

#undef RINGING_DO_STL_INSTANTIATIONS

#endif // RINGING_AS_DLL
