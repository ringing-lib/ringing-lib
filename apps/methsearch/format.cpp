// -*- C++ -*- format.cpp - classes to handle format specifiers
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include "format.h"
#include "libraries.h"
#include "methodutils.h"
#include "falseness.h"
#include "music.h"
#if RINGING_OLD_INCLUDES
#include <iterator.h>
#include <algo.h>
#include <map.h>
#else
#include <iterator>
#include <algorithm>
#include <map>
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#include <iomanip.h>
#else
#include <ostream>
#include <iomanip>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <assert.h>
#else
#include <cctype>
#include <cassert>
#endif
#include <ringing/streamutils.h>
#include <ringing/row.h>
#include <ringing/method.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

class histogram_entry
{
public:
  struct cmp
  { 
    typedef bool value_type;
    typedef histogram_entry first_argument_type;
    typedef histogram_entry second_argument_type;
    
    bool operator()( const histogram_entry &x, 
		     const histogram_entry &y ) const;
  };

  histogram_entry( const format_string &f, const method &m );

  void print( ostream &os, size_t count ) const;

  RINGING_FAKE_DEFAULT_CONSTRUCTOR( histogram_entry );

private:
  friend struct cmp;

  const format_string &f;

  row lead_head;
  string pn, compressed_pn;
  map< size_t, row > rows;
  map< size_t, string > paths;
  method changes;
  unsigned int blow_count, lh_order;
  string name, full_name;
  string class_name, stage_name;
  int music_score;
  string falseness_group;
};

bool histogram_entry::cmp::operator()( const histogram_entry &x, 
				       const histogram_entry &y ) const
{
  assert( &x.f == &y.f );

  if ( x.f.has_lead_head )
    {
      if ( x.lead_head < y.lead_head )
	return true;
      else if ( x.lead_head > y.lead_head )
	return false;
    }
  
  if ( x.f.has_pn )
    {
      if ( x.pn < y.pn )
	return true;
      else if ( x.pn > y.pn )
	return false;
    }

  if ( x.f.has_compressed_pn )
    {
      if ( x.compressed_pn < y.compressed_pn )
	return true;
      else if ( x.compressed_pn > y.compressed_pn )
	return false;
    }

  if ( x.f.has_rows.size() )
    for ( vector<size_t>::const_iterator i( x.f.has_rows.begin() ), 
	    e( x.f.has_rows.end() ); i != e; ++i )
      {
	if ( *x.rows.find(*i) < *y.rows.find(*i) )
	  return true;
	else if ( *x.rows.find(*i) > *y.rows.find(*i) )
	  return false;
      }

  if ( x.f.has_changes.size() )
    for ( vector<size_t>::const_iterator i( x.f.has_changes.begin() ), 
	    e( x.f.has_changes.end() ); i != e; ++i )
      {
	if ( x.changes[*i-1] < y.changes[*i-1] )
	  return true;
	else if ( x.changes[*i-1] > y.changes[*i-1] )
	  return false;
      }

  if ( x.f.has_path.size() )
    for ( vector<size_t>::const_iterator i( x.f.has_path.begin() ), 
	    e( x.f.has_path.end() ); i != e; ++i )
      {
	if ( *x.paths.find(*i) < *y.paths.find(*i) )
	  return true;
	else if ( *x.paths.find(*i) > *y.paths.find(*i) )
	  return false;	 
      }

  if ( x.f.has_blow_count )
    {
      if ( x.blow_count < y.blow_count )
	return true;
      else if ( x.blow_count > y.blow_count )
	return false;
    }

  if ( x.f.has_lh_order )
    {
      if ( x.lh_order < y.lh_order )
	return true;
      else if ( x.lh_order > y.lh_order )
	return false;
    }

  if ( x.f.has_name )
    {
      if ( x.name < y.name )
	return true;
      else if ( x.name > y.name )
	return false;
    }

  if ( x.f.has_full_name )
    {
      if ( x.full_name < y.full_name )
	return true;
      else if ( x.full_name > y.full_name )
	return false;
    }

  if ( x.f.has_stage_name )
    {
      if ( x.stage_name < y.stage_name )
	return true;
      else if ( x.stage_name > y.stage_name )
	return false;
    }

  if ( x.f.has_class_name )
    {
      if ( x.class_name < y.class_name )
	return true;
      else if ( x.class_name > y.class_name )
	return false;
    }

  if ( x.f.has_music_score )
    {
      if ( x.music_score < y.music_score )
	return true;
      else if ( x.music_score > y.music_score )
	return false;
    }

  if ( x.f.has_falseness_group )
    {
      if ( x.falseness_group < y.falseness_group )
	return true;
      else if ( x.falseness_group > y.falseness_group )
	return false;
    }

  // They're equal
  return false;
}

