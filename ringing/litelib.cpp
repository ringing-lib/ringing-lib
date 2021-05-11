// -*- C++ -*- litelib.cpp - Lightweight library format
// Copyright (C) 2007, 2009, 2010, 2011, 2017, 2021
// Richard Smith <richard@ex-parrot.com>.

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_INCLUDES
#include <iosfwd.h>
#include <fstream.h>
#else
#include <iosfwd>
#include <fstream>
#endif

#include <ringing/litelib.h>
#include <ringing/lexical_cast.h>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_DEFINE_LIBRARY_FACET( litelib::payload );

class litelib::impl : public library_base {
public:
  impl(int b, const string& filename, int flags)
    : b(b), f_owner( new ifstream(filename.c_str()) ), f(*f_owner), 
      flags(flags)
  { ok = f.good(); skip_bom(); }

  impl(int b, istream& is, int flags)
    : b(b), f(is), flags(flags)
  { ok = f.good(); skip_bom(); }

  // Iterators into the library
  class entry_type;
  friend class entry_type;
  virtual const_iterator begin() const;

private:
  void skip_bom();

  virtual bool good(void) const { return bool(f); }

private:
  int b;
  scoped_pointer<ifstream> f_owner;
  istream& f;
  int flags;
  bool ok;
};

void litelib::impl::skip_bom()
{
  // Handle UTF-8 BOM: EF BB BF
  if ( f && f.peek() == 0xEF ) {
    f.get();
    if ( f && f.peek() != 0xBB ) 
      ok = false;
    else { 
      f.get();
      if ( f && f.peek() != 0xBF ) 
        ok = false;
      else
        f.get();
    }
  }
}

litelib::litelib(int b, const string& filename, int flags)
  : library( new impl(b, filename, flags) ) {}

litelib::litelib(int b, istream& is, int flags)
  : library( new impl(b, is, flags) ) {}

class litelib::impl::entry_type : public library_entry::impl
{
public:
  explicit entry_type(int b, int flags) : b(b), flags(flags) {}

private:
  virtual string name() const;
  virtual string base_name() const;
  virtual string pn() const;
  virtual int bells() const { return b; }

  virtual bool readentry( library_base &lb );

  virtual library_entry::impl *clone() const { return new entry_type(*this); }

  virtual bool has_facet( const library_facet_id& id ) const;

  string get_payload() const;
  virtual shared_pointer< library_facet_base >
    get_facet( const library_facet_id& id ) const;

  int read_bells();

  // The current line
  string linebuf;

  string::size_type linestart;

  int b; // bells
  int flags;
};

string litelib::impl::entry_type::name() const 
{
  if ( flags & litelib::payload_is_name ) 
    return get_payload();
  else
    return string(); 
}

static void trim_trailing_whitespace( string &line )
{
  string::iterator b( line.begin() ), j( line.end() );
  while ( j-1 != b && isspace(*(j-1)) ) --j;
  line = string( b, j );
}

static bool remove_suffix( string& n, string const& suffix )
{
  string::size_type i = n.find(suffix);
  if ( i != string::npos && i == n.length() - suffix.length() ) {
    n.erase(i); trim_trailing_whitespace(n);
    return true;
  }
  return false;
}

string litelib::impl::entry_type::base_name() const 
{
  string n = name();
  if ( remove_suffix( n, method::stagename(b) ) ) {
    bool had_class = false;
    for ( method::m_class c = method::M_BOB; 
            c <= method::M_SLOW_COURSE && !had_class;  
            c = (method::m_class)( (int)c + 1 ) )
      if ( remove_suffix( n, method::classname(c) ) )
        had_class = true;
   
    if (had_class) remove_suffix( n, method::classname( method::M_LITTLE ) );
    remove_suffix( n, method::classname( method::M_DIFFERENTIAL ) );
  }
  return n;
}

string litelib::impl::entry_type::pn() const
{
  string::size_type i = linebuf.find_first_of(" \t\r\n", linestart);

  return linebuf.substr(linestart, 
     i == string::npos ? string::npos : i-linestart);
}

bool litelib::impl::entry_type::readentry( library_base &lb )
{
  istream &ifs = dynamic_cast<litelib::impl&>(lb).f;
  int req_bells = dynamic_cast<litelib::impl&>(lb).b;
  
  while (ifs) {
    getline( ifs, linebuf );

    if (ifs) {
      linestart = linebuf.find_first_not_of(" \t");
      if (linestart != string::npos) {
        int line_bells = read_bells();
        if (req_bells == 0 && line_bells) {
          b = line_bells;
          break;
        }
        else if (req_bells && (line_bells == req_bells || line_bells == 0))
          break;
      }
    }
  }

  return bool(ifs);
}

int litelib::impl::entry_type::read_bells()
{
  string b_pn = pn();
  string::size_type i = b_pn.find(":");
  if (i != string::npos) {
    linestart += i + 1;
    return lexical_cast<int>(b_pn.substr(0, i));
  }
  return 0;
}

library_base::const_iterator litelib::impl::begin() const
{
  if (f_owner) {
    f_owner->clear();
    f_owner->seekg(0, ios::beg);
  }
  return const_iterator(const_cast< litelib::impl * >(this),
                        new entry_type(b, flags));
}

bool litelib::impl::entry_type::has_facet( const library_facet_id& id ) const
{
  if ( id == litelib::payload::id )
    return true;

  else
    return false;
}

string litelib::impl::entry_type::get_payload() const
{
  string::size_type i = linebuf.find_first_of(" \t\r\n", linestart);
  i = linebuf.find_first_not_of(" \t\r\n", i);
  string::size_type j = linebuf.find_last_not_of(" \t\r\n");

  string val;  
  if (i < linebuf.length()) val = linebuf.substr(i, j-i+1);
  return val;
}

shared_pointer< library_facet_base >
litelib::impl::entry_type::get_facet( const library_facet_id& id ) const
{
  shared_pointer< library_facet_base > result;

  if ( id == litelib::payload::id ) 
    result.reset( new litelib::payload( get_payload() ) );

  return result;
}

RINGING_END_NAMESPACE
