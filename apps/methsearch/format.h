// -*- C++ -*- format.h - classes to handle format specifiers
// Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

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


#ifndef METHSEARCH_FORMAT_INCLUDED
#define METHSEARCH_FORMAT_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#include <vector.h>
#include <utility.h>
#else
#include <stdexcept>
#include <vector>
#include <utility>
#endif
#include <string>
#if RINGING_HAVE_OLD_IOSTREAMS
#include <ostream.h>
#else
#include <iosfwd>
#endif
#include <ringing/pointers.h>

// Forward declare ringing::method
RINGING_START_NAMESPACE
class method;
RINGING_END_NAMESPACE

RINGING_USING_NAMESPACE
RINGING_USING_STD

class method_properties 
{
public:
  explicit method_properties( const method& m );

  // All out-of-line so impl is complete
  method_properties();
 ~method_properties();
  method_properties( const method_properties& );
  method_properties& operator=( const method_properties& );

  string get_property( int num_opt, const string& name ) const;

private:

  class impl;
  shared_pointer<impl> pimpl;
};


// Exception to do exit(0) but calling destructors
class exit_exception
{
};

class argument_error : public std::runtime_error
{
public:
  argument_error( const string &str )
    : runtime_error( str )
  {}
};

class histogram_entry;

struct format_string
{
  enum format_type { stat_type, normal_type, require_type, preparsed_type };

  explicit format_string( const string &fmt = "", 
			  format_type type = normal_type );

  void print_method( const method_properties &m, ostream &os ) const;
  void add_method_to_stats( const method_properties &m ) const;

  vector< pair< int, string > > vars;

  bool has_name;
  bool has_falseness_group;
  
  bool null() const { return fmt.empty(); }

  static size_t parse_requirement( const string& str );

private:
  friend class histogram_entry;
  string fmt;
};

class statistics
{
public:
  static size_t output( ostream &os );
  static void add_entry( const histogram_entry &entry );

  // Public to avoid MSVC compilation errors
 ~statistics();

private:
  struct impl;
  scoped_pointer< impl > pimpl;

  static impl &instance();

  statistics();
};

size_t parse_requirement( const string& str );

void clear_status();
void output_status( const method &m );
void output_count( unsigned long );
void output_raw_count( unsigned long );


#endif // METHSEARCH_FORMAT_INCLUDED
