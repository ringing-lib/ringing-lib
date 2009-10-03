// -*- C++ -*- dom_gdome.cpp - DOM wrapper for Gdome
// Copyright (C) 2008, 2009 Richard Smith <richard@ex-parrot.com>.

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

#include <ringing/dom.h>

#include <gdome.h>

// Needed to hook into error handling in the underlying library
#include <libxml/xmlerror.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

// TODO: There's significant code duplication between here and xmllib.cpp

class transcode_output
{
public:
  transcode_output( const char* src )
    : data( gdome_str_mkref(src) )
  {}

 ~transcode_output() { gdome_str_unref(data); }

  GdomeDOMString* to_string() const { return data; }

private:
  transcode_output( const transcode_output& ); // Unimplemented 
  transcode_output& operator=( const transcode_output& ); // Unimplemented 

  GdomeDOMString* data;
};

#define TRANSCODE_O(str) transcode_output(str).to_string()

class transcode_input
{
public:
  transcode_input( const GdomeDOMString* src )
    : data(src)
  {}

 ~transcode_input() 
  { if (data) gdome_str_unref( const_cast<GdomeDOMString*>(data) ); }

  char const* to_string() const { return data ? data->str : NULL; }

private:
  transcode_input( const transcode_input& ); // Unimplemented 
  transcode_input& operator=( const transcode_input& ); // Unimplemented 

  GdomeDOMString const* data;
};

#define TRANSCODE_I(str) transcode_input(str).to_string()

struct scoped_libxml2_err
{
public:
  scoped_libxml2_err() : errs(false) {
    xmlSetStructuredErrorFunc(&errs, flag_xml_error);
  }

  void test() {
    if (errs) throw runtime_error("Unable to load XML document");
  }

 ~scoped_libxml2_err() { 
    xmlSetStructuredErrorFunc(NULL, NULL);
  }

private:
  scoped_libxml2_err( const scoped_libxml2_err& ); // Unimplemented 
  scoped_libxml2_err& operator=( const scoped_libxml2_err& ); // Unimplemented 
 
  // Function prototype matches libxml's xmlStructuredErrorFunc
  static void flag_xml_error( void* userData, xmlErrorPtr error )
  {
    *(bool*)userData = true;
  }

  bool errs;
};

RINGING_END_ANON_NAMESPACE

struct dom_document::impl 
{
  impl() : domimpl(NULL), doc(NULL), ofilename(NULL) {}
 ~impl() {
    if (domimpl) gdome_di_unref(domimpl, &exc), domimpl=NULL;
    if (doc) gdome_doc_unref(doc, &exc), doc=NULL;
    if (ofilename) gdome_str_unref(ofilename), ofilename=NULL;
  }

  GdomeException exc;
  GdomeDOMImplementation* domimpl;
  GdomeDocument* doc;
  GdomeDOMString* ofilename;
};

struct dom_element::impl 
{
  impl( GdomeDocument* doc, GdomeElement* elt ) 
    : doc(doc), elt(elt) { gdome_doc_ref(doc, &exc); }
 ~impl() { 
    gdome_doc_unref(doc, &exc); 
    gdome_el_unref(elt, &exc); 
  }

  GdomeException exc;
  GdomeDocument* doc;
  GdomeElement* elt;
};


dom_document::dom_document( string const& filename, io mode, filetype type )
  : pimpl( new impl )
{
  pimpl->domimpl = gdome_di_mkref();

  if (mode == out) {
    pimpl->ofilename = gdome_str_mkref(filename.c_str());
  }
  else if (mode == in) {
    // We want errors by exceptions because otherwise they're 
    // reported on stdout
    scoped_libxml2_err do_throw_error;
    pimpl->doc = gdome_di_createDocFromURI
      ( pimpl->domimpl, filename.c_str(), GDOME_LOAD_PARSING, &pimpl->exc );
    do_throw_error.test();
  }
}

void dom_document::finalise()
{
  if ( pimpl->ofilename && pimpl->doc && pimpl->domimpl ) {
    gdome_di_saveDocToFile( pimpl->domimpl, pimpl->doc, 
                            TRANSCODE_I( pimpl->ofilename ), // sic
                            GDOME_SAVE_LIBXML_INDENT, &pimpl->exc );

    // Prevent multiple finalisation
    gdome_str_unref( pimpl->ofilename );
    pimpl->ofilename = NULL;
  }
}

