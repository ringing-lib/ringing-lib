// -*- C++ -*- format.cpp - classes to handle format specifiers
// Copyright (C) 2002, 2003, 2004, 2005, 2008, 2009, 2010, 2011, 2021
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

#include "format.h"
#include "expression.h"
#include "tokeniser.h"
#include "output.h"
#include "stringutils.h"
#if RINGING_OLD_INCLUDES
#include <iterator.h>
#include <algo.h>
#include <map.h>
#include <stack.h>
#include <stdexcept.h>
#include <functional.h>
#include <iostream.h>
#else
#include <iterator>
#include <algorithm>
#include <map>
#include <stack>
#include <stdexcept>
#include <functional>
#include <iostream>
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#else
#include <ostream>
#include <iomanip>
#include <fstream>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#else
#include <cctype>
#include <cassert>
#include <cstdlib>
#endif
#include <ringing/streamutils.h>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/pointers.h>
#include <ringing/library.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

class statistics
{
public:
  static RINGING_ULLONG output( ostream &os );
  static void add_entry( const histogram_entry &entry );

  // Public to avoid MSVC compilation errors
 ~statistics();

private:
  struct impl;
  scoped_pointer< impl > pimpl;

  static impl &instance();

  statistics();
};

// -------------------------------------------------------------

class fmtout::impl : public libout::interface {
public:
  impl( string const& fmt, string const& filename ) 
    : fs( fmt, format_string::normal_type ) {
    if ( filename.size() && filename != "-" ) 
      os.reset( new ofstream( filename.c_str() ) );
  }

private:
  virtual void append( library_entry const& entry ) {
    method_properties props( entry );
    fs.print_method( props, (os ? *os : cout) );
  }

  virtual void flush() {}

  format_string fs;
  scoped_pointer< ostream > os;
};

fmtout::fmtout( string const& fmt, string const& filename ) 
  : libout( new impl( fmt, filename ) ) 
{}

class statsout::impl : public libout::interface {
public:
  explicit impl( string const& fmt )
    : fs( fmt, format_string::stat_type ),
      os( cout )
  {}

private:
  virtual void append( library_entry const& entry ) {
    method_properties props( entry );
    fs.add_method_to_stats( props );
  }

  virtual void flush() {
    statistics::output( os );
  }

  format_string fs;
  ostream& os;
};

statsout::statsout( string const& fmt )
  : libout( new impl(fmt) )
{}

// -------------------------------------------------------------

RINGING_START_ANON_NAMESPACE
static bool have_falseness_groups = false;
static bool have_names = false;
static bool have_cc_ids = false;
static bool have_old_lhcodes = false;
static bool have_payloads = false;
static int  max_lead_offset = 0;
RINGING_END_ANON_NAMESPACE

bool formats_have_falseness_groups() { return have_falseness_groups; }
bool formats_have_names() { return have_names; }
bool formats_have_cc_ids() { return have_cc_ids; }
bool formats_have_payloads() { return have_payloads; }
bool formats_have_old_lhcodes() { return have_old_lhcodes; }
int  formats_max_lead_offset() { return max_lead_offset; }

// -------------------------------------------------------------

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

  void print( ostream &os, RINGING_ULLONG count ) const;

  RINGING_FAKE_DEFAULT_CONSTRUCTOR( histogram_entry )

private:
  friend struct cmp;

  const format_string &f;

  method_properties props;
};

