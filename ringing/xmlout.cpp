// -*- C++ -*- xmlout.cpp - Output of xml libraries
// Copyright (C) 2004 Richard Smith <richard@ex-parrot.com>.

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

#include <ringing/xmlout.h>
#include <ringing/library.h>
#include <ringing/peal.h>

#if RINGING_USE_XERCES
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

#if RINGING_USE_XERCES

XERCES_CPP_NAMESPACE_USE

#define METHODS_XMLNS "http://methods.ringing.org/NS/method"
#define XSI_XMLNS "http://www.w3.org/2001/XMLSchema-instance"

RINGING_START_ANON_NAMESPACE

// TODO: There's significant code duplication between here and xmllib.cpp

class transcode
{
public:
  transcode( const char* src )
    : data( XMLString::transcode(src) )
  {}

 ~transcode() { XMLString::release(&data); }

  XMLCh* to_string() const { return data; }

private:
  transcode( const transcode& ); // Unimplemented 
  transcode& operator=( const transcode& ); // Unimplemented 

  XMLCh* data;
};

#define TRANSCODE(str) transcode(str).to_string()

struct init_xerces_t {
  init_xerces_t() { XMLPlatformUtils::Initialize(); }
 ~init_xerces_t() { XMLPlatformUtils::Terminate(); }
};

bool init_xerces() {
  static struct init_xerces_t tmp;
  return true;
}

RINGING_END_ANON_NAMESPACE


class xmlout::impl : public libout::interface {
public:
  impl( const string& filename );
 ~impl();
  
  virtual void append( library_entry const& entry );
  virtual void flush();

private:
  // XML helper functions
  DOMElement* add_elt( DOMElement* parent, char const* name );
  void add_simple_elt( DOMElement* parent, char const* name, 
                       string const& content );

  void add_peal( DOMElement* peal_elt, peal const& p );

  bool force_init;
  DOMDocument* doc;
  scoped_pointer< DOMWriter > writer;
  scoped_pointer< XMLFormatTarget > target;
};



DOMElement* xmlout::impl::add_elt( DOMElement* parent, char const* name )
{
  DOMElement* elt
    = doc->createElementNS( TRANSCODE( METHODS_XMLNS ), TRANSCODE( name ) );
  parent->appendChild( elt );
  return elt;
}

void xmlout::impl::add_simple_elt( DOMElement* parent, char const* name, 
				   string const& content )
{
  DOMElement* elt = add_elt( parent, name );

  // We only add the text if it is not empty.  This is because
  // the DOMWriter will output <foo/> for an element with no DOMText
  // children, but <foo></foo> for an element with an empty DOMText
  // child.  I prefer the former.
  if ( content.size() ) {
    DOMText* txt = doc->createTextNode( TRANSCODE( content.c_str() ) );
    elt->appendChild( txt );
  }
}


xmlout::impl::impl( const string& filename )
  : force_init( init_xerces() ),
    doc(NULL)
{
  // "LS" means we require the "Load and Save" feature
  DOMImplementation* i
    = DOMImplementationRegistry::getDOMImplementation( TRANSCODE("LS") );

  if ( i ) {
    doc = i->createDocument( TRANSCODE( METHODS_XMLNS ), 
			     TRANSCODE("methods"), NULL );

    writer.reset( i->createDOMWriter() );

    // Turn pretty-printing on.  (TODO: Make this optional.)
    if ( writer->canSetFeature( XMLUni::fgDOMWRTFormatPrettyPrint, true ) )
      writer->setFeature( XMLUni::fgDOMWRTFormatPrettyPrint, true );
  }

  if ( filename.size() > 1 || filename.size() == 1 && filename != "-" ) 
    target.reset( new LocalFileFormatTarget(filename.c_str()) );
  else
    target.reset( new StdOutFormatTarget );

  if ( !doc || !writer || !target )
    throw runtime_error( "Failed to create an XML document" );

}

void xmlout::impl::flush()
{
  if ( writer && target && doc ) {
    writer->writeNode(target.get(), *doc);

    // Safe against multiple flushes
    doc->release(); doc = NULL;
  }
}

xmlout::impl::~impl()
{
  try { 
    flush();
  } catch(...) {}

  if ( doc )
    doc->release();
}

