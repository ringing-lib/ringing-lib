// -*- C++ -*- args.cpp - argument-parsing things
// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2008, 2010
// Martin Bright <martin@boojum.org.uk>
// and Richard Smith <richard@ex-parrot.com>

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

#include "args.h"
#include "stringutils.h"

#include <ringing/row.h>
#include <ringing/streamutils.h>

#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#else
#include <cctype>
#endif
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#else
#include <iostream>
#endif

RINGING_USING_NAMESPACE
RINGING_USING_STD

bool option::process(const string& a, const arg_parser& ap) const {
  ap.error( "Unprocessed argument.  This is a bug in the program." );
  return false;
}

arg_parser::arg_parser(const string& n, const string& d,
		       const string& s) 
  : default_opt(0), progname(n), description(d), synopsis(s) 
{
#if RINGING_WINDOWS
  const char dir_sep[] = "/\\";
#else
  const char dir_sep = '/';
#endif

  // Strip directory names
  string::size_type i = progname.find_last_of( dir_sep );
  if ( i != string::npos )
    progname.erase(0, i+1);

  // Strip '.exe' suffix
#if RINGING_WINDOWS
  i = progname.rfind(".exe");
  if ( i == progname.size() - 4 /* = strlen(".exe") */ )
    progname.erase( progname.size()-4 );
#endif

  // Strip 'lt-' prefix (from libtool)
  i = progname.find("lt-");
  if ( i == 0 )
    progname.erase( 0, 3 );

#if RINGING_WINDOWS
  progname = lower_case( progname );
#endif
}

arg_parser::~arg_parser()
{
  for (args_t::iterator i=args.begin(), e=args.end(); i!=e; ++i) {
    delete *i; *i = NULL;
  }
}

void arg_parser::add(const option* o) {
  args_t::iterator i = args.insert(args.end(), o);
  if(o->shortname >= 32)
    shortindex.insert(pair<char, args_t::const_iterator>(o->shortname, i));
  if(!o->longname.empty()) 
    longindex.insert(pair<string, args_t::const_iterator>(o->longname, i));
}

void arg_parser::set_default(const option* o) {
  if (default_opt) delete default_opt;
  default_opt = o;
}

bool arg_parser::parse(int argc, char** argv) const
{
  int i;
  bool done_args = false;

  for(i = 1; i < argc; ++i) {
    if(!done_args && argv[i][0] == '-') { // Found an option
      if(argv[i][1] == '-') { // It's a long one
	if(argv[i][2] == '\0') { done_args = 1; continue; }
	char* t = argv[i] + 2; while(*t != '\0' && *t != '=') ++t;
	string s(argv[i] + 2, t); 
	longindex_t::const_iterator b = longindex.find(s);
       	if(b == longindex.end()) {
	  error( make_string() << "Unrecognised option: --" << s );
	  return false;
	}
	const option* a = *((*b).second);
	if(*t == '=') {
	  if(!(a->flags & option::takes_arg)) {
	    error( make_string() << "Option --" << s 
		   << " does not take an argument." );
	    return false;
	  }
	  if(!a->process(t + 1, *this)) return false;
	} else {
	  if((a->flags & option::takes_arg) && !(a->flags & option::opt_arg)) {
	    error( make_string() << "Option --" << s 
		   << " requires an argument." );
	    return false;
	  }
	  if(!a->process(string(), *this)) return false;
	}
      } else if(argv[i][1] != '\0') { // It's a short one
	char* t = argv[i] + 1;
	const option* a;
	do {
	  shortindex_t::const_iterator b = shortindex.find(*t);
	  if(b == shortindex.end()) {
	    error( make_string() << "Unrecognised option: -" << *t );
	    return false;
	  }
	  a = *((*b).second);
	  if(a->flags & option::takes_arg) {
	    if(a->flags & option::opt_arg) {
	      if(!a->process(t + 1, *this)) return false;
	    } else {
	      if(t[1] == '\0') {
		++i;
		if(i >= argc || (argv[i][0] == '-' && argv[i][1] != '\0')) {
		  error( make_string() << "Option -" << *t 
			 << " requires an argument." );
		  return false;
		} else
		  if(!a->process(argv[i], *this)) return false;
	      } else
		if(!a->process(t + 1, *this)) return false;
	    }
	  } else {
	    if(!a->process(string(), *this)) return false;
	    ++t;
	  }
	} while(*t != '\0' && !(a->flags & option::takes_arg));
      } else {
        if(!default_opt || !default_opt->process("-", *this)) return false;
      }
    } else {
      if(!default_opt || !default_opt->process(argv[i], *this)) return false;
    }
  }
  return true;
}

