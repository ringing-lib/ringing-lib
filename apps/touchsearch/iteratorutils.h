// -*- C++ -*- iteratorutils.h - utility iterator classes
// Copyright (C) 2002, 2003, 2007 Richard Smith <richard@ex-parrot.com>

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

#ifndef TOUCHSEARCH_ITERATORUTILS_INCLUDED
#define TOUCHSEARCH_ITERATORUTILS_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <iterator.h>
#else
#include <iterator>
#endif

RINGING_USING_STD

template < class Function >
class iter_from_fun_t 
  : RINGING_STD_OUTPUT_ITERATOR( typename Function::argument_type )
{
public:
  // Standard iterator typedefs
  typedef void value_type;
  typedef void reference;
  typedef void pointer;
  typedef void differnce_type;
  typedef output_iterator_tag iterator_category;

  // Output Iterator requirements
  iter_from_fun_t &operator*() { return *this; }
  iter_from_fun_t &operator++() { return *this; }
  iter_from_fun_t &operator++(int) { return *this; }
  iter_from_fun_t &operator=( typename Function::argument_type arg )
  { func( arg ); return *this; }

  // Construction
  iter_from_fun_t( const Function &func ) : func( func ) {}

private:
  // Data members
  Function func;
};

// Construction helper
template < class Function >
iter_from_fun_t< Function > iter_from_fun( const Function &func )
{
  return iter_from_fun_t< Function >( func );
}

#endif // TOUCHSEARCH_ITERATORUTILS_INCLUDED

