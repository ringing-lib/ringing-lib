// -*- C++ -*- args.h - argument-parsing things
// Copyright (C) 2001, 2002, 2003, 2008, 2010, 2011, 2022
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

#ifndef RINGING_ARGS_INCLUDED
#define RINGING_ARGS_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <map>
#include <list>
#include <vector>
#include <string>
#include "init_val.h"

RINGING_START_NAMESPACE
class row;
class method;
RINGING_END_NAMESPACE


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
  virtual bool validate(const arg_parser& ap) const;
};

class arguments_base {
public:
  void init();

  virtual void bind( arg_parser& p ) = 0;
  virtual bool validate( arg_parser& p ) = 0;
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

  void process( arguments_base& args, int argc, char const* const* argv );

  bool parse(int argc, char const* const* argv) const;
  bool validate() const;

  void help() const;
  void usage() const;
  void version() const;
  void error(const string &s) const;

  std::string program_name() const { return progname; }

private:
  void wrap(const string& s, int l, int r, int c) const;
  bool do_parse(int argc, char const* const* argv) const;

  // Unimplemented copy constructor & assignment operator
  arg_parser(const arg_parser&);
  arg_parser& operator=(const arg_parser&);
};

//
// Some convenient subclasses of option:
//
// In all of the constructors, 
//   c    is the short form of the option,
//   l    is the long form of the option,
//   d    is the description string
//   a    is the name of the argument (if applicable)
// 

class boolean_opt : public option {
public:
  boolean_opt( char c, const string &l, const string &d,
	       bool &opt, bool val=true );

  boolean_opt( char c, const string &l, const string &d,
	       init_val_base<bool> &opt, bool val=true );
  
private:
  // Sets opt = val.
  virtual bool process( const string &, const arg_parser & ) const;
  bool &opt, val;
};

// -x sets opt=1, -xx sets opt=2, -xxx sets opt=3, etc.
class repeated_boolean_opt : public option {
public:
  repeated_boolean_opt( char c, const string &l, const string &d,
                        int &opt );

  repeated_boolean_opt( char c, const string &l, const string &d,
                        init_val_base<int> &opt );
  
private:
  // Sets opt = val.
  virtual bool process( const string &, const arg_parser & ) const;
  int &opt;
};

class integer_opt : public option {
public:
  // Use these constructor if the argument is not optional
  integer_opt( char c, const string &l, const string &d, const string &a,
	       int &opt );

  integer_opt( char c, const string &l, const string &d, const string &a,
	       init_val_base<int> &opt );

  // Use this constructor if the argument is optional
  integer_opt( char c, const string& l, const string& d, const string& a,
	       int& opt, int default_val );

  integer_opt( char c, const string& l, const string& d, const string& a,
	       init_val_base<int>& opt, int default_val );

protected:
  int get_value() const { return opt; }

private:
  // If an argument is given, sets opt to the integer value of that string
  // Otherwise, if a default_value was passed to the constructor, set
  // opt to that.
  virtual bool process( const string&, const arg_parser& ) const;
  int &opt, default_val;
};

class string_opt : public option {
public:
  // Use these constructor if the argument is not optional
  string_opt( char c, const string& l, const string& d, const string& a,
	      string& opt );

  // Use this constructor if the argument is optional
  string_opt( char c, const string& l, const string& d, const string& a,
	      string& opt, const string& default_val );

private:
  // Sets opt to the given argument.
  virtual bool process( const string&, const arg_parser& ) const;
  string &opt, default_val;
};

class strings_opt : public option {
public:
  // Use these constructor if the argument is not optional
  strings_opt( char c, const string& l, const string& d, const string& a,
	       vector<string>& opt );

private:
  // Sets opt to the given argument.
  virtual bool process( const string&, const arg_parser& ) const;
  vector<string>& opt;
};

class delegate_opt : public option {
public:
  delegate_opt( char c, const string& l, const string& d, const string& a,
		void (*fn)(const string&) );

  delegate_opt( char c, const string& l, const string& d, const string& a,
		void (*fn)(const string&, const arg_parser&) );

private:
  // Calls the function fn with the argument.
  virtual bool process( const string&, const arg_parser & ) const;

  union {
    void (*fn1)(const string&);
    void (*fn2)(const string&, const arg_parser& );
  };

  bool fn_has_ap;
};

// A range in one of the following forms:
//
//   "M-N"   opt.first = M, opt.second = N
//   "-N"    opt.first = 0, opt.second = N
//   "M-"    opt.first = M, opt.second = size_t(-1)
//   "N"     opt.first = M, opt.second = N
// 
class range_opt : public option {
public:
  // Use these constructor if the argument is not optional
  range_opt( char c, const string& l, const string& d, const string& a,
	     pair<size_t, size_t>& opt );

private:
  // Sets opt to the given argument.
  virtual bool process( const string&, const arg_parser& ) const;
  pair<size_t, size_t>& opt;
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

class bells_opt : public integer_opt {
public:
  bells_opt( init_val_base<int>& opt );

private:
  virtual bool validate( const arg_parser& ap ) const;
};

class row_opt : public option {
public:
  row_opt( char c, const string &l, const string &d, const string& a,
	   RINGING_PREFIX row& opt );

private:
  // Sets opt = val
  virtual bool process( const string& val, const arg_parser& ap ) const;
  RINGING_PREFIX row &opt;
};

class method_opt : public option {
public:
  method_opt( char c, const string &l, const string &d, const string& a,
	      string& methstr, const init_val_base<int>& bells, 
              RINGING_PREFIX method& meth, bool required = true );

private:
  // Process sets methstr = val
  virtual bool process( const string& val, const arg_parser& ap ) const;
  // Validate set meth = method(methstr, bells);
  virtual bool validate( const arg_parser& ap ) const;

  string &methstr;
  const init_val_base<int>& bells;
  RINGING_PREFIX method& meth;
  bool required;
};

#endif