void arg_parser::wrap(const string& s, int l, int r, int c) const
{
  string::const_iterator j = s.begin(), k = j;
  while(j != s.end()) { 
    while(k != s.end() && (k - j) < (r - l) 
	  && !isspace(*k)) ++k;
    if(c + (k - j) > r) { 
      cout << '\n' << string(l, ' ');
      if(isspace(*j)) ++j;
      c = l;
    }
    while(j != k) { cout << *j++; ++c; }
    if (k != s.end()) ++k;
  }
}

void arg_parser::version() const
{
  cout << progname << " (Ringing Class Library) " RINGING_VERSION "\n";
  exit(0);
}

void arg_parser::help() const
{
  args_t::const_iterator i;
  int c;
  static const int desc_col = 25;
  string::const_iterator vtab;
  bool had_opt = false, had_arg = false;

  vtab = description.begin(); 
  while(vtab != description.end() && *vtab != '\v') ++vtab;
  if(vtab != description.begin()) {
    wrap(string(description.begin(), vtab), 0, 78, 0);
    cout << "\n\n";
  }

  cout << "Usage: " << progname << ' ';
  for(string::const_iterator l = synopsis.begin(); l != synopsis.end(); ++l)
    if(*l == '\n') cout << "\n  or:  " << progname << ' '; else cout << *l;
  cout << "\n\n";
     
  for(i = args.begin(); i != args.end(); ++i)
    if(*i) {
      if((*i)->flags & option::info) {
	cout << (*i)->desc << "\n";
	had_opt = true;
      } else if(isprint((*i)->shortname) || !(*i)->longname.empty()) {
	had_opt = true;
	cout << "  "; c = 2;
	if(isprint((*i)->shortname)) {
	  cout << '-' << (*i)->shortname; c += 2;
	  if(!(*i)->longname.empty()) { 
	    cout << ", "; c += 2;
	  }
	}
	if(!(*i)->longname.empty()) {
	  cout << "--" << (*i)->longname;
	  c += 2 + (*i)->longname.size();
	  if((*i)->flags & option::takes_arg) {
	    had_arg = true;
	    if((*i)->flags & option::opt_arg) { cout << '['; ++c; }
	    cout << '='; ++c;
	    cout << (*i)->optionname; c += (*i)->optionname.size();
	    if((*i)->flags & option::opt_arg) { cout << ']'; ++c; }
	  }
	}
	if(!(*i)->desc.empty()) {
	  if(c < desc_col - 1) { 
	    cout << string(desc_col - c, ' '); 
	    c = desc_col; 
	  } else if(c > desc_col + 5) {
	    cout << '\n' << string(desc_col, ' ');
	    c = desc_col;
	  } else {
	    cout << ' '; ++c;
	  }
	  wrap((*i)->desc, desc_col, 78, c);
	}
	cout << '\n';
      }
    }

  if(had_opt) cout << '\n';

  if(had_arg) cout << 
"Required or optional arguments to long options are also required or\n"
    "optional arguments to the corresponding short options.\n";
  
  if(vtab != description.end()) {
    wrap(string(vtab, description.end()), 0, 78, 0);
    cout << "\n\n";
  } else if ( had_arg ) {
    cout << "\n";
  }     
}

void arg_parser::usage() const
{
  cerr << "Usage: " << progname << ' ';
  for(string::const_iterator i = synopsis.begin(); i != synopsis.end(); ++i)
    if(*i == '\n') cerr << "\n  or:  " << progname << ' '; else cerr << *i;
  cerr << "\nType `" << progname << " --help' for more information.\n"; 
}

void arg_parser::error(const string &msg) const
{
  cerr << progname << ": " << msg << "\n";
}



boolean_opt::boolean_opt( char c, const string &l, const string &d,
			  bool &opt, bool val ) 
  : option(c, l, d), opt(opt), val(val)
{}

boolean_opt::boolean_opt( char c, const string &l, const string &d,
			  init_val_base<bool> &opt, bool val ) 
  : option(c, l, d), opt(opt.get()), val(val)
{}

bool boolean_opt::process( const string &, const arg_parser & ) const
{
  opt = val;
  return true;
}


repeated_boolean_opt::repeated_boolean_opt( char c, const string &l, 
                                            const string &d,
			                    int &opt ) 
  : option(c, l, d), opt(opt)
{}

repeated_boolean_opt::repeated_boolean_opt( char c, const string &l, 
                                            const string &d,
			                    init_val_base<int> &opt ) 
  : option(c, l, d), opt(opt.get())
{}

bool repeated_boolean_opt::process( const string &, const arg_parser & ) const
{
  ++opt;
  return true;
}


