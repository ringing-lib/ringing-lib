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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "format.h"
#include "expression.h"
#include "libraries.h"
#include "methodutils.h"
#include "falseness.h"
#include "music.h"
#include "tokeniser.h"
#include "exec.h"
#if RINGING_OLD_INCLUDES
#include <iterator.h>
#include <algo.h>
#include <map.h>
#include <stack.h>
#include <stdexcept.h>
#include <functional.h>
#else
#include <iterator>
#include <algorithm>
#include <map>
#include <stack>
#include <stdexcept>
#include <functional>
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
#include <ringing/pointers.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct method_properties::impl
{
  explicit impl( const method& m ) : m(m) {}
  const method& m;
  mutable map< pair< int, string >, string > cache;
};


method_properties::method_properties( const method& m )
  : pimpl( new impl( m ) )
{  
}

method_properties::method_properties()
{
  // Out-of-line so impl is a complete type
}

method_properties::~method_properties()
{
  // Out-of-line so impl is a complete type
}

method_properties& 
method_properties::operator=( const method_properties& other )
{
  // Out-of-line so impl is a complete type
  pimpl = other.pimpl;
}


method_properties::method_properties( const method_properties& other )
  : pimpl( other.pimpl )
{
  // Out-of-line so impl is a complete type
}


string method_properties::get_property( int num_opt, const string& name ) const
{
  pair< int, string > cache_key( num_opt, name );

  {
    // Is it in the cache?
    map< pair< int, string >, string >::const_iterator cacheval
      = pimpl->cache.find( cache_key );
    if ( cacheval != pimpl->cache.end() )
      return cacheval->second;
  }

  make_string os;
  const method& m = pimpl->m;

  // Generate the value
  if ( name.size() == 1 )
    {
      switch ( name[0] ) 
	{
	case 'l': 
	  os << m.lh();
	  break;

	case 'p': 
	  for ( method::const_iterator i(m.begin()), e(m.end()); i!=e; ++i )
	    os << *i << ".";
	  break;

	case 'q': 
	  os << get_compressed_pn(m);
	  break;

	case 'Q': 
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
	  os << setw(num_opt) << max_blows_per_place(m); 
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

	case 'y':
	  os << method_symmetry_string( m );
	  break;

	case 'n':
	  // TODO: Should we cache fullname at same time?
	  os << method_libraries::lookup_method( m ).name();
	  break;

	case 'N':
	  // TODO: Should we cache fullname at same time?
	  os << method_libraries::lookup_method( m ).fullname();
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

	default:
	  throw logic_error( "Unknown variable requested" );
	}
    }
  else
    {
      throw logic_error( "Unknown variable requested" );
    }

  return pimpl->cache[ cache_key ] = os;
}


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

  histogram_entry( const format_string &f, 
		   const method_properties &m );

  void print( ostream &os, size_t count ) const;

  RINGING_FAKE_DEFAULT_CONSTRUCTOR( histogram_entry );

private:
  friend struct cmp;

  const format_string &f;

  method_properties props;
};

bool histogram_entry::cmp::operator()( const histogram_entry &x, 
				       const histogram_entry &y ) const
{
  assert( &x.f == &y.f );

  for ( vector< pair<int, string> >::const_iterator 
	  i( x.f.vars.begin() ), e( x.f.vars.end() );  i != e;  ++i )
    {
      // These are not taken into account when comparing strings
      // for stats.  For '%' and '$' this is because they are constant,
      // and for 'c' it's because it's still unknown.
      if ( i->second == "%" || i->second == "$" || i->second == "c" )
	continue;  

      string xval, yval;

      if ( i->second == "*" ) {
	xval = expression_cache::evaluate(i->first, x.props);
	yval = expression_cache::evaluate(i->first, y.props);
      }
      else {
	xval = x.props.get_property( i->first, i->second );
	yval = y.props.get_property( i->first, i->second );
      }

      if ( xval < yval ) 
	return true;
      else if ( xval > yval )
	return false;
    }

  // They're equal
  return false;
}

histogram_entry::histogram_entry
  ( const format_string &f, 
    const method_properties &props )
  : f( f ), props( props )
{
}