bool histogram_entry::cmp::operator()( const histogram_entry &x, 
                                       const histogram_entry &y ) const
{
  assert( &x.f == &y.f );

  for ( vector< pair< pair<int,int>, string> >::const_iterator 
          i( x.f.vars.begin() ), e( x.f.vars.end() );  i != e;  ++i )
    {
      // These are not taken into account when comparing strings
      // for stats.  For '%', '$' and ')' this is because they are constant,
      // and for 'c' it's because it's still unknown.
      if ( i->second == "%" || i->second == "$" || i->second == ")" 
           || i->second == "c" )
        continue;  

      string xval, yval;

      if ( i->second == "*" ) {
        // TODO: We should cache these in case they contain lengthy 
        // command invocations and/or non-deterministic output
        xval = expression_cache::evaluate(i->first.first, x.props);
        yval = expression_cache::evaluate(i->first.first, y.props);
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

histogram_entry::histogram_entry( const format_string &f, 
                                  const method_properties &props )
  : f( f ), props( props )
{
}



void histogram_entry::print( ostream &os2, RINGING_ULLONG count ) const
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
              while ( iter != end && isdigit(*iter) ) ++iter;
              
              int num_opt = 0;
              if ( iter2 != iter )
                num_opt = atoi( string( &*iter2, &*iter ).c_str() );
              
              int num2_opt = 0;
              if ( *iter == ',' ) {
                ++iter; iter2 = iter;
                while ( iter != end && isdigit(*iter) ) ++iter;

                if ( iter2 != iter )
                  num2_opt = atoi( string( &*iter2, &*iter ).c_str() );
              }
             
              switch ( *iter )
                {
                case '%': case '$': case ')':
                  os << *iter;
                  break;

                case 'c':
                  os << setw(num_opt) << count; 
                  break;

                case '*': 
                  os << expression_cache::evaluate(num_opt, props); 
                  break;

                default:
                  os << props.get_property( make_pair(num_opt, num2_opt), 
                                            string(1, *iter) );
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

RINGING_START_ANON_NAMESPACE
static int parse_xdigit(char c)
{
  if (c >= '0' && c <= '9') 
    return c-'0';
  else if ( c >= 'A' && c <= 'F' )
    return 0xA + c-'A';
  else if ( c >= 'a' && c <= 'f' )
    return 0xA + c-'a';
  else
    throw argument_error("Invalid \\xNN character escape sequence");
}
RINGING_END_ANON_NAMESPACE

void format_string::print_method( const method_properties &props, 
                                  ostream &os ) const
{
  histogram_entry( *this, props ).print( os, 0 );
}

format_string::format_string( const string &infmt, 
                              format_string::format_type type )
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
        iter != end; ++iter ) {
    // '%' exists for backwards compatibility.
    // It's only allowed at the top scope because of conflicts
    // with the modulus operator in expressions.
    if ( *iter == '$' || !in_expr && *iter == '%' ) {
      if ( ++iter == end ) 
        throw argument_error( "Format ends with an unescaped `$'" );

      // This is done before numeric prefix handling as $[ cannot have
      // a numeric prefix (it conflicts with $N* used internally), and
      // we need to set in_expr before any output occurs.
      if (*iter == '[') {
        in_expr = true;
        in_exec_expr = false;
        outfmts.push( shared_pointer<make_string>( new make_string ) );
        parens.push(*iter);
      }

      // $( ) is used to delimit expressions to be executed by 
      // the shell.
      if (*iter == '(') {
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

      int num2_opt = 0;
      bool got_num2_opt = false;
      if ( *iter == ',' ) {
        ++iter;
        string::const_iterator iter2(iter);
        while ( iter != end && isdigit(*iter) ) ++iter;

        if ( iter == end ) 
          throw argument_error( "End of format reached whilst processing "
                                "numeric argument to `$'" );
 
        if ( iter != iter2 ) {
          got_num2_opt = true;
          num2_opt = atoi( string( &*iter2, &*iter ).c_str() );
        }
      }

      // Errors for formats that can only be used with -H or -R
      switch ( *iter ) {
        case 'c': 
          if ( type != stat_type )
            throw argument_error( make_string() << "The `$" << *iter << "'"
                                  " can only be used in stats formats" );
          break;

        case '#': case 'T':
          if ( type != normal_type )
            throw argument_error( make_string() << "The `$" << *iter << "'"
                                  " can only be used in output formats" );
          break;

        case 'p': case 'q': case 'Q': case 'n': case 'N': 
        case 'i':
          if ( type != normal_type && type != require_type 
               && parens.empty() )
            throw argument_error( make_string() << "The `$" << *iter << "'"
                                  " can only be used in output formats" );
          break;

        case '%': case '$': case 'l': case 'r': case 'b': 
        case 'C': case 'S': case 'M': case 'h': case 'F':
        case 'o': case 'd': case 'u': case '[': case '(':
        case 'y': case 'O': case ')': case 'L': case 'D':
        case 'P': case 's': case 'a': case '?': case 'B':
        case 'G': case 'V': case 'U':
          // Can be used in either
          break;

        default:
          throw argument_error( make_string() << "Unknown format "
                                "specifier: `$" << *iter << "'" );
        }


      // Errors for formats that must or mustn't have a numeric argument
      switch ( *iter ) {
        case '%': case '$': case 'c': case 'b': case 'M': 
        case 'o': case 'u': case ')': case 'L': case 's':
        case 'i': case '#': case '?': case 'B': case 'G':
        case 'p': case 'q':
          // Option may but needn't have a number
          break;

        case 'n': case 'N': case 'Q': case 'l': 
        case 'C': case 'S': case 'F': case 'd': case '[': case '(':
        case 'y': case 'O': case 'D': case 'a': case 'T': case 'V':
        case 'U':
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

      if ( got_num2_opt ) switch ( *iter ) {
        case 'p': case 'q': break;

        default:
          throw argument_error
            ( make_string() << "The `$" << *iter << "' "
              "format specifier must not be preceeded by two numbers" );
      }

      switch ( *iter ) {
        case 'n': case 'N': have_names = true; break;
        case 'i': have_cc_ids = true; break;
        case 'a': have_payloads = true; break;
        case 'F': have_falseness_groups = true; break;
        case 'D': have_old_lhcodes = true; break;
        case 'h': case 'r':
          max_lead_offset = max( max_lead_offset, num_opt );
      }

      // $[ options are converted into $N* options by the ']' 
      // handling code. 
      //
      if (*iter != '[' && *iter != '(') {
        // Normalise to '$' so later code can ignore '%'.
        *outfmts.top() << '$';
          
        // Pass numeric arg through
        if (got_num_opt) *outfmts.top() << num_opt;
        if (got_num2_opt) *outfmts.top() << ',' << num2_opt;
        
        *outfmts.top() << *iter;

        // Mark the option as used
        if ( !in_expr && !in_exec_expr ) 
          vars.push_back( make_pair( make_pair(num_opt, num2_opt),
                                     string(1, *iter) ) );
      }
    }
    else if ( *iter == '\\' ) {
      if ( iter+1 == end ) {
        // Supresses terminating line break
        line_break = false;
      }
      else switch ( *++iter ) {
        // Handle escaping at this stage
        case 'a': *outfmts.top() << '\a';  break;
        case 'b': *outfmts.top() << '\b';  break;
        case 'f': *outfmts.top() << '\f';  break;
        case 'n': *outfmts.top() << '\n';  break;
        case 'r': *outfmts.top() << '\r';  break;
        case 't': *outfmts.top() << '\t';  break;
        case 'v': *outfmts.top() << '\v';  break;
        // Escape '$' from the next stage
        case '$': *outfmts.top() << "$$";  break;
        case 'x': {
          unsigned char value = 0x00;
          value += parse_xdigit(*++iter) * 0x10;
          value += parse_xdigit(*++iter);
          *outfmts.top() << char(value);
        } break;
        // For everything else, \x -> literal x
        default:  *outfmts.top() << *iter; break;
      }
    }
    else if ( *iter == ')' && in_exec_expr ) {
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

      try {
        int const n = store_exec_expression( expr );
        // $[N]* is magic.  It means look up pre-parsed expression N.
        *outfmts.top() << '$' << n << '*';
        if ( !in_expr && !in_exec_expr )
          vars.push_back( make_pair( make_pair(n,0), "*" ) );
      } 
      catch ( const argument_error& e ) {
        // Add more context to the error
        throw argument_error( make_string() << "In $(...) expression: " 
                              << e.what() );
      }
    }
    else if ( *iter == '(' && in_expr ) {
      parens.push(*iter);
      *outfmts.top() << *iter;
    }
    else if ( *iter == ')' && in_expr ) {
      if ( parens.top() != '(' )
        throw argument_error( "Unmatched parenthesis in $[ ] expression" );
      parens.pop();
      *outfmts.top() << *iter;
    }
    else if ( *iter == ']' && in_expr ) {
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

      try {
        int const n = expression_cache::store( expression(expr) );
        // $[N]* is magic.  It means look up pre-parsed expression N.
        *outfmts.top() << '$' << n << '*';
        if ( !in_expr && !in_exec_expr )
           vars.push_back( make_pair( make_pair(n,0), "*" ) );
      } 
      catch ( const argument_error& e ) {
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
  typedef map< histogram_entry, RINGING_ULLONG, 
               histogram_entry::cmp > map_type;
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

RINGING_ULLONG statistics::output( ostream &os )
{
  RINGING_ULLONG count(0ul);

  for ( impl::map_type::const_iterator i( instance().histogram.begin() ),
          e( instance().histogram.end() ); i != e;  ++i ) {
    i->first.print( os, i->second );
    count += i->second;
  }

  return count;
}

void statistics::add_entry( const histogram_entry &entry )
{
  // Grrr... This is needed because I've got a bug in the code somewhere
  // { make_string ms; entry.print( ms.out_stream(), 0 ); }

  ++instance().histogram[entry];
}

void clear_status()
{
  cerr << '\r' << string( display_columns() - 1, ' ' ) << '\r';
}

void output_status( const method &m )
{
  string s = m.format(method::M_DASH);

  int width = display_columns() - 12;
  if ( s.size() > width ) 
    s = s.substr(0, width) + "...";
  
  clear_status();
  cerr << "Trying " << s << flush;
}

void output_raw_count( ostream& out, RINGING_ULLONG c )
{
  out << c << "\n";
}

void output_count( ostream& out, RINGING_ULLONG c )
{
  out << "Found " << c << " methods\n";
}

void output_node_count( ostream& out, RINGING_ULLONG c )
{
  out << "Searched " << c << " nodes\n";
}
