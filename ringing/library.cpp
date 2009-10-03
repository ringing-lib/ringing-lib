// library.cpp : Libraryish things
// Copyright (C) 2001, 2002, 2004, 2009 Martin Bright <martin@boojum.org.uk>
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

#if RINGING_HAS_PRAGMA_INTERFACE
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
string library::libpath;

void library::addtype(init_function lt) 
{ 
  libtypes.push_back(lt); 
}

void library::setpath(string const& p)
{
  libpath = p;
}

RINGING_START_ANON_NAMESPACE
list<string> split_path( string const& p )
{
  list<string> pp;
  string::size_type i=0;
  while (true) {
    string::size_type j = p.find(':', i);
    if ( j == string::npos ) {
      if ( p.size() ) pp.push_back( p.substr(i) );
      break;
    }
    
    pp.push_back( p.substr(i, j-i) );
    i = j+1;
  }
  return pp;
}

bool try_load_lib( shared_pointer<library_base>& lb,
                   list<library::init_function> const& lts, 
                   string const& filename )
{
  typedef list<library::init_function>::const_iterator iterator;
  for ( iterator i=lts.begin(), e=lts.end(); !lb && i!=e; ++i )
    lb.reset( (**i)(filename) );
  return (bool)lb;
}

RINGING_END_ANON_NAMESPACE

#if RINGING_USE_EXCEPTIONS
library_base::invalid_name::invalid_name() 
  : invalid_argument("The method name supplied could not be found in the library file") {}
#endif

library::library(const string& filename)
{
  char const sep = RINGING_WINDOWS ? '\\' : '/';

  bool isabs = false;
  if ( filename[0] == sep ) isabs = true;
#if RINGING_WINDOWS
  if ( filename.size() >= 3 && filename[1] == ':' && filename[2] == sep )
    isabs = true;
#endif

  if ( filename.size() && !try_load_lib( lb, libtypes, filename ) && !isabs )
  {
    list<string> locs( split_path(libpath) );
    for (list<string>::const_iterator i=locs.begin(), e=locs.end(); i!=e; ++i)
      if ( try_load_lib( lb, libtypes, *i + sep + filename ) )
        break;
  }
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

method library_entry::impl::meth() const
{
  return method( pn(), bells(), base_name() );
}

shared_pointer< library_facet_base > 
library_entry::impl::get_facet( const library_facet_id& id ) const
{
  return shared_pointer< library_facet_base >();
}


bool library_entry::impl::has_facet( const library_facet_id& id ) const
{
  // Quite inefficient: subclasses are encouraged to override this.
  return bool( get_facet(id) );
}

RINGING_END_NAMESPACE