histogram_entry::histogram_entry( const format_string &f, const method &m )
  : f( f ),
    music_score(0)
{
  if ( f.has_lead_head )
    lead_head = m.lh();

  if ( f.has_pn )
    {
      make_string tmp;
      for ( method::const_iterator i(m.begin()), e(m.end()); i!=e; ++i )
	tmp << *i << ".";
      pn = tmp;
    }

  if ( f.has_compressed_pn )
    {
      compressed_pn = get_compressed_pn(m);
    }

  if ( f.has_rows.size() )
    for ( vector<size_t>::const_iterator i( f.has_rows.begin() ), 
	    e( f.has_rows.end() ); i != e; ++i )
      {
	rows[*i] = row( m.bells() );
	for ( method::const_iterator i2( m.begin() ), 
		e2( m.begin() + *i ); i2 != e2; ++i2 )
	  rows[*i] *= *i2;
      }

  if ( f.has_changes.size() )
    changes = m;

  if ( f.has_path.size() )
    for ( vector<size_t>::const_iterator i( f.has_path.begin() ), 
	    e( f.has_path.end() ); i != e; ++i )
      {
	make_string os; bell b(*i-1); os << b;
	for ( method::const_iterator i2( m.begin() ), 
		e2( m.end() ); i2 != e2; ++i2 )
	  os << (b *= *i2);
	paths[*i] = os;
      }

  if ( f.has_blow_count )
    blow_count = max_blows_per_place( m );

  if ( f.has_lh_order )
    lh_order = m.leads();

  if ( f.has_name || f.has_full_name )
    {
      const method &m2 = method_libraries::lookup_method( m );

      if ( f.has_name ) name = m2.name();
      if ( f.has_full_name ) full_name = m2.fullname();
    }

  if ( f.has_class_name )
    class_name = method::classname( m.methclass() );

  if ( f.has_stage_name )
    stage_name = method::stagename( m.bells() );

  if ( f.has_music_score )
    music_score = musical_analysis::analyse( m );

  if ( f.has_falseness_group )
    falseness_group = falseness_group_table::codes( m );
}


void histogram_entry::print( ostream &os, size_t count ) const
{
  for ( string::const_iterator iter( f.fmt.begin() ),
	  end( f.fmt.end() ); iter != end; ++iter )
    {
      if ( *iter == '%' || *iter == '$' ) 
	{

	  string::const_iterator iter2(++iter);
	  while ( iter != end && isdigit(*iter) )
	    ++iter;

	  int num_opt = 0;
	  if ( iter2 != iter )
	    num_opt = atoi( string( &*iter2, &*iter ).c_str() );

	  switch ( *iter )
	    {
	    case '%': os << '%'; break;
	    case '$': os << '$'; break;
	    case 'c': os << setw(num_opt) << count; break;
	    case 'l': os << lead_head;  break;
	    case 'p': os << pn; break;
	    case 'q': os << compressed_pn; break;
	    case 'r': os << rows.find( num_opt )->second; break;
	    case 'h': os << changes[ num_opt-1 ]; break;
	    case 'b': os << setw(num_opt) << blow_count; break;
	    case 'o': os << setw(num_opt) << lh_order; break;
	    case 'n': os << name; break;
	    case 'N': os << full_name; break;
	    case 'C': os << class_name; break;
	    case 'S': os << stage_name; break;
	    case 'M': os << setw(num_opt) << music_score; break;
	    case 'F': os << falseness_group; break;
	    case 'P': os << paths.find( num_opt )->second; break;
	    }
	}
      else if ( *iter == '\\' )
	{
	  switch ( *++iter )
	    {
	    case 't': os << '\t';  break;
	    case 'n': os << '\n';  break;
	    default:  os << *iter; break;
	    }
	}
      else 
	os << *iter;
    }

  if ( f.line_break)
    os << endl;
  else
    os << flush;
}

void format_string::add_method_to_stats( const method &m ) const
{
  statistics::add_entry( histogram_entry( *this, m ) );
}

void format_string::print_method( const method &m, ostream &os ) const
{
  histogram_entry( *this, m ).print( os, 0 );
}