dom_element::dom_element( impl* i )
  : pimpl(i)
{
}

dom_element dom_document::get_document() const
{
  return new dom_element::impl
    ( pimpl->doc, gdome_doc_documentElement( pimpl->doc, &pimpl->exc ) );
}

dom_element dom_document::create_document( char const* ns, char const* qn )
{
  pimpl->doc = gdome_di_createDocument
    ( pimpl->domimpl, TRANSCODE_O(ns), TRANSCODE_O(qn), NULL, &pimpl->exc );

  return get_document();
}

string dom_element::get_name() const
{
  return TRANSCODE_I( gdome_el_tagName( pimpl->elt, &pimpl->exc ) );
}

string dom_element::get_content() const
{
  string value;

  GdomeNode* t 
    = gdome_n_firstChild( GDOME_N(pimpl->elt), &pimpl->exc );

  while (t) {
    if ( gdome_n_nodeType(t, &pimpl->exc) == GDOME_TEXT_NODE )
      value += TRANSCODE_I( gdome_cd_data( GDOME_CD(t), &pimpl->exc ) );

    GdomeNode* next = gdome_n_nextSibling( t, &pimpl->exc );
    gdome_n_unref(t, &pimpl->exc); 
    t = next;
  }

  if (t) 
    gdome_n_unref(t, &pimpl->exc);
  return value;
}

dom_element dom_element::get_first_child() const
{
  GdomeNode* t = gdome_n_firstChild( GDOME_N(pimpl->elt), &pimpl->exc );

  while (t && gdome_n_nodeType(t, &pimpl->exc) != GDOME_ELEMENT_NODE ) {
    GdomeNode* next = gdome_n_nextSibling( t, &pimpl->exc );
    gdome_n_unref(t, &pimpl->exc);
    t = next;
  }

  if (t)
    return new impl( pimpl->doc, GDOME_EL(t) );
  else
    return dom_element();
}

dom_element dom_element::get_next_sibling() const
{
  GdomeNode* t = gdome_n_nextSibling( GDOME_N(pimpl->elt), &pimpl->exc );

  while (t && gdome_n_nodeType(t, &pimpl->exc) != GDOME_ELEMENT_NODE ) {
    GdomeNode* next = gdome_n_nextSibling( t, &pimpl->exc );
    gdome_n_unref(t, &pimpl->exc);
    t = next;
  }

  if (t)
    return new impl( pimpl->doc, GDOME_EL(t) );
  else
    return dom_element();
}


dom_element dom_element::add_elt( char const* ns, char const* name )
{
  GdomeElement* child 
    = gdome_doc_createElementNS( pimpl->doc, TRANSCODE_O(ns),
                                 TRANSCODE_O(name), &pimpl->exc );

  GdomeNode* child2 
    = gdome_n_appendChild( GDOME_N(pimpl->elt), GDOME_N(child), &pimpl->exc );

  gdome_n_unref(child2, &pimpl->exc);
  return new impl( pimpl->doc, child );
}

void dom_element::add_content( string const& content )
{
  // We only add the text if it is not empty.  This is because
  // the DOMWriter will output <foo/> for an element with no DOMText
  // children, but <foo></foo> for an element with an empty DOMText
  // child.  I prefer the former.
  if ( content.size() ) {
    GdomeText* txt
      = gdome_doc_createTextNode( pimpl->doc, TRANSCODE_O( content.c_str() ),
                                  &pimpl->exc );
    
    GdomeNode* txt2 
      = gdome_n_appendChild( GDOME_N(pimpl->elt), GDOME_N(txt), &pimpl->exc );
    
    gdome_n_unref(txt2, &pimpl->exc);
    gdome_t_unref(txt, &pimpl->exc);
  }
}

void dom_element::add_attr( char const* ns, char const* name, char const* val )
{
  if (ns)
    gdome_el_setAttributeNS
      ( pimpl->elt, TRANSCODE_O(ns), TRANSCODE_O(name), TRANSCODE_O(val), 
        &pimpl->exc );
  else
    gdome_el_setAttribute
      ( pimpl->elt, TRANSCODE_O(name), TRANSCODE_O(val), &pimpl->exc );
}

RINGING_END_NAMESPACE

