// library.cpp : Libraryish things
// Copyright (C) 2001-2 Martin Bright <martin@boojum.org.uk>
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

#include <ringing/common.h>

#ifdef RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/library.h>
#if RINGING_OLD_INCLUDES
#include <fstream.h>
#else
#include <fstream>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

list<library::init_function> library::libtypes;

#if RINGING_USE_EXCEPTIONS
library_base::invalid_name::invalid_name() 
  : invalid_argument("The method name supplied could not be found in the library file") {}
#endif

library::library(const string& filename)
{
  typedef list<init_function>::const_iterator iterator;
  for ( iterator i=libtypes.begin(), e=libtypes.end(); !lb && i!=e; ++i )
    lb.reset( (**i)(filename) );
}


// Return a list of items
int library_base::dir(list<string>& result) const
{
  if (!good())
    return 0;

  size_t orig_size( result.size() );

  for ( const_iterator i(begin()); i != end(); ++i )
    result.push_back( i->name() );

  return result.size() - orig_size;
}

// Return a list of items
int library_base::mdir(list<method>& result) const
{
  if (!good())
    return 0;

  size_t orig_size( result.size() );

  for ( const_iterator i(begin()); i != end(); ++i )
    result.push_back( method( i->pn(), i->bells(), i->base_name() ) );

  return result.size() - orig_size;
}

// This function is for creating lower case strings.
static void lowercase(char &c)
{
  c = tolower(c);
}

library_base::const_iterator library_base::end() const
{
  return const_iterator();
}

// Load a method from a Central Council Method library
method library_base::load(const string& name, int stage) const
{
  string methname(name);
  for_each(methname.begin(), methname.end(), lowercase);

  for ( const_iterator i(begin()); i != end(); ++i )
    {
      // Extract the method name section
      string wordbuf( i->name() );
      
      // Make all letters lower case
      for_each(wordbuf.begin(), wordbuf.end(), lowercase);
	    
      // Do we have the correct line for the method?
      if ( wordbuf == methname &&
	   // If stage is 0 then any stage is permitted.
	   (!stage || i->bells() == stage) )
	{
	  // we have found the method.
	  return method( i->pn(), i->bells(), i->base_name() );
	}
    }

  // If we are here we couldn't find the method.

#if RINGING_USE_EXCEPTIONS
  // If we are using exceptions, throw one to notify it couldn't be found.
  throw invalid_name();
#endif 
  // Visual Studio 5 requires a return statement even after a throw.

  // Otherwise we have to return something to avoid warning and errors, so
  // make up something strange. Give it a name so it can always be checked
  // against.
  return method( 0, 0, "Not Found" );
}


RINGING_END_NAMESPACE