integer_opt::integer_opt( char c, const string &l, const string &d, 
			  const string &a, int &opt )
  : option(c, l, d, a), opt(opt)
{}

integer_opt::integer_opt( char c, const string &l, const string &d, 
			  const string &a, init_val_base<int> &opt )
  : option(c, l, d, a), opt(opt.get())
{}

integer_opt::integer_opt( char c, const string& l, const string& d, 
			  const string& a, int& opt, int default_val )
  : option(c, l, d, a, true), opt(opt), default_val(default_val)
{}

integer_opt::integer_opt( char c, const string& l, const string& d, 
			  const string& a, init_val_base<int>& opt, 
			  int default_val )
  : option(c, l, d, a, true), opt(opt.get()), default_val(default_val)
{}

bool integer_opt::process( const string &arg, const arg_parser &ap ) const
{
  if ( (flags & opt_arg) && arg.empty() )
    opt = default_val;
  else try {
    opt = lexical_cast<int>(arg);
  } catch ( bad_lexical_cast const& ) {
    ap.error( make_string() << "Invalid integer argument: \"" << arg << "\"" );
    return false;
  }
  return true;
}


string_opt::string_opt( char c, const string& l, const string& d, 
			const string& a, string& opt ) 
  : option(c, l, d, a), opt(opt)
{}

string_opt::string_opt( char c, const string& l, const string& d, 
			const string& a, string& opt, 
			const string& default_val  ) 
  : option(c, l, d, a, true), opt(opt), default_val(default_val)
{}

bool string_opt::process( const string &arg, const arg_parser & ) const
{
  if ( (flags & opt_arg) && arg.empty() )
    opt = default_val;
  else 
    opt = arg;
  return true;
}

strings_opt::strings_opt( char c, const string& l, const string& d, 
			  const string& a, vector<string>& opt ) 
  : option(c, l, d, a), opt(opt)
{}

bool strings_opt::process( const string &arg, const arg_parser & ) const
{
  opt.push_back(arg);
  return true;
}

delegate_opt::delegate_opt( char c, const string& l, const string& d, 
			    const string& a, void (*fn)(const string&) )
  : option(c, l, d, a), fn1(fn), fn_has_ap(false)
{}

delegate_opt::delegate_opt( char c, const string& l, const string& d, 
			    const string& a, 
			    void (*fn)(const string&, const arg_parser&) )
  : option(c, l, d, a), fn2(fn), fn_has_ap(true)
{}

bool delegate_opt::process( const string &arg, const arg_parser &ap ) const
{
  if (!fn_has_ap)
    (*fn1)( arg );
  else
    (*fn2)( arg, ap );
  return true;
}

range_opt::range_opt( char c, const string& l, const string& d, 
		      const string& a, pair<size_t, size_t>& opt ) 
  : option(c, l, d, a), opt(opt)
{}

bool range_opt::process( const string &arg, const arg_parser& ap ) const
{
  try
    {
      string::size_type pos( arg.find('-') );
      if ( pos == string::npos ) {
	opt.first = opt.second = lexical_cast<size_t>(arg);
	return true;
      }

      if ( pos != 0u )
	opt.first = lexical_cast<size_t>( arg.substr(0, pos) );
      else
	opt.first = 0;
 
      if ( pos != arg.size() - 1 )
	opt.second = lexical_cast<size_t>( arg.substr(pos + 1) );
      else
	opt.second = static_cast<size_t>(-1);

      if ( opt.second < opt.first ) {
	ap.error( "Negative length range" );
	return false;
      }

      return true;
    }
  catch ( bad_lexical_cast const& ) 
    {
      ap.error( make_string() << "Invalid range argument: \"" << arg << "\"" );
      return false;
    }
}

help_opt::help_opt()
  : option( '?', "help", "Print this help message" )
{
}

bool help_opt::process( const string &, const arg_parser &ap ) const
{
  ap.help();
  exit(0);
  return true; // To keep MSVC 5 happy
}

version_opt::version_opt()
  : option('V', "version", "Print the program version" )
{
}

bool version_opt::process( const string &, const arg_parser &ap ) const
{
  ap.version();
  return true; // To keep MSVC 5 happy
}

row_opt::row_opt( char c, const string &l, const string &d, const string& a,
	          row& opt )
  : option(c, l, d, a), opt(opt)
{}

bool row_opt::process( const string& arg, const arg_parser& ap ) const 
{
  try {
    opt = arg;
  } 
  catch ( bell::invalid const& ) {
    ap.error(make_string() << "Invalid bell in row '" << arg << "'");
    return false;
  }
  catch ( row::invalid const& ex ) {
    ap.error(make_string() << "Invalid row '" << arg << "'");
    return false;
  }
  return true;
}

