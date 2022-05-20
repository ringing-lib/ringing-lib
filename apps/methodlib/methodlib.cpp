// -*- C++ -*- psline.cpp - print out lines for methods
// Copyright (C) 2021, 2022 Richard Smith <richard@ex-parrot.com>

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
#include <ringing/library.h>
#include <ringing/cclib.h>
#include <ringing/mslib.h>
#include <ringing/xmllib.h>
#include <ringing/methodset.h>
#include <ringing/method_stream.h>
#include <ringing/litelib.h>
#include <ringing/streamutils.h>

#include <map>
#include <iostream>

#include "init_val.h"
#include "args.h"


RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  arguments( int argc, char const *argv[] );

  init_val<int,0>       bells;
  init_val<int,0>       length;
  vector<string>        titles;
  vector<string>        payloads;
  init_val<bool,false>  read_titles;
  string                starts_with;
  init_val<bool,false>  inc_bells;
  init_val<bool,false>  copy_payload;
  init_val<bool,false>  full_title;
  init_val<bool,false>  pn_only;
  vector<string>        libs;
  string                suffixstr;
  vector<string>        suffixes;

private:
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt
           ( 'b', "bells",  "The number of bells.", "BELLS", bells ) );

  p.add( new integer_opt
           ( 'l', "length", "Require N changes per lead.", "N", length ) );

  p.add( new strings_opt
           ( 't', "title",  "Find method with this full title.", "TITLE", 
             titles ) );

  p.add( new string_opt
           ( '\0', "starts-with",  "Find method whose name start with STR.", 
             "STR", starts_with ) );

  p.add( new boolean_opt
           ( 'T', "read-titles",  "Read titles to find from standard input.",
             read_titles ) );

  p.add( new boolean_opt
           ( 'B', "print-bells",  "Include the number of bells in the output",
             inc_bells ) );

  p.add( new boolean_opt
           ( 'a', "copy-payload",  "Treat the input as tab-delimited with "
                "the name in the first field, and copy the remaining fields "
                "into the output instead of the full method title",
             copy_payload ) );

  p.add( new boolean_opt
           ( 'f', "full-title",  "Print the full method title, including "
                "class and stage names",
             full_title ) );

  p.add( new boolean_opt
           ( 'P', "pn-only",  "Print only the place notation",
             pn_only ) );

  p.add( new string_opt
           ( 'S', "method-suffixes",  
             "Try suffixes when searching for methods by name.", 
             "STRS", suffixstr ) );

  p.set_default( new strings_opt( '\0', "", "", "", libs ) );
}

