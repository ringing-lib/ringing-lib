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
#include <iostream.h>
#include <map.h>
#include <list.h>
#else
#include <iostream>
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

  virtual bool process(const string& a, const arg_parser& ap) const {
    cerr << "Unprocessed argument.  This is a bug in the program.\n";
    return false;
  }
};

class arg_parser {
protected:
  typedef list<const option*> args_t;
  args_t args;
  typedef map<char, args_t::const_iterator> shortindex_t;
  shortindex_t shortindex;
  typedef map<string, args_t::const_iterator> longindex_t;
  longindex_t longindex;
  string progname;

public:
  arg_parser(const string& n, const option* arg_handler = NULL) 
    { progname = n; args.push_back(arg_handler); }
  void add(const option* o);
  bool parse(int argc, char** argv) const;
  void help() const;
  void usage() const;

private:
  void wrap(const string& s, int l, int r, int c) const;
};

#endif