void histogram_entry::print( ostream &os2, size_t count ) const
{
  make_string os;

  try 
    {
      for ( string::const_iterator iter( f.fmt.begin() ),
	      end( f.fmt.end() ); iter != end; ++iter )
	{
	  if ( *iter == '$' ) 
	    {
	      string::const_iterator iter2(++iter);

	      while ( iter != end && isdigit(*iter) )
		++iter;
	      
	      int num_opt = 0;
	      if ( iter2 != iter )
		num_opt = atoi( string( &*iter2, &*iter ).c_str() );
	      
	      switch ( *iter )
		{
		case '%': case '$': 
		  os << *iter;
		  break;

		case 'c':
		  os << setw(num_opt) << count; 
		  break;

		case '*': 
		  os << expression_cache::evaluate(num_opt, props); 
		  break;

		default:
		  os << props.get_property( num_opt, string(1, *iter) );
		}
	    }
	  else
	    {
	      os << *iter;
	    }
	}
    }
  catch ( const script_exception& sc )
    {
      switch ( sc.type() )
	{
	case script_exception::suppress_output:
	  return;

	case script_exception::abort_search:
	  os2 << string(os) << flush;
	  throw exit_exception();
	  
	default:
	  throw;
	}
    }

  os2 << string(os) << flush;
}


void format_string::add_method_to_stats( const method_properties &props ) const
{
  statistics::add_entry( histogram_entry( *this, props ) );
}

void format_string::print_method( const method_properties &props, 
				  ostream &os ) const
{
  histogram_entry( *this, props ).print( os, 0 );
}

format_string::format_string( const string &infmt, 
			      format_string::format_type type )
  : has_name(false),
    has_falseness_group(false)
{
  if ( type == preparsed_type ) {
    fmt = infmt; 
    return;
  }

  assert( type == stat_type || type == normal_type || type == require_type );

  bool line_break = true;

  // State
  bool in_expr( type == require_type );
  bool in_exec_expr( false );
  stack< shared_pointer< make_string > > outfmts;
  stack<char> parens;

  outfmts.push( shared_pointer<make_string>( new make_string ) );

  for ( string::const_iterator iter( infmt.begin() ), end( infmt.end() ); 
	iter != end; ++iter )
    {
      // '%' exists for backwards compatibility.
      // It's only allowed at the top scope because of conflicts
      // with the modulus operator in expressions.
      if ( *iter == '$' || !in_expr && *iter == '%' ) 
	{
	  if ( ++iter == end ) 
	    throw argument_error( "Format ends with an unescaped `$'" );

	  // This is done before numeric prefix handling as $[ cannot have
	  // a numeric prefix (it conflicts with $N* used internally), and
	  // we need to set in_expr before any output occurs.
	  if (*iter == '[')
	    {
	      in_expr = true;
	      in_exec_expr = false;
	      outfmts.push( shared_pointer<make_string>( new make_string ) );
	      parens.push(*iter);
	    }

	  // $( ) is used to delimit expressions to be executed by 
	  // the shell.
	  if (*iter == '(')
	    {
	      in_expr = false;
	      in_exec_expr = true;
	      outfmts.push( shared_pointer<make_string>( new make_string ) );
	      parens.push('`'); // '(' is already used for nested paretheses
 	                        // in $[ ] expressions.
	    }

	  // Handle numeric prefix:
	  int num_opt = 0;
	  bool got_num_opt = false;
	  {
	    string::const_iterator iter2(iter);
	    
	    while ( iter != end && isdigit(*iter) ) ++iter;

	    if ( iter == end ) 
	      throw argument_error( "End of format reached whilst processing "
				    "numeric argument to `$'" );
	    if ( iter != iter2 ) {
	      got_num_opt = true;
	      num_opt = atoi( string( &*iter2, &*iter ).c_str() );
	    }
	  }

	  // Errors for formats that can only be used with -H or -R
	  switch ( *iter )
	    {
	    case 'c': 
	      if ( type != stat_type )
		throw argument_error( make_string() << "The `$" << *iter << "'"
				      " can only be used in stats formats" );
	      break;

	    case 'p': case 'q': case 'Q': case 'n': case 'N': case 'P': 
	      if ( type != normal_type && type != require_type )
		throw argument_error( make_string() << "The `$" << *iter << "'"
				      " can only be used in output formats" );
	      break;

	    case '%': case '$': case 'l': case 'r': case 'b': 
	    case 'C': case 'S': case 'M': case 'h': case 'F':
	    case 'o': case 'd': case 'u': case '[': case '(':
	    case 'y':
	      // Can be used in either
	      break;

	    default:
	      throw argument_error( make_string() << "Unknown format "
				    "specifier: `$" << *iter << "'" );
	    }


	  // Errors for formats that must or mustn't have a numeric argument
	  switch ( *iter )
	    {
	    case '%': case '$': case 'c': case 'b': case 'M': 
	    case 'o': case 'u':
	      // Option may but needn't have a number
	      break;

	    case 'n': case 'N': case 'p': case 'q': case 'Q': case 'l': 
	    case 'C': case 'S': case 'F': case 'd': case '[': case '(':
	    case 'y':
	      if ( got_num_opt )
		throw argument_error
		  ( make_string() << "The `$" << *iter << "' "
		    "format specifier must not be preceeded by "
		    "a number" );
	      break;

	    case 'r': case 'h': case 'P':
	      if ( !got_num_opt )
		throw argument_error
		  ( make_string() << "The `$" << *iter << "' "
		    "format specifier must be preceeded by a number" );
	      break;
	    }

	  switch ( *iter )
	    {
	    case 'n': case 'N':
	      has_name = true;
	      break;

	    case 'F':
	      has_falseness_group = true;
	      break;
	    }

	  // $[ options are converted into $N* options by the ']' 
	  // handling code. 
	  //
	  if (*iter != '[' && *iter != '(') {
	    // Normalise to '$' so later code can ignore '%'.
	    *outfmts.top() << '$';
	      
	    // Pass numeric arg through
	    if (got_num_opt) *outfmts.top() << num_opt;
	    
	    *outfmts.top() << *iter;

	    // Mark the option as used
	    vars.push_back( make_pair( num_opt, string(1, *iter) ) );
	  }
	}
      else if ( *iter == '\\' )
	{
	  if ( iter+1 == end ) {
	    // Supresses terminating line break
	    line_break = false;
	  }
	  else switch ( *++iter ) {
	    // Handle escaping at this stage
	    case 't': *outfmts.top() << '\t';  break;
	    case 'n': *outfmts.top() << '\n';  break;
	    // Escape '$' from the next stage
	    case '$': *outfmts.top() << "$$";  break;
	    // For everything else, \x -> literal x
	    default:  *outfmts.top() << *iter; break;
	  }
	}
      else if ( *iter == ')' && in_exec_expr )
	{
	  // Note: We do not count parentheses in $( ) expressions
	  assert( parens.top() == '`' );

	  string expr( *outfmts.top() );
	  outfmts.pop(); parens.pop();
	  if ( parens.empty() ) {
	    in_expr = ( type == require_type );
	    in_exec_expr = false;
	  }
	  else {
	    in_expr = ( parens.top() != '`' );
	    in_exec_expr = ( parens.top() == '`' );
	  }

	  try 
	    {
	      // $[N]* is magic.  It means look up pre-parsed expression N.
	      *outfmts.top() << '$' 
			     << store_exec_expression( expr ) 
			     << '*';
	    } 
	  catch ( const argument_error& e ) 
	    {
	      // Add more context to the error
	      throw argument_error( make_string() << "In $(...) expression: " 
				    << e.what() );
	    }

	}
      else if ( *iter == '(' && in_expr )
	{
	  parens.push(*iter);
	}
      else if ( *iter == ')' && in_expr )
	{
	  if ( parens.top() != '(' )
	    throw argument_error( "Unmatched parenthesis in $[ ] expression" );
	  parens.pop();
	}
      else if ( *iter == ']' && in_expr )
	{
	  if ( parens.top() != '[' )
	    throw argument_error( "Unmatched brackets in $[ ] expression" );

	  string expr( *outfmts.top() );
	  outfmts.pop(); parens.pop();
	  if ( parens.empty() ) {
	    in_expr = ( type == require_type );
	    in_exec_expr = false;
	  }
	  else {
	    in_expr = ( parens.top() != '`' );
	    in_exec_expr = ( parens.top() == '`' );
	  }

	  try 
	    {
	      // $[N]* is magic.  It means look up pre-parsed expression N.
	      *outfmts.top() << '$' 
			     << expression_cache::store( expression(expr) ) 
			     << '*';
	    } 
	  catch ( const argument_error& e ) 
	    {
	      // Add more context to the error
	      throw argument_error( make_string() << "In $[...] expression: " 
				    << e.what() );
	    }
	}
      else 
	*outfmts.top() << *iter;
    }
  
  assert( type == require_type ? in_expr : !in_expr );

  if ( line_break )
    *outfmts.top() << '\n';

  fmt = *outfmts.top();

  assert(( outfmts.pop(), outfmts.empty() ));
}


size_t format_string::parse_requirement( const string& str )
{
  format_string fs( str, require_type );
  return expression_cache::store( expression( fs.fmt ) );
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

void output_raw_count( unsigned long c )
{
  cout << c << "\n";
}

void output_count( unsigned long c )
{
  cout << "Found " << c << " methods\n";
}