// TODO: This is duplicated in ringing/library.cpp
RINGING_START_ANON_NAMESPACE
vector<string> split_path( string const& p )
{
  vector<string> pp;
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

RINGING_END_ANON_NAMESPACE


bool arguments::validate( arg_parser& ap )
{
  // Do this here so that it has lower priority than any libraries given
  // explicitly on the command line
  if ( char const* const lib_env = getenv("METHOD_LIBRARY") )
    libs.push_back(lib_env);

  if ( libs.empty() ) {
    ap.error( "Please provide at least one library" );
    return false;
  }

  if ( bells == 1 || bells > int(bell::MAX_BELLS) ) {
    ap.error( make_string() << "The number of bells must be between 2 and " 	              << bell::MAX_BELLS << " (inclusive)" );
    return false;
  }

  if ( read_titles + !titles.empty() + !starts_with.empty() > 1 ) {
    ap.error( "At most one of -t, -T and --starts-with may be used");
    return false;
  }

  if ( copy_payload + full_title + pn_only > 1 ) {
    ap.error( "At most one of -a, -f and -P may be used");
    return false;
  }

  suffixes = split_path(suffixstr);
  if (suffixes.empty())
    suffixes.push_back(string());

  if ( suffixstr.size() && titles.size() )
    payloads = titles;

  return true;
}

arguments::arguments( int argc, char const *argv[] )
{
  arg_parser ap( argv[0], "methodlib -- extract methods from a method library",
                 "[OPTIONS] [LIBRARY]" );
  bind( ap );

  if ( !ap.parse(argc, argv) ) {
    ap.usage();
    exit(1);
  }

  if ( !validate(ap) )
    exit(1);
}

void read_library( const string &filename, libout& out ) {
  try {
   library l( filename );

    if (!l.good()) {
      cerr << "Unable to read library " << filename << "\n";
      exit(1);
    }

    if ( l.empty() ) {
      cerr << "The library " << filename << " appears to be empty\n";
      exit(1);
    }

    copy( l.begin(), l.end(), back_inserter(out) );
  }
  catch ( const exception &e ) {
    cerr << "Unable to read library: " << filename << ": " 
         << e.what() << '\n';
    exit(1);
  }
}

bool read_one_title( arguments& args ) {
  args.titles.clear();
  args.payloads.clear();

  string line;
  size_t i;
  do {
    if ( !getline( cin, line ) ) return false;
    if ( (i = line.find('#')) != string::npos )
      line.erase(i);
    i = line.find_last_not_of(" \t\f\v\n\r");
  } while ( i == string::npos );

  // Trim trailing white space
  line = line.substr(0, i+1);
  if ( args.copy_payload ) {
    i = line.find('\t');
    if ( i == string::npos ) {
      args.titles.push_back(line);
      args.payloads.push_back(string());
    } else {
      args.titles.push_back(line.substr(0, i));
      args.payloads.push_back(line.substr(i+1));
    }
  }
  else {
    args.titles.push_back(line);
    if (args.suffixstr.size())
      args.payloads.push_back(line);
  }
  return true;
}

bool apply_filters( arguments& args, library_entry const& le, libout& out ) {
  if ( (!args.bells || le.bells() == args.bells) &&
       (!args.length || le.meth().length() == args.length) ) {
    out.append(le);
    return true;
  }
  else return false;
}

bool filter_by_titles( library const& meths, arguments& args, libout& out ) {
  bool okay = true;
  for ( size_t i=0, n=args.titles.size(); i != n; ++i ) {
    library_entry le;
    for ( string suffix : args.suffixes ) {
      string title( args.titles[i] );
      if (suffix.length())
        (title += ' ') += suffix;
      le = meths.find(title);
      if (!le.null()) break;
    }

    if (le.null()) {
      cerr << "Method not found: " << args.titles[i] << endl;
      okay = false;
    }
    else {
      if (args.copy_payload)
        le.set_facet<litelib::payload>( args.payloads[i] );
      else if (args.suffixstr.size())
        le.set_facet<litelib::payload>( args.titles[i] );
      if (!apply_filters( args, le, out ))
        okay = false;
    }
  }
  return okay;
}

bool filter_by_start( library const& meths, arguments& args, libout& out ) {
  bool okay = false;
  for ( library::const_iterator i(meths.begin()), e(meths.end()); i != e; ++i )
    if ( args.starts_with.empty() 
           || i->meth().fullname().rfind(args.starts_with, 0) == 0 )
      if ( apply_filters( args, *i, out ) )
        okay = true;
  return okay;
}

int main(int argc, char const** argv) {
  // Register mslib last, as various things can accidentally match it
  cclib::registerlib();
  xmllib::registerlib();
  mslib::registerlib();

  library::setpath_from_env();

  arguments args( argc, argv );

  methodset meths;
  for ( vector< string >::const_iterator 
          i( args.libs.begin() ), e( args.libs.end() ); i != e; ++i )
    read_library( *i, meths );
 
  method_stream::name_form form 
    = args.pn_only    ? method_stream::none
    : args.full_title ? method_stream::full_title  
    :                   method_stream::payload_or_name; 
  method_stream out(args.inc_bells, form);
  
  bool okay = false;
  if (args.read_titles) {
    okay = true;
    while ( read_one_title(args) )
      if (!filter_by_titles(meths, args, out))
        okay = false;
  }
  else if (args.titles.size())
    okay = filter_by_titles(meths, args, out);
  else
    okay = filter_by_start(meths, args, out);
  return okay ? 0 : 1;
}
