// -*- C++ -*- output.cpp - generic classes to handle output of methods
// Copyright (C) 2002, 2003, 2004, 2005, 2009, 2010, 2011, 2020, 2021
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


#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "output.h"
#include "libraries.h"
#include "methodutils.h"
#include "falseness.h"
#include "music.h"
#include "expression.h" // for get_last_exec_status
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
#if RINGING_OLD_C_INCLUDES
#include <time.h>
#else
#include <ctime>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/library.h>
#include <ringing/cclib.h> // For cc_collection_id
#include <ringing/litelib.h> // For litelib::payload
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

char const* expr_error_string = "<ERROR>";

static bool formats_in_unicode = false;

void set_formats_in_unicode(bool val)
{
  formats_in_unicode = val;
}

class method_properties::impl2 : public library_entry::impl
{
public:
  explicit impl2( const method& m, const string& payload )
    : m(m), payload(payload), named(false) {}

  string get_property( pair<int,int> const& num_opts, 
                       const string& name ) const;

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
  const string payload;
  mutable bool named; // Have we looked up the name; not whether it is named
  mutable map< pair< pair<int,int>, string >, string > cache;
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

static string format_pn( const method& m, pair<int,int> const& nums, 
                         int fmt_opts ) {
  if (nums.first) {
    if ( nums.first > m.size() || nums.first <= 0 ) 
      throw runtime_error( "Place notation offset out of range" );
    method::const_iterator i( m.begin() + nums.first - 1 );

    vector<change> ch;
    if (nums.second) {
      if ( nums.second > m.size() || nums.second < nums.first ) 
        throw runtime_error( "Place notation offset out of range" );
      method::const_iterator e( m.begin() + nums.second );

      ch.assign(i, e);
    }
    else ch.assign(i, i+1);

    return method(ch).format( fmt_opts );
  }
  else return m.format( fmt_opts );
}

string method_properties::impl2::get_property( pair<int,int> const& num_opts,
					       const string& prop_name ) const
{
  pair< pair<int,int>, string > cache_key( num_opts, prop_name );

  {
    // Is it in the cache?
    map< pair< pair<int,int>, string >, string >::const_iterator cacheval
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
	  os << setw(num_opts.first) << m.size();
	  break;
	  
	case 'l': 
	  os << m.lh();
	  break;

	case 'p': 
	  os << format_pn( m, num_opts, method::M_UCROSS | method::M_DOTS | 
			                method::M_EXTERNAL );
	  break;

	case 'q': 
	  os << format_pn( m, num_opts, method::M_DASH | 
                                        method::M_FULL_SYMMETRY );
	  break;

	case 'Q': 
	  os << m.format( method::M_DASH | method::M_SYMMETRY |
                          method::M_OMIT_LH );
	  break;

	case 'r': {
	  // TODO:  Should we cache all rows at same time?
	  row r( m.bells() );
	  int n( num_opts.first );
	  for ( method::const_iterator 
		  i( m.begin() ), e( m.begin() + num_opts.first );
		n && i != e; ++i, --n ) 
	    r *= *i;
	  if ( n )
	    throw runtime_error( "Format specifies row after end of method" );
	  os << r;
	} break;

	case 'h': 
	  try { 
            os << m.at( num_opts.first-1 ); 
          } catch ( out_of_range const& ) {
            // This should only happen if we're filtering with -U0 and no -n.
            os << expr_error_string;
          }
	  break;

	case 'b': 
	  os << setw(num_opts.first) << m.maxblows();
	  break;

	case 'o': 
	  os << setw(num_opts.first) << m.leads();
	  break;

	case 'u': 
	  os << setw(num_opts.first) << m.huntbells(); 
	  break;

	case 'G':
	  os << setw(num_opts.first) << m.lh().num_cycles();
          break;

	case 'B': 
	  os << setw(num_opts.first) << m.bells(); 
	  break;

	case 'd': 
	  os << m.lhcode(); 
	  break;

	case 'D':
	  os << old_lhcode(formats_in_unicode, m);
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
	  os << setw(num_opts.first) << musical_analysis::analyse(m);
	  break;

	case 'F': 
	  os << falseness_group_codes(m);
	  break;

	case 'P': {
	  bell b(num_opts.first-1); os << b;
	  for ( method::const_iterator i( m.begin() ), e( m.end() ); 
		i != e; ++i ) os << (b *= *i);
	} break;

	case 'O': 
	  os << tenors_together_coursing_order(m);
	  break;

        case 's':
          os << setw(num_opts.first) << staticity(m);
          break;

	case '#': {
	  static RINGING_ULLONG n=0;
	  os << setw(num_opts.first) << ++n;
	} break;
 
        case 'i': {
          library_entry const& e = method_libraries::instance().find(m);
          if ( !e.null() && e.has_facet<cc_collection_id>() )
            os << setw(num_opts.first) << e.get_facet<cc_collection_id>();
          else 
            os << string( num_opts.first, ' ' );
        } break;

        case 'T': {
          char buf[16]; time_t t = time(NULL);
          // This use of the internal libc struct tm may not be thread safe.
          strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
          os << buf;
        } break;

        case 'a': 
          os << payload;
          break;
 
	case 'V': {
          method work( split_over_and_under(m).first );
          library_entry le( overwork_map().find(work) );
          if ( le.null() || !le.has_facet<litelib::payload>() ) 
            os << work.format( method::M_DASH | method::M_FULL_SYMMETRY );
          else
            os << le.get_facet<litelib::payload>();
	} break;

	case 'U': {
          method work( split_over_and_under(m).second );
          library_entry le( underwork_map().find(work) );
          if ( le.null() || !le.has_facet<litelib::payload>() ) 
            os << work.format( method::M_DASH | method::M_FULL_SYMMETRY );
          else
            os << le.get_facet<litelib::payload>();
	} break;

        case '?':
          // Return it to avoid caching it
          return os << setw(num_opts.first) << get_last_exec_status(); 

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



method_properties::method_properties( const method& m, const string& payload )
  : library_entry( new impl2( m, payload ) )
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
  {
    string pay;
    if ( le.has_facet<litelib::payload>() )
      pay = le.get_facet<litelib::payload>();

    library_entry::operator=( library_entry( new impl2( le.meth(), pay ) ) );
  }
}

string method_properties::get_property( pair<int,int> const& num_opts,
                                        const string& name ) const
{
  // MSVC 6.0 has issues with the get_impl<impl>() syntax  
  return get_impl( (impl2*)NULL )->get_property( num_opts, name );
}

method_properties::~method_properties()
{
  clear_last_exec_status();
}
