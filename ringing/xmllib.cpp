// -*- C++ -*- xmllib.cpp - Access to the online XML method library
// Copyright (C) 2003, 2004, 2006, 2008, 2009, 2018
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

// $Id$

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_C_INCLUDES
#include <stdio.h>
#else
#include <cstdio>
#endif
#include <ringing/xmllib.h>
#include <ringing/pointers.h>
#include <ringing/library.h>
#include <ringing/peal.h>
#include <ringing/dom.h>

// Important note:  DO NOT include <ringing/streamutils.h> from here.  That
// file includes GPL'd content and this file is only LGPL'd.

#ifdef _MSC_VER
// Microsoft have unilaterally deprecated sscanf in favour of a non-standard
// extension, sscanf_s.  4996 is the warning about it being deprecated.
#pragma warning (disable: 4996)
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class xmllib::impl : public library_base {
public:
  impl( xmllib::file_arg_type type, const string& url );

private:
  // Iterators into the library
  class entry_type;
  friend class entry_type;
  virtual library_base::const_iterator begin() const;

  // Library interface
  virtual bool good() const  { return true; } 

  // Data members
  shared_pointer<dom_document> doc;
  bool cc_xml;
};

xmllib::impl::impl( xmllib::file_arg_type type, const string& url )
{
  dom_document::filetype ftype;
  string filename;

  switch (type) {
    case xmllib::filename:
      ftype = dom_document::file; filename = url;
      break;
      
    case xmllib::default_url:
      filename = "http://methods.ringing.org/cgi-bin/simple.pl?format=old&";
  
    case xmllib::url:
      ftype = dom_document::url; filename += url;
      break;

    default:
      abort();
  }
      
  doc.reset( new dom_document( filename, dom_document::in, ftype ) );

  if ( doc->get_document().get_name() == "methods" ) 
    cc_xml = false;
  else if ( doc->get_document().get_name() == "collection" )
    cc_xml = true;
  else
    throw runtime_error
      ( "Document root should be a <methods/> or <collection/> element" );
}

class xmllib::impl::entry_type : public library_entry::impl
{
  dom_element 
    next_sibling_element( dom_element start, const string& name) const;
  string get_field( dom_element const& parent, const string& elt_name ) const;
  string extract_text( const dom_element& e ) const;
  peal get_peal( const string& elt_name ) const;
  string facet_elt( const library_facet_id& id ) const;

  virtual string name() const;
  virtual string base_name() const;
  virtual string pn() const;
  virtual int bells() const;
  
  virtual bool has_facet( const library_facet_id& id ) const;

  virtual shared_pointer< library_facet_base >
    get_facet( const library_facet_id& id ) const;

  friend class xmllib::impl;
  explicit entry_type( const shared_pointer<dom_document>& doc, bool cc_xml );
  virtual bool readentry( library_base &lb );
  virtual library_entry::impl *clone() const;

  bool cc_xml;
  shared_pointer<dom_document> doc;  // To deal with persistence
  dom_element methset, meth;
};

dom_element xmllib::impl::entry_type
  ::next_sibling_element( dom_element start, const string& name ) const
{
  while ( start && start.get_name() != name )
    start = start.get_next_sibling();
  return start;
}

string xmllib::impl::entry_type::get_field( dom_element const& parent,
					    const string& name) const
{
  if (!parent) return string();
  dom_element e = next_sibling_element( parent.get_first_child(), name );
  if (e)  
    return extract_text( e );
  else
    return string();
}

peal xmllib::impl::entry_type::get_peal( const string& name ) const
{
  dom_element e = next_sibling_element( meth.get_first_child(), name );
  if (!e) return peal();

  peal::date dt;
  if ( sscanf( get_field( e, "date" ).c_str(), "%d-%d-%d", 
	       &dt.year, &dt.month, &dt.day ) != 3 )
    dt.day = dt.month = dt.year = 0;

  string loc;

  dom_element l = next_sibling_element( meth.get_first_child(), "location" );
  if ( l ) {
    // TODO: The peal object should know about the different 
    // parts of a location
    string tmp;

    // CC XML only
    tmp=get_field( l, "room" );
    if (tmp.size() && loc.size()) loc += ", ";
    if (tmp.size()) loc += tmp;

    // CC XML only
    tmp=get_field( l, "building" );
    if (tmp.size() && loc.size()) loc += ", ";
    if (tmp.size()) loc += tmp;

    // Our XML only
    tmp=get_field( l, "dedication" );
    if (tmp.size() && loc.size()) loc += ", ";
    if (tmp.size()) loc += tmp;

    // Our XML only
    tmp=get_field( l, "place" );
    if (tmp.size() && loc.size()) loc += ", ";
    if (tmp.size()) loc += tmp;

    // CC XML only
    tmp=get_field( l, "town" );
    if (tmp.size() && loc.size()) loc += ", ";
    if (tmp.size()) loc += tmp;

    // Both formats
    tmp=get_field( l, "county" );
    if (tmp.size() && loc.size()) loc += ", ";
    if (tmp.size()) loc += tmp;

    // CC XML only
    tmp=get_field( l, "region" );
    if (tmp.size() && loc.size()) loc += ", ";
    if (tmp.size()) loc += tmp;

    // Both formats
    tmp=get_field( l, "country" );
    if (tmp.size() && loc.size()) loc += ", ";
    if (tmp.size()) loc += tmp;
  }

  return peal( dt, loc );
}

