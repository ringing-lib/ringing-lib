// -*- C++ -*- dom_xerces.cpp - DOM wrapper for Xerces
// Copyright (C) 2003, 2004, 2006, 2008 Richard Smith <richard@ex-parrot.com>.

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
#include <stdexcept.h>
#else
#include <stdexcept>
#endif
#include <string>

#include <ringing/dom.h>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMCharacterData.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLURL.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/URLInputSource.hpp>
#include <xercesc/framework/StdInInputSource.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>


RINGING_START_NAMESPACE

RINGING_USING_STD

XERCES_CPP_NAMESPACE_USE

RINGING_START_ANON_NAMESPACE

// TODO: There's significant code duplication between here and xmllib.cpp

class transcode_output
{
public:
  transcode_output( const char* src )
    : data( XMLString::transcode(src) )
  {}

 ~transcode_output() { XMLString::release(&data); }

  XMLCh* to_string() const { return data; }

private:
  transcode_output( const transcode_output& ); // Unimplemented 
  transcode_output& operator=( const transcode_output& ); // Unimplemented 

  XMLCh* data;
};

#define TRANSCODE_O(str) transcode_output(str).to_string()

class transcode_input
{
public:
  transcode_input( const XMLCh* src )
    : data( XMLString::transcode(src) )
  {}

 ~transcode_input() { XMLString::release(&data); }

  string to_string() const { return string(data); }

private:
  transcode_input( const transcode_input& ); // Unimplemented 
  transcode_input& operator=( const transcode_input& ); // Unimplemented 

  char* data;
};

#define TRANSCODE_I(str) transcode_input(str).to_string()

struct init_xerces_t {
  init_xerces_t() { XMLPlatformUtils::Initialize(); }
 ~init_xerces_t() { XMLPlatformUtils::Terminate(); }
};

bool init_xerces() {
  static struct init_xerces_t tmp;
  return true;
}

RINGING_END_ANON_NAMESPACE


struct dom_document::impl 
{
  impl() : force_init( init_xerces() ), domimpl(NULL), doc(NULL) {}
 ~impl() { if (doc) doc->release(); }

  bool force_init;

  DOMImplementation* domimpl;
  DOMDocument* doc;
  scoped_pointer< DOMWriter > writer;
  scoped_pointer< XercesDOMParser > parser;
  scoped_pointer< XMLFormatTarget > target;
};

struct dom_element::impl 
{
  impl( DOMDocument* doc, DOMElement* elt ) : doc(doc), elt(elt) {}

  DOMDocument* doc;
  DOMElement* elt;
};

dom_document::dom_document( string const& filename, io mode, filetype type )
  : pimpl( new impl )
{
  if ( mode == out ) {
    // "LS" means we require the "Load and Save" feature
    pimpl->domimpl 
      = DOMImplementationRegistry::getDOMImplementation( TRANSCODE_O("LS") );
  
    if ( pimpl->domimpl ) {
      pimpl->writer.reset( pimpl->domimpl->createDOMWriter() );
  
      // Turn pretty-printing on.  (TODO: Make this optional.)
      if ( pimpl->writer->canSetFeature
             ( XMLUni::fgDOMWRTFormatPrettyPrint, true ) )
        pimpl->writer->setFeature( XMLUni::fgDOMWRTFormatPrettyPrint, true );
    }
  
    if ( type == file ) 
      pimpl->target.reset( new LocalFileFormatTarget(filename.c_str()) );
    else if ( type == stdio )
      pimpl->target.reset( new StdOutFormatTarget );
  
    if ( !pimpl->domimpl || !pimpl->writer || !pimpl->target )
      throw runtime_error( "Failed to create an XML document" );
  } 
  else if ( mode == in ) { 
    pimpl->parser.reset( new XercesDOMParser );
    pimpl->parser->setDoNamespaces(true);

    try {
      scoped_pointer<InputSource> src;
      switch (type) {
        case dom_document::stdio:
          src.reset( new StdInInputSource );
          break;
        
	case dom_document::file:
          src.reset
            ( new LocalFileInputSource( TRANSCODE_O( filename.c_str() ) ) );
	  break;
	  
	case dom_document::url:
	  src.reset( new URLInputSource( XMLURL( filename.c_str() ) ) );
          break;
      }

      pimpl->parser->parse( *src );
      pimpl->doc = pimpl->parser->adoptDocument();

      if ( !pimpl->doc || !pimpl->doc->getDocumentElement() )
        throw runtime_error( "No document element" );
    }
    catch ( const XMLException& e ) {
      string err( "An error occured parsing the XML: " );
      err.append( TRANSCODE_I( e.getMessage() ) );
      throw runtime_error( err );
    }
  }
  else throw logic_error( "DOM file must be opened as input or output" );
}

