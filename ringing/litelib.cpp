// -*- C++ -*- litelib.cpp - Lightweight library format
// Copyright (C) 2007, 2009 Richard Smith <richard@ex-parrot.com>.

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

// $Id$

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
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_DEFINE_LIBRARY_FACET( litelib::payload );

class litelib::impl : public library_base {
public:
  impl(int b, const string& filename)
    : b(b), f_owner( new ifstream(filename.c_str()) ), f(*f_owner)
  { ok = f.good(); skip_bom(); }

  impl(int b, istream& is)
    : b(b), f(is)
  { ok = f.good(); skip_bom(); }

  // Is this file in the right format?
  // static library_base *canread(const string& filename);

  // Iterators into the library
  class entry_type;
  friend class entry_type;
  virtual const_iterator begin() const;

private:
  void skip_bom();

  virtual bool good(void) const { return f; }

private:
  int b;
  scoped_pointer<ifstream> f_owner;
  istream& f;
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

//void litelib::registerlib(void)
//{
//  library::addtype(&impl::canread);
//}
  
litelib::litelib(int b, const string& filename)
  : library( new impl(b, filename) ) {}

litelib::litelib(int b, istream& is)
  : library( new impl(b, is) ) {}

class litelib::impl::entry_type : public library_entry::impl
{
public:
  explicit entry_type(int b) : b(b) {}

private:
  virtual string name() const { return string(); }
  virtual string base_name() const { return string(); }
  virtual string pn() const;
  virtual int bells() const { return b; }

  virtual bool readentry( library_base &lb );

  virtual library_entry::impl *clone() const { return new entry_type(*this); }

  virtual bool has_facet( const library_facet_id& id ) const;

  virtual shared_pointer< library_facet_base >
    get_facet( const library_facet_id& id ) const;


  // The current line
  string linebuf;

  string::size_type linestart;

  int b; // bells
};

string litelib::impl::entry_type::pn() const
{
  string::size_type i = linebuf.find_first_of(" \t\r\n", linestart);

  return linebuf.substr(linestart, 
     i == string::npos ? string::npos : i-linestart);
}

bool litelib::impl::entry_type::readentry( library_base &lb )
{
  istream &ifs = dynamic_cast<litelib::impl&>(lb).f;
  
  while (ifs)
    {
      getline( ifs, linebuf );

      if (ifs) 
      {
        linestart = linebuf.find_first_not_of(" \t");
        if (linestart != string::npos) break;
      }
    }

  return ifs;
}

library_base::const_iterator litelib::impl::begin() const
{
  if (f_owner) {
    f_owner->clear();
    f_owner->seekg(0, ios::beg);
  }
  return const_iterator(const_cast< litelib::impl * >(this),
                        new entry_type(b));
}

bool litelib::impl::entry_type::has_facet( const library_facet_id& id ) const
{
  if ( id == litelib::payload::id )
    return true;

  else
    return false;
}

shared_pointer< library_facet_base >
litelib::impl::entry_type::get_facet( const library_facet_id& id ) const
{
  shared_pointer< library_facet_base > result;

  if ( id == litelib::payload::id ) 
  {
    string::size_type i = linebuf.find_first_of(" \t\r\n", linestart);
    i = linebuf.find_first_not_of(" \t\r\n", i);

    result.reset( new litelib::payload( linebuf.substr(i) ) );
  }

  return result;
}

RINGING_END_NAMESPACE
