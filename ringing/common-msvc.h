// -*- C++ -*- common-msvc.h.in - Common #defines for the MSVC compiler.
//
// Copyright (C) 2001, 2002, 2003 Martin Bright <martin@boojum.org.uk> 
// and Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_COMMON_MSVC_H
#define RINGING_COMMON_MSVC_H

// Getting the GNU Autotools to play nicely with Microsoft Visual Studio 
// is tedious and requires installing a Unix-like environment (e.g. Cygwin
// or perhaps MinGW) on top of Windows, even just to get the ./configure
// script to work.   This header exists to allow users of Visual Studio
// to avoid the Autotools entirely.  Lucky them.

#ifndef RINGING_INSIDE_COMMON_H
# error This header should not be included directly.
#endif


#define RINGING_PACKAGE "ringing-lib"
#define RINGING_VERSION "0.2.8"


#define RINGING_OLD_INCLUDES 0

#define RINGING_OLD_C_INCLUDES 0

#define RINGING_USE_EXCEPTIONS 1

#if _MSC_VER < 1200
# define RINGING_USE_NAMESPACES 0
#else
# define RINGING_USE_NAMESPACES 1
#endif

#define RINGING_USE_STD 1

#define RINGING_USE_TEMPLATE_FUNCTION_SPECIALISATION 1

#define RINGING_PROTECTED_MEMBER_BASES 0

#if _MSC_VER < 1200
# define RINGING_PREMATURE_MEMBER_INSTANTIATION 1
#else
# define RINGING_PREMATURE_MEMBER_INSTANTIATION 0
#endif

#if _MSC_VER < 1300
# define RINGING_HAVE_STD_ITERATOR 0
#else
# define RINGING_HAVE_STD_ITERATOR 1
#endif

#define RINGING_HAVE_OLD_IOSTREAMS 0

#define RINGING_USE_STRINGSTREAM 1

#define RINGING_AS_DLL 0

// *** Define this to be 1 if want to include GNU readline support
// or to 0 otherwise.
#define RINGING_USE_READLINE 0

// *** Define this to be 1 if want to include XML support via the 
// Apache Xerces library, or to 0 otherwise.
#define RINGING_USE_XERCES 0

#endif // RINGING_COMMON_MSVC_H
