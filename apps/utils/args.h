// -*- C++ -*- args.h - argument-parsing things
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

#ifndef RINGING_ARGS_INCLUDED
#define RINGING_ARGS_INCLUDED

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <map.h>
#include <list.h>
#else
#include <map>
#include <list>
#endif
#include <string>

RINGING_USING_STD

class arg_parser;

class option {
public:
  char shortname;
  string longname;
  string desc;
  string optionname;
  int flags;
  enum flag_t { takes_arg = 1, opt_arg = 2, info = 4 };

  option(char c, string l, string d) 
    : shortname(c), longname(l), desc(d), flags(0) {}
  option(string d) : shortname(0), desc(d), flags(info) {}
  option(char c, string l, string d, string n, bool opt = false)
    : shortname(c), longname(l), desc(d), optionname(n), 
      flags(opt ? (takes_arg | opt_arg) : takes_arg) {}
  option() : shortname(0), flags(0) {}
  virtual ~option() {}

  virtual bool process(const string& a, const arg_parser& ap) const;
};

class arg_parser {
protected:
  typedef list<const option*> args_t;
  args_t args;
  const option* default_opt;
  typedef map<char, args_t::const_iterator> shortindex_t;
  shortindex_t shortindex;
  typedef map<string, args_t::const_iterator> longindex_t;
  longindex_t longindex;
  string progname, description, synopsis;

public:
  arg_parser(const string& progname, const string& description, 
	     const string& synoposis);
 ~arg_parser();

  void add(const option* o);

  // The default option is used for text on the command line 
  // that is not an option (i.e. not prefixed with a dash).
  void set_default(const option* o);

  bool parse(int argc, char** argv) const;
  void help() const;
  void usage() const;
  void version() const;
  void error(const string &s) const;

private:
  void wrap(const string& s, int l, int r, int c) const;

  // Unimplemented copy constructor & assignment operator
  arg_parser(const arg_parser&);
  arg_parser& operator=(const arg_parser&);
};

//
// Some convenient subclasses of option:
//

class boolean_opt : public option
{
public:
  boolean_opt( char c, const string &l, const string &d,
	       bool &opt, bool val=true ) 
    : option(c, l, d), opt(opt), val(val)
  {}

private:
  // Sets opt = val.
  virtual bool process( const string &, const arg_parser & ) const;
  bool &opt, val;
};


class integer_opt : public option {
public:
  // Use this constructor if the argument is not optional
  integer_opt( char c, const string &l, const string &d, const string &a,
	       int &opt )
    : option(c, l, d, a), opt(opt)
  {}

  // Use this constructor if the argument is optional
  integer_opt( char c, const string& l, const string& d, const string& a,
	       int& opt, int default_val )
    : option(c, l, d, a, true), opt(opt), default_val(default_val)
  {}

private:
  // If an argument is given, sets opt to the integer value of that string
  // Otherwise, if a default_value was passed to the constructor, set
  // opt to that.
  virtual bool process( const string&, const arg_parser& ) const;
  int &opt, default_val;
};

class string_opt : public option {
public:
  string_opt( char c, const string& l, const string& d, const string& a,
	      string& opt ) 
    : option(c, l, d, a), opt(opt)
  {}

private:
  // Sets opt to the given argument.
  virtual bool process( const string&, const arg_parser& ) const;
  string &opt;
};

class delegate_opt : public option {
public:
  delegate_opt( char c, const string& l, const string& d, const string& a,
		void (*fn)(const string&) )
    : option(c, l, d, a), fn1(fn), fn_has_ap(false)
  {}

  delegate_opt( char c, const string& l, const string& d, const string& a,
		void (*fn)(const string&, const arg_parser&) )
    : option(c, l, d, a), fn2(fn), fn_has_ap(true)
  {}

private:
  // Calls the function fn with the argument.
  virtual bool process( const string&, const arg_parser & ) const;

  union {
    void (*fn1)(const string&);
    void (*fn2)(const string&, const arg_parser& );
  };

  bool fn_has_ap;
};

class help_opt : public option {
public:
  // Default uses  -?, --help   'Print this help message'
  help_opt();
  help_opt( char c, const string& l, const string& d ) : option(c, l, d) {}

private:
  // Just calls ap.help() and exits successfully
  virtual bool process( const string&, const arg_parser& ap ) const;
};

class version_opt : public option {
public:
  // Default uses  -V, --version   'Print the program version'
  version_opt();
  version_opt( char c, const string &l, const string &d ) : option(c, l, d) {}

private:
  // Just calls ap.version() and exits successfully
  virtual bool process( const string &, const arg_parser & ) const;
};

#endif