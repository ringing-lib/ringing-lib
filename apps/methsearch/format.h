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
#include <map.h>
#else
#include <stdexcept>
#include <vector>
#include <map>
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

class expression
{
public:
  expression() {}
  expression( const string& str );

  class node {
  public:
    node() {} // Keep gcc-2.95.3 happy
    virtual ~node() {}
    virtual string s_evaluate( const method& m ) const = 0;
    virtual long   i_evaluate( const method& m ) const = 0;
    
  private:
    node(const node&); // Unimplemented
    node& operator=(const node&); // Unimplemented
  };

  bool null() const { return !pimpl; }
  string evaluate( const method& m ) const;
  bool   b_evaluate( const method& m ) const;

private:
  class parser;
  shared_pointer<node> pimpl;
};

struct format_string
{
  enum format_type { stat_type, normal_type };

  format_string( const string &fmt, format_type type );

  void print_method( const method &m, ostream &os ) const;
  void add_method_to_stats( const method &m ) const;

  bool has_lead_head;
  bool has_pn;
  bool has_compressed_pn;
  bool has_short_compressed_pn;
  bool has_blow_count;
  bool has_lh_order;
  bool has_lh_code;
  bool has_hunt_bells;
  
  bool has_name;
  bool has_full_name;
  bool has_class_name;
  bool has_stage_name;
  
  bool has_music_score;
  bool has_falseness_group;
  
  bool line_break;

  bool has_expression;

  vector<size_t> has_rows;
  vector<size_t> has_changes;
  vector<size_t> has_path;
  
  map<string, expression> exprs;

  string fmt;
};

class histogram_entry;

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

void clear_status();
void output_status( const method &m );
void output_count( unsigned long );


#endif // METHSEARCH_FORMAT_INCLUDED
