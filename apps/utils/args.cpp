// -*- C++ -*- args.cpp - argument-parsing things
// Copyright (C) 2001 Martin Bright <martin@boojum.org.uk>

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
#include "args.h"

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
  : progname(n), description(d), synopsis(s) 
{
}

void arg_parser::add(const option* o) {
  args_t::iterator i = args.insert(args.end(), o);
  if(o->shortname >= 32)
    shortindex.insert(pair<char, args_t::const_iterator>(o->shortname, i));
  if(!o->longname.empty()) 
    longindex.insert(pair<string, args_t::const_iterator>(o->longname, i));
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
        if(args.front() && !args.front()->process("-", *this)) return false;
      }
    } else {
      const option* a = *(args.begin());
      if(a && !a->process(argv[i], *this)) return false;
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
    ++k;
  }
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

  if(had_arg) cout << 
"Required or optional arguments to long options are also required or\n"
    "optional arguments to the corresponding short options.\n";
  if(had_opt) cout << '\n';
  
  if(vtab != description.end()) {
    wrap(string(vtab, description.end()), 0, 78, 0);
    cout << '\n';
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


bool string_opt::process( const string &arg, const arg_parser & ) const
{
  opt = arg;
  return true;
}

bool boolean_opt::process( const string &, const arg_parser & ) const
{
  opt = val;
  return true;
}

bool integer_opt::process( const string &arg, const arg_parser &ap ) const
{
  if ( (flags & opt_arg) && arg.empty() )
    opt = default_val;
  else {
    istringstream is(arg); 
    is >> opt;
    if(!is || is.get() != EOF) {
      ap.error( make_string() << "Invalid integer argument: \"" << arg << "\"" );
      return false;
    }
  }
  return true;
}

bool delegate_opt::process( const string &arg, const arg_parser &ap ) const
{
  if (!fn_has_ap)
    (*fn1)( arg );
  else
    (*fn2)( arg, ap );
  return true;
}

bool help_opt::process( const string &, const arg_parser &ap ) const
{
  ap.help();
  exit(0);
  return true; // To keep MSVC 5 happy
}