void dom_document::finalise()
{
  if ( pimpl->writer && pimpl->target && pimpl->doc ) {
    pimpl->writer->writeNode(pimpl->target.get(), *pimpl->doc);

    // Safe against multiple flushes
    pimpl->doc->release(); pimpl->doc = NULL;
  }
}

dom_element::dom_element( impl* i )
  : pimpl(i)
{}

dom_element dom_document::get_document() const
{
  return new dom_element::impl( pimpl->doc, pimpl->doc->getDocumentElement() );
}

dom_element dom_document::create_document( char const* ns, char const* qn )
{
  pimpl->doc 
    = pimpl->domimpl->createDocument( TRANSCODE_O(ns), TRANSCODE_O(qn), NULL );

  return get_document();
}

string dom_element::get_name() const 
{
  return TRANSCODE_I( pimpl->elt->getTagName() );
}

string dom_element::get_content() const
{
  string value;

  for ( DOMNode *t = pimpl->elt->getFirstChild(); t; t = t->getNextSibling() )
    if ( t->getNodeType() == DOMNode::TEXT_NODE )
      value += TRANSCODE_I( static_cast<DOMCharacterData*>(t)->getData() );

  return value;
}

dom_element dom_element::get_first_child() const
{
  DOMNode* e = pimpl->elt->getFirstChild();
  while ( e && e->getNodeType() != DOMNode::ELEMENT_NODE )
    e = e->getNextSibling();
  if (e)
    return dom_element( new impl( pimpl->doc, static_cast<DOMElement*>(e) ) );
  else
    return dom_element();
}

dom_element dom_element::get_next_sibling() const
{
  DOMNode* e = pimpl->elt->getNextSibling();
  while ( e && e->getNodeType() != DOMNode::ELEMENT_NODE )
    e = e->getNextSibling();
  if (e)
    return dom_element( new impl( pimpl->doc, static_cast<DOMElement*>(e) ) );
  else
    return dom_element();
}

dom_element dom_element::add_elt( char const* ns, char const* name )
{
  DOMElement* elt = 
    pimpl->doc->createElementNS( TRANSCODE_O( ns ), TRANSCODE_O( name ) );
  pimpl->elt->appendChild(elt);

  return new dom_element::impl( pimpl->doc, elt );
}

void dom_element::add_content( string const& content )
{
  // We only add the text if it is not empty.  This is because
  // the DOMWriter will output <foo/> for an element with no DOMText
  // children, but <foo></foo> for an element with an empty DOMText
  // child.  I prefer the former.
  if ( content.size() ) {
    DOMText* txt = pimpl->doc->createTextNode( TRANSCODE_O( content.c_str() ) );
    pimpl->elt->appendChild( txt );
  }
}

void dom_element::add_attr( char const* ns, char const* name, char const* val )
{
  if (ns)
    pimpl->elt->setAttributeNS
      ( TRANSCODE_O( ns ), TRANSCODE_O( name ), TRANSCODE_O( val ) );
  else
    pimpl->elt->setAttribute( TRANSCODE_O( name ), TRANSCODE_O( val ) );
}

RINGING_END_NAMESPACE

