// -*- C++ -*- output.cpp - generic classes to handle output of methods
// Copyright (C) 2002, 2003, 2004, 2005, 2009 
// Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "output.h"
#include "libraries.h"
#include "methodutils.h"
#include "falseness.h"
#include "music.h"
#include "format.h" // for clear_status
#if RINGING_OLD_INCLUDES
#include <map.h>
#include <utility.h>
#include <stdexcept.h>
#else
#include <map>
#include <utility>
#include <stdexcept>
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#include <iomanip.h>
#else
#include <ostream>
#include <iomanip>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/library.h>
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

class method_properties::impl2 : public library_entry::impl
{
public:
  explicit impl2( const method& m ) : m(m), named(false) {}

  string get_property( int num_opt, const string& name ) const;

private:
  // library_entry::impl interface:
  virtual impl* clone() const { return new impl2(*this); }
  
  virtual string name() const;
  virtual string base_name() const;
  virtual string pn() const;
  virtual int bells() const { return m.bells(); }
  virtual method meth() const;
  
  virtual bool readentry( library_base& lb ) 
  { throw runtime_error("Not the value of an input iterator"); }

  void lookup() const;

  // Data members
  mutable method m;
  mutable bool named; // Have we looked up the name; not whether it is named
  mutable map< pair< int, string >, string > cache;
};

string method_properties::impl2::pn() const
{
  // A random default
  return m.format( method::M_LCROSS | method::M_EXTERNAL );
}

void method_properties::impl2::lookup() const
{
  if ( !named ) {
    m = method_libraries::lookup_method( m );
    named = true;
  }
}

method method_properties::impl2::meth() const
{
  if (!named) lookup();
  return m;
}

string method_properties::impl2::name() const 
{
  if (!named) lookup();
  return m.fullname();
}

string method_properties::impl2::base_name() const 
{
  if (!named) lookup();
  return m.name();
}

string method_properties::impl2::get_property( int num_opt, 
					       const string& prop_name ) const
{
  pair< int, string > cache_key( num_opt, prop_name );

  {
    // Is it in the cache?
    map< pair< int, string >, string >::const_iterator cacheval
      = cache.find( cache_key );
    if ( cacheval != cache.end() )
      return cacheval->second;
  }

  make_string os;

  // Generate the value
  if ( prop_name.size() == 1 )
    {
      switch ( prop_name[0] ) 
	{
	case 'L':
	  os << setw(num_opt) << m.size();
	  break;
	  
	case 'l': 
	  os << m.lh();
	  break;

	case 'p': 
	  os << m.format( method::M_UCROSS | method::M_DOTS | 
			  method::M_EXTERNAL );
	  break;

	case 'q': 
	  os << m.format( method::M_DASH | method::M_SYMMETRY );
	  break;

	case 'Q': 
	  // TODO: 'Q' should use m.format()
	  os << get_short_compressed_pn(m);
	  break;

	case 'r': {
	  // TODO:  Should we cache all rows at same time?
	  row r( m.bells() );
	  int n( num_opt );
	  for ( method::const_iterator 
		  i( m.begin() ), e( m.begin() + num_opt );
		n && i != e; ++i, --n ) 
	    r *= *i;
	  if ( n )
	    throw runtime_error( "Format specifies row after end of method" );
	  os << r;
	} break;

	case 'h': 
	  os << m.at( num_opt-1 ); 
	  break;

	case 'b': 
	  os << setw(num_opt) << m.maxblows();
	  break;

	case 'o': 
	  os << setw(num_opt) << m.leads();
	  break;

	case 'u': 
	  os << setw(num_opt) << m.huntbells(); 
	  break;

	case 'd': 
	  os << m.lhcode(); 
	  break;

	case 'D':
	  os << old_lhcode(m);
	  break;

	case 'y':
	  os << method_symmetry_string( m );
	  break;

	case 'n':
	  os << base_name();
	  break;

	case 'N':
	  os << name();
	  break;

	case 'C': 
	  os << method::classname( m.methclass() ); 
	  break;

	case 'S': 
	  os << method::stagename( m.bells() );
	  break;

	case 'M': 
	  os << setw(num_opt) << musical_analysis::analyse(m);
	  break;

	case 'F': 
	  os << falseness_group_codes(m);
	  break;

	case 'P': {
	  bell b(num_opt-1); os << b;
	  for ( method::const_iterator i( m.begin() ), e( m.end() ); 
		i != e; ++i ) os << (b *= *i);
	} break;

	case 'O': 
	  os << tenors_together_coursing_order(m);
	  break;

	case '#': {
	  static int n=0;
	  os << setw(num_opt) << ++n;
	} break;

	default:
	  throw logic_error( "Unknown variable requested" );
	}
    }
  else
    {
      throw logic_error( "Unknown variable requested" );
    }

  return cache[ cache_key ] = os;
}



method_properties::method_properties( const method& m )
  : library_entry( new impl2( m ) )
{  
}

method_properties::method_properties( const library_entry& le )
  : library_entry( le )
{
  // A sort of create-on-demand downcast.  If le is a method_properties
  // already, the dynamic_cast succeeds, we already have the correct type
  // and don't need to do anything further.  (This is an efficiency hack,
  // but a significant one.)  Otherwise, extract method from le
  // and reset the impl with a new version.   NB: We cannot just conditionally
  // initialise the base class with the right type straight off as we cannot
  // call the protected get_impl methods on le directly, only on our base;
  // and we don't try dynamic casting the handle (le itself) because that
  // might have been sliced.

  // MSVC 6.0 has issues with the get_impl<impl>() syntax  
  if ( ! dynamic_cast<impl2*>( get_impl( (impl*)NULL ) ) ) 
    library_entry::operator=( library_entry( new impl2( le.meth() ) ) );
}

string method_properties::get_property( int num_opt, const string& name ) const
{
  // MSVC 6.0 has issues with the get_impl<impl>() syntax  
  return get_impl( (impl2*)NULL )->get_property( num_opt, name );
}

