// -*- C++ -*- xmllib.cpp - Access to the online XML method library
// Copyright (C) 2003, 2004 Richard Smith <richard@ex-parrot.com>.

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

#include <ringing/xmllib.h>
#include <ringing/pointers.h>
#include <ringing/library.h>
#include <ringing/xmllib.h>

// Important note:  DO NOT include <ringing/streamutils.h> from here.  That
// file includes GPL'd content and this file is only LGPL'd.

#if RINGING_USE_XERCES
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLURL.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/URLInputSource.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMCharacterData.hpp>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

#if RINGING_USE_XERCES

XERCES_CPP_NAMESPACE_USE

RINGING_START_ANON_NAMESPACE

class transcode
{
public:
  transcode( const XMLCh* src )
    : data( XMLString::transcode(src) )
  {}

 ~transcode() { XMLString::release(&data); }

  string to_string() const { return string(data); }

private:
  transcode( const transcode& ); // Unimplemented 
  transcode& operator=( const transcode& ); // Unimplemented 

  char* data;
};

bool init_xerces()
{
  static bool done = false;
  if ( !done )
    XMLPlatformUtils::Initialize();
  done = true;
  return done;
}

DOMElement* next_sibling_element( DOMNode* start, const string& name )
{
  while ( start 
	  && ( start->getNodeType() != DOMNode::ELEMENT_NODE ||
	       transcode( static_cast<DOMElement*>(start)->getTagName() )
	       .to_string() != name ) ) 
    start = start->getNextSibling();

  return static_cast<DOMElement*>(start);
}  


RINGING_END_ANON_NAMESPACE


class xmllib::impl : public library_base
{
public:
  impl( xmllib::file_arg_type type, const string& url );
 ~impl() {}

private:
  // Iterators into the library
  class entry_type;
  friend class entry_type;
  virtual library_base::const_iterator begin() const;

  // Library interface
  virtual bool good() const  { return true; } 

  // Data members
  bool force_init;
  XercesDOMParser parser;
  shared_pointer<DOMDocument> doc;
};

xmllib::xmllib( xmllib::file_arg_type type, const string& url )
  : library( new impl(type, url) )
{
}

xmllib::impl::impl( xmllib::file_arg_type type, const string& url )
  : force_init( init_xerces() )
{
  try
    {
      switch (type) 
	{
	case xmllib::filename:
	  parser.parse( url.c_str() );
	  break;
	  
	case xmllib::url:
	  parser.parse( URLInputSource( XMLURL( url.c_str() ) ) );
	  break;

	case xmllib::default_url:
	  {
	    string full_url( "http://methods.ringing.org/cgi-bin/simple.pl?" );
	    full_url.append( url );
	    parser.parse( URLInputSource( XMLURL( full_url.c_str() ) ) );
	  }
	  break;
	}
      
      doc.reset( parser.adoptDocument() );

      if ( !doc->getDocumentElement() )
	throw runtime_error( "No document element" );

      if ( transcode( doc->getDocumentElement()->getTagName() )
	   .to_string() != "methods" ) 
	throw runtime_error
	  ( "Document root should be a <methods/> element" );
    } 
  catch ( const XMLException& e ) 
    {
      string err( "An error occured parsing the XML: " );
      err.append( transcode( e.getMessage() ).to_string() );

      throw runtime_error( err );
    }
}

library_base *xmllib::canread( const string& name )
{
  try {
    return new xmllib::impl(xmllib::filename, name);
  } catch (...) {
    return NULL;
  }
}

class xmllib::impl::entry_type : public library_entry::impl
{
  string get_field( const string& ) const;
  string extract_text( const DOMElement* e ) const;

  virtual string name() const;
  virtual string base_name() const;
  virtual string pn() const;
  virtual int bells() const;
  
  friend class xmllib::impl;
  explicit entry_type( const shared_pointer<DOMDocument>& doc );
  virtual bool readentry( library_base &lb );
  virtual library_entry::impl *clone() const;

  shared_pointer<DOMDocument> doc;  // To deal with persistence
  DOMElement* meth;
};

string xmllib::impl::entry_type::get_field( const string& name ) const
{
  DOMElement *e = next_sibling_element( meth->getFirstChild(), name );
  if (!e) {
    string err( "XML <method> element has no <" );
    err.append( name );
    err.append( "> sub-element" );
    throw runtime_error( err );
  }
  return extract_text( e );
}

string xmllib::impl::entry_type::extract_text( const DOMElement* e ) const
{
  string value;

  for ( DOMNode *t = e->getFirstChild(); t; t = t->getNextSibling() )
    if ( t->getNodeType() == DOMNode::TEXT_NODE )
      {
	DOMCharacterData *cd = static_cast<DOMCharacterData*>(t);
	if (cd)
	  value.append( transcode( cd->getData() ).to_string() );
      }

  return value;
}

string xmllib::impl::entry_type::name() const
{
  return get_field("title"); 
}

string xmllib::impl::entry_type::base_name() const
{
  return get_field("name"); 
}

string xmllib::impl::entry_type::pn() const
{
  DOMElement *pn_elt = next_sibling_element( meth->getFirstChild(), "pn" );

  if ( !pn_elt ) 
    throw runtime_error
      ( "XML method element has no <pn> element" );

  if ( DOMElement *block_elt 
       = next_sibling_element( pn_elt->getFirstChild(), "block" ) )
    {
      return extract_text( block_elt );
    }
  else if ( DOMElement *sym_elt
	= next_sibling_element( pn_elt->getFirstChild(), "symblock" ) )
    {
      string value( 1u, '&' );
      value.append( extract_text( sym_elt ) );
      value.append( 1u, ',');
      
      sym_elt = next_sibling_element( pn_elt->getFirstChild(), "symblock" );

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

int xmllib::impl::entry_type::bells() const 
{
  return atoi( get_field("stage").c_str() );
}


library_entry::impl *xmllib::impl::entry_type::clone() const
{
  return new entry_type(*this);
}

bool xmllib::impl::entry_type::readentry( library_base& lb )
{
  meth = next_sibling_element
	   ( meth ? meth->getNextSibling()
		  : doc->getDocumentElement()->getFirstChild(),
	     "method" );

  return !!meth;
}

xmllib::impl::entry_type::entry_type( const shared_pointer<DOMDocument>& doc )
  : doc(doc), meth(NULL)
{
}

library_base::const_iterator xmllib::impl::begin() const
{
  return const_iterator( const_cast<xmllib::impl*>(this), 
			 new entry_type(doc) );
}

#else // Stub code if were not using xerces

class xmllib::impl {};

xmllib::xmllib( xmllib::file_arg_type, const string& )
{
  throw runtime_error( "XML libraries not supported in this build" );
}

library_base *xmllib::canread( const string& name )
{
  return NULL;
}

#endif // RINGING_USE_XERCES

RINGING_END_NAMESPACE

