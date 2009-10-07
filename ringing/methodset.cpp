// methodset.cpp - A set of methods with library and libout interfaces
// Copyright (C) 2009 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/methodset.h>
#if RINGING_OLD_INCLUDES
#include <set.h>
#else
#include <set>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class methodset::impl : public library_base, public libout::interface
{
public:
  class entry : public method {
  public:
    explicit entry( method const& m ) : method(m) {}
    explicit entry( library_entry const& src ) : method(src.meth()) {}
    // XXX: Facets
    // Inherit comparison operators from method, so will work fine in a set
  };
 
  class entry_ref : public library_entry::impl
  {
  public:
    entry_ref() : valid(false) {}
    explicit entry_ref( set<entry>::const_iterator i ) : valid(true), i(i) {}

    virtual library_entry::impl *clone() const { 
      return new entry_ref(*this); 
    }
  
    virtual string name() const { return i->name(); }
    virtual string base_name() const { return i->name(); }
    virtual string pn() const 
      { return i->format( method::M_SYMMETRY | method::M_DASH ); }
    virtual int bells() const { return i->bells(); }
    virtual method meth() const { return *i; }
    virtual bool readentry( library_base &lb );

  private:
    bool valid;
    set<entry>::const_iterator i;

    friend class methodset::impl;
  };

  virtual bool good() const { return true; }
  virtual void flush() {}
  virtual library_entry find( method const& pn ) const;
  virtual library_base::const_iterator begin() const;
  virtual void append( library_entry const& e );
  
  void clear() { data.clear(); }

private:
  // This needs to be something like a list or set that doesn't 
  // invalidate iterators on mutation.
  set<entry> data;
};

void methodset::impl::append( library_entry const& e ) 
{
  entry new_entry(e);  
  data.insert(new_entry);
}

bool methodset::impl::entry_ref::readentry( library_base &lb ) 
{
  set<entry>& data = dynamic_cast<methodset::impl&>(lb).data;
  if (!valid) {
    i = data.begin(); 
    valid = true;
  } 
  else ++i;

  if (i == data.end()) {
    valid = false;
    return false;
  }
  return true;
}

library_base::const_iterator methodset::impl::begin() const
{
  return library_base::const_iterator( const_cast<methodset::impl*>(this),
                                       new entry_ref );
}

methodset::methodset() 
{ 
  this->set_impl( new impl ); 
}

library_entry methodset::impl::find( method const& pn ) const
{
  set<entry>::iterator i = data.find( entry(pn) );
  if ( i == data.end() )
    return library_entry();
  else
    return library_entry( new entry_ref(i) );
}

void methodset::clear() 
{
  this->libout::get_impl<impl>()->clear();
}

RINGING_END_NAMESPACE