format_string::format_string( const string &fmt, 
			      format_string::format_type type )
  : has_lead_head(false),
    has_pn(false),
    has_compressed_pn(false),
    has_blow_count(false),
    has_lh_order(false),

    has_name(false),
    has_full_name(false),
    has_class_name(false),
    has_stage_name(false),

    has_music_score(false),
    has_falseness_group(false),

    line_break(true),

    fmt(fmt)
{
  assert( type == stat_type || type == normal_type );

  for ( string::const_iterator iter( fmt.begin() ), end( fmt.end() ); 
	iter != end; ++iter )
    {
      if ( *iter == '%' || *iter == '$' ) 
	{
	  if ( ++iter == end ) 
	    throw argument_error( "Format ends with an unescaped `%'" );

	  string::const_iterator iter2(iter);

	  while ( iter != end && isdigit(*iter) )
	    ++iter;

	  if ( iter == end ) 
	    throw argument_error( "End of format reached whilst processing "
				  "numeric argument to `%'" );
	  // Errors for formats that can only be used with -H or -R
	  switch ( *iter )
	    {
	    case 'c': 
	      if ( type != stat_type )
		throw argument_error( make_string() << "The `%" << *iter << "'"
				      " can only be used in stats formats" );
	      break;

	    case 'p': case 'q': case 'n': case 'N': case 'P':
	      if ( type != normal_type )
		throw argument_error( make_string() << "The `%" << *iter << "'"
				      " can only be used in output formats" );
	      break;

	    case '%': case '$': case 'l': case 'r': case 'b': 
	    case 'C': case 'S': case 'M': case 'h': case 'F':
	    case 'o':
	      // Can be used in either
	      break;

	    default:
	      throw argument_error( make_string() << "Unknown format "
				    "specifier: `%" << *iter << "'" );
	    }


	  int num_opt = atoi( string( &*iter2, &*iter ).c_str() );

	  // Errors for formats that must or mustn't have a numeric argument
	  switch ( *iter )
	    {
	    case '%': case '$': case 'c': case 'b': case 'M': 
	    case 'o':
	      // Option may but needn't have a number
	      break;

	    case 'n': case 'N': case 'p': case 'q': case 'l': 
	    case 'C': case 'S': case 'F':
	      if ( iter != iter2 )
		throw argument_error
		  ( make_string() << "The `%" << *iter << "' "
		    "format specifier must not be preceeded by "
		    "a number" );
	      break;

	    case 'r': case 'h': case 'P':
	      if ( iter == iter2 )
		throw argument_error
		  ( make_string() << "The `%" << *iter << "' "
		    "format specifier must be preceeded by a number" );
	      break;
	    }

	  // Mark the option as used
	  switch ( *iter )
	    {
	    case 'l': 
	      has_lead_head = true; 
	      break;

	    case 'p': 
	      has_pn = true; 
	      break;

	    case 'q': 
	      has_compressed_pn = true; 
	      break;

	    case 'b':
	      has_blow_count = true;
	      break;

	    case 'o':
	      has_lh_order = true;
	      break;

	    case 'r':
	      if ( find( has_rows.begin(), has_rows.end(), num_opt ) 
		     == has_rows.end() )
		has_rows.push_back( num_opt );
	      break;

	    case 'h':
	      if ( find( has_changes.begin(), has_changes.end(), num_opt ) 
		   == has_changes.end() )
		has_changes.push_back( num_opt );
	      break;

	    case 'P':
	      if ( find( has_path.begin(), has_path.end(), num_opt ) 
		   == has_path.end() )
		has_path.push_back( num_opt );
	      break;

	    case 'n':
	      has_name = true;
	      break;

	    case 'N':
	      has_full_name = true;
	      break;

	    case 'C':
	      has_class_name = true;
	      break;

	    case 'S':
	      has_stage_name = true;
	      break;

	    case 'M':
	      has_music_score = true;
	      break;

	    case 'F':
	      has_falseness_group = true;
	      break;
	    }
	}
      else if ( *iter == '\\' )
	{
	  if ( ++iter == end ) 
	    {
	      line_break = false;
	      this->fmt.erase(this->fmt.end()-1);
	      return;
	    }
	}
    }
}



struct statistics::impl
{
  typedef map< histogram_entry, size_t, histogram_entry::cmp > map_type;
  map_type histogram;
};

statistics::statistics()
  : pimpl( new impl )
{
}

statistics::~statistics()
{
}

statistics::impl &statistics::instance()
{
  static statistics tmp;
  return *tmp.pimpl;
}

size_t statistics::output( ostream &os )
{
  size_t count(0u);

  for ( impl::map_type::const_iterator 
	  i( instance().histogram.begin() ),
	  e( instance().histogram.end() );
	i != e;  ++i )
    {
      i->first.print( os, i->second );
      count += i->second;
    }

  return count;
}

void statistics::add_entry( const histogram_entry &entry )
{
  ++instance().histogram[entry];
}

void clear_status()
{
  cerr << '\r' << string( 60, ' ' ) << '\r';
}

void output_status( const method &m )
{
  make_string tmp;
  for ( method::const_iterator i(m.begin()), e(m.end()); i!=e; ++i )
    tmp << *i << ".";
  string s = tmp;

  if ( s.size() > 45 ) 
    s = s.substr(0, 45) + "...";
  
  clear_status();
  cerr << "Trying " << s << flush;
}

void output_count( unsigned long c )
{
  cout << "Found " << c << " methods\n";
}