void xmlout::impl::add_peal( DOMElement* peal_elt, peal const& p )
{
  // A date of {0,0,0} is used to mean 'unspecified'.
  if ( p.when().day ) {
    char buffer[32];
    snprintf( buffer, 32, "%4d-%02d-%02d", 
	      p.when().year, p.when().month, p.when().day );
    add_simple_elt( peal_elt, "date", buffer );
  }

  // TODO:  This is contrary to the methods XML schema, but I 
  // don't have the information in a structured format. This 
  // probably reflects a deficiency in the methods XML schema.
  if ( p.where().size() )
    add_simple_elt( peal_elt, "location", p.where() );
}

void xmlout::impl::append( library_entry const& entry ) 
{
  if ( !doc )
    throw runtime_error( "No current document" ); 

  method meth( entry.meth() );

  DOMElement* meth_elt = add_elt( doc->getDocumentElement(), "method" );

  // TODO:  This framework doesn't allow for the distinction between 
  // methods that are known to be unnamed (which should have xsi:nil set)
  // and methods that are not known to be named (which should not).
  // We play it safe and never set xsi:nil for unnamed methods.
  {
    string name( meth.name() );
    if ( name.size() ) {
      add_simple_elt( meth_elt, "name", name );
      add_simple_elt( meth_elt, "title", meth.fullname() );
    }
  }

  { // We don't use make_sting because this file is LGPL'd.
    char buffer[32];
    snprintf( buffer, 32, "%d", meth.bells() );
    add_simple_elt( meth_elt, "stage", buffer );
  }

  add_simple_elt( meth_elt, "classes", method::classname( meth.methclass() ) );

  {
    DOMElement* pn_elt = add_elt( meth_elt, "pn" );

    int const fmt_opts = method::M_LCROSS | method::M_EXTERNAL;

    int sp = meth.symmetry_point();
    if ( sp == -1 ) {
      add_simple_elt( pn_elt, "block", meth.format(fmt_opts) );
    } else {
      method b1, b2; 

      copy( meth.begin(), meth.begin() + sp+1, back_inserter( b1 ) );
      copy( meth.begin() + 2*sp+1, meth.begin() + (meth.length()/2 + sp+1), 
	    back_inserter( b2 ) );

      add_simple_elt( pn_elt, "symblock", b1.format(fmt_opts) );
      add_simple_elt( pn_elt, "symblock", b2.format(fmt_opts) );
    }
  }

  add_simple_elt( meth_elt, "lead-head", meth.lh().print() );

  {
    DOMElement* cl_elt = add_elt( meth_elt, "classification" );
    DOMElement* lhcode_elt = add_elt( cl_elt, "lhcode" );
    string lhcode = meth.lhcode();
    if(lhcode.empty() || lhcode[0] == 'z') {
      lhcode_elt->setAttributeNS( TRANSCODE( XSI_XMLNS ),
				  TRANSCODE( "xsi:nil" ),
				  TRANSCODE( "true" ) );
    } else {
      lhcode_elt->setAttributeNS( TRANSCODE( METHODS_XMLNS ),
				  TRANSCODE( "code" ),
				  TRANSCODE( lhcode.c_str() ));
    }
  }

  if ( entry.has_facet< first_tower_peal >() )
    add_peal( add_elt( meth_elt, "first-tower" ),
	      entry.get_facet< first_tower_peal >() );

  if ( entry.has_facet< first_hand_peal >() )
    add_peal( add_elt( meth_elt, "first-hand" ),
	      entry.get_facet< first_hand_peal >() );
  
  {
    DOMElement* refs_elt = NULL;

    if ( entry.has_facet< rw_ref >() ) {
      if ( !refs_elt ) refs_elt = add_elt( meth_elt, "refs" );
      add_simple_elt( refs_elt, "rw-ref", entry.get_facet< rw_ref >() );
    }
  }
}

xmlout::xmlout( const string& filename )
  : libout( new impl(filename) )
{}


#else // Stub code if were not using xerces

class xmlout::impl : public libout::interface {};

xmlout::xmlout( const string& )
{
  throw runtime_error( "XML libraries not supported in this build" );
}

#endif // RINGING_USE_XERCES



RINGING_END_NAMESPACE