string xmllib::impl::entry_type::extract_text( const dom_element& e ) const
{
  return e.get_content();
}

string xmllib::impl::entry_type::name() const
{
  return get_field( meth, "title" );
}

string xmllib::impl::entry_type::base_name() const
{
  return get_field( meth, "name" );
}

string xmllib::impl::entry_type::pn() const
{
  if ( dom_element pn_elt
         = next_sibling_element( meth.get_first_child(), "pn" ) ) {

    if ( dom_element block_elt 
         = next_sibling_element( pn_elt.get_first_child(), "block" ) )
      return extract_text( block_elt );
    
    else if ( dom_element sym_elt
              = next_sibling_element( pn_elt.get_first_child(), "symblock" ) ) {
      string value( 1u, '&' );
      value.append( extract_text( sym_elt ) );
      value.append( 1u, ',');
        
      sym_elt = next_sibling_element( sym_elt.get_next_sibling(), "symblock" );
  
      if ( !sym_elt )
        throw runtime_error
          ( "XML <pn> element has only one <symblock> element" );
  
      value.append( 1u, '&' );
      value.append( extract_text( sym_elt ) );
  
      return value;
    }
    else 
      throw runtime_error
        ( "XML <pn> element has unrecognised content" );
  }
  else if ( dom_element notn_elt
            = next_sibling_element( meth.get_first_child(), "notation" ) ) {

    dom_element sym_elt = next_sibling_element( meth.get_first_child(), 
                                                "symmetry" );

    if ( !sym_elt || sym_elt.get_content().find("palindromic") == string::npos )
      return extract_text( notn_elt );

    else {
      string notn( extract_text( notn_elt ) );
      size_t i = notn.find(',');

      if ( i == string::npos || i == 0 || i == notn.size()-1)
        return notn;
     
      string value( 1u, '&' );
      value.append( notn, 0, i+1 );
      value.append( 1u, '&' );
      value.append( notn, i+1, string::npos );
      return value;
    }
  }
  else
    throw runtime_error
      ( "XML method element has no <pn> or <notation> element" );

}

int xmllib::impl::entry_type::bells() const 
{
  string stage( get_field(meth, "stage") );
  if (stage.empty() && methset)
    stage = get_field( next_sibling_element( methset.get_first_child(), 
                                             "properties" ), "stage" );
  
  int b = atoi( stage.c_str() );
  if ( b == 0 )
    throw runtime_error
      ( "Invalid or missing stage specified" );
  return b;
}


library_entry::impl *xmllib::impl::entry_type::clone() const
{
  return new entry_type(*this);
}

bool xmllib::impl::entry_type::readentry( library_base& lb )
{
  if (meth)
    meth = meth.get_next_sibling();
  else if (cc_xml) {
    methset = next_sibling_element( doc->get_document().get_first_child(),
                                    "methodSet" );
    meth = methset.get_first_child();
  }
  else
    meth = doc->get_document().get_first_child();

  while (true) {
    meth = next_sibling_element( meth, "method" );
    if (meth) return true;

    if (methset)
      methset = next_sibling_element( methset.get_next_sibling(), "methodSet" );
    
    if (!methset) return false;
    meth = methset.get_first_child();
  }
}

string xmllib::impl::entry_type::facet_elt( const library_facet_id& id ) const
{
  if ( id == first_tower_peal::id )
    return cc_xml ? "firstTowerbellPeal" : "first-tower";

  else if ( id == first_hand_peal::id ) 
    return cc_xml ? "firstHandbellPeal" : "first-hand";

  else return string();
}

bool xmllib::impl::entry_type::has_facet( const library_facet_id& id ) const
{
  string elt( facet_elt(id) );
  if (elt.size())
    return next_sibling_element( meth.get_first_child(), elt );
  else 
    return false;
}

shared_pointer< library_facet_base >
xmllib::impl::entry_type::get_facet( const library_facet_id& id ) const
{
  shared_pointer< library_facet_base > result;
  string elt( facet_elt(id) );

  if (elt.size()) {
    if ( next_sibling_element( meth.get_first_child(), elt ) ) 
      result.reset( new first_tower_peal( get_peal(elt) ) );
  }

  return result;
}


xmllib::impl::entry_type::entry_type( const shared_pointer<dom_document>& doc,
                                      bool cc_xml )
  : doc(doc), cc_xml(cc_xml)
{
}

library_base::const_iterator xmllib::impl::begin() const
{
  return const_iterator( const_cast<xmllib::impl*>(this), 
			 new entry_type(doc, cc_xml) );
}

xmllib::xmllib( xmllib::file_arg_type type, const string& url )
  : library( new impl(type, url) )
{
}

library_base *xmllib::canread( const string& name )
{
  try {
    return new xmllib::impl(xmllib::filename, name);
  } catch (...) {
    return NULL;
  }
}

RINGING_END_NAMESPACE

