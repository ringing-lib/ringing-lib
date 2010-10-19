// methodset.cpp - A set of methods with library and libout interfaces
// Copyright (C) 2009, 2010 Richard Smith <richard@ex-parrot.com>

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
#include <map.h>
#include <vector.h>
#else
#include <set>
#include <map>
#include <vector>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class methodset::impl : public library_base, public libout::interface
{
public:
  class entry : public method {
  public:
    explicit entry( method const& m ) : method(m) {}
    explicit entry( library_entry const& src,
                    vector<library_facet_id> const& ids );
    
    // Inherit comparison operators from method, so will work fine in a set

    shared_pointer< library_facet_base > 
      get_facet( const library_facet_id& id ) const;
 
  private:
    void copy_facets( library_entry const& src, 
                      vector<library_facet_id> const& ids );

    typedef map< library_facet_id, shared_pointer< library_facet_base > > 
      facet_map;
   
    facet_map facets;
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
    virtual shared_pointer< library_facet_base > 
      get_facet( const library_facet_id& id ) const { return i->get_facet(id); }

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
  virtual void append( method const& e );
  
  void clear() { data.clear(); }
  size_t size() const { return data.size(); }

  void store_facet( library_facet_id id ) { facet_ids.push_back(id); }

private:
  // This needs to be something like a list or set that doesn't 
  // invalidate iterators on mutation.
  set<entry> data;

  // Facets to copy
  vector<library_facet_id> facet_ids;
};

void methodset::impl::append( library_entry const& e ) 
{
  data.insert( entry(e, facet_ids) );  
}

void methodset::impl::append( method const& m ) 
{
  data.insert( entry(m) );  
}

methodset::impl::entry::entry( library_entry const& src,
                               vector<library_facet_id> const& ids )
  : method( src.meth() )
{
  for ( vector<library_facet_id>::const_iterator i=ids.begin(), e=ids.end();
        i != e; ++i ) {
    shared_pointer<library_facet_base> f( src.get_facet(*i) );
    if (f) facets[*i] = f;
  }
}

shared_pointer< library_facet_base >
methodset::impl::entry::get_facet( const library_facet_id& id ) const
{
  facet_map::const_iterator i = facets.find(id);
  if ( i != facets.end() ) return i->second;
  else return shared_pointer< library_facet_base >();
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

size_t methodset::size() const
{
  return this->libout::get_impl<impl>()->size();
}

void methodset::append( method const& m )
{
  this->libout::get_impl<impl>()->append( m );
}

void methodset::store_facet( library_facet_id id )
{
  this->libout::get_impl<impl>()->store_facet(id);
}

RINGING_END_NAMESPACE

