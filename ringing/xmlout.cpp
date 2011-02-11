// -*- C++ -*- xmlout.cpp - Output of xml libraries
// Copyright (C) 2004, 2008, 2009 Richard Smith <richard@ex-parrot.com>.

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
#include <ringing/xmlout.h>
#include <ringing/library.h>
#include <ringing/peal.h>
#include <ringing/dom.h>
#include <ringing/row.h>

#ifdef _MSC_VER
// Microsoft have unilaterally deprecated snprintf in favour of a non-standard
// extension, snprintf_s.  4996 is the warning about it being deprecated.
#pragma warning (disable: 4996)
#define snprintf _snprintf
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

#define METHODS_XMLNS "http://methods.ringing.org/NS/method"
#define XSI_XMLNS "http://www.w3.org/2001/XMLSchema-instance"

class xmlout::impl : public libout::interface {
public:
  impl( const string& filename );
 ~impl();
  
  virtual void append( library_entry const& entry );
  virtual void flush();

private:
  static dom_document::filetype filetype(string const& filename);

  void add_peal( dom_element peal_elt, peal const& p );

  dom_document doc;
  dom_element docelt;

  static const char *txt_classes[12];
};

dom_document::filetype xmlout::impl::filetype(string const& filename)
{
  if (filename.empty() || filename == "-")
    return dom_document::stdio;
  else
    return dom_document::file;
}


const char *xmlout::impl::txt_classes[12] = {
  "",
  "principle",
  "plain/bob",
  "plain/place",
  "treble-dodging/treble-bob",
  "treble-dodging/surprise",
  "treble-dodging/delight",
  "treble-place",
  "alliance",
  "hybrid",
  "slow-course"
};


xmlout::impl::impl( const string& filename )
  : doc( filename, dom_document::out, filetype(filename) ),
    docelt( doc.create_document( METHODS_XMLNS, "methods" ) )
{ 
}

void xmlout::impl::flush()
{
  doc.finalise();
}

xmlout::impl::~impl()
{
  try { 
    flush();
  } catch(...) {}
}

void xmlout::impl::add_peal( dom_element peal_elt, peal const& p )
{
  // A date of {0,0,0} is used to mean 'unspecified'.
  if ( p.when().day ) {
    char buffer[32];
    snprintf( buffer, 32, "%4d-%02d-%02d", 
	      p.when().year, p.when().month, p.when().day );
    peal_elt.add_elt( METHODS_XMLNS, "date" ).add_content( buffer );
  }

  // TODO:  This is contrary to the methods XML schema, but I 
  // don't have the information in a structured format. This 
  // probably reflects a deficiency in the methods XML schema.
  if ( p.where().size() )
    peal_elt.add_elt( METHODS_XMLNS, "location" ).add_content( p.where() );
}

void xmlout::impl::append( library_entry const& entry ) 
{
  method meth( entry.meth() );

  dom_element meth_elt = docelt.add_elt( METHODS_XMLNS, "method" );

  // TODO:  This framework doesn't allow for the distinction between 
  // methods that are known to be unnamed (which should have xsi:nil set)
  // and methods that are not known to be named (which should not).
  // We play it safe and never set xsi:nil for unnamed methods.
  // Note that not having a name doesn't mean that a method has no name!
  // Remember Little Bob.
  meth_elt.add_elt( METHODS_XMLNS, "name" ).add_content( meth.name() );
  meth_elt.add_elt( METHODS_XMLNS, "title" ).add_content( meth.fullname() );

  { // We don't use make_sting because this file is LGPL'd.
    char buffer[32];
    snprintf( buffer, 32, "%d", meth.bells() );
    meth_elt.add_elt( METHODS_XMLNS, "stage" ).add_content( buffer );
  }

  // Classes doesn't include things like Little and Differential
  int methclass = meth.methclass();
  meth_elt.add_elt( METHODS_XMLNS, "classes") 
    .add_content( method::classname( methclass & method::M_MASK ) );

  { // <pn>
    dom_element pn_elt = meth_elt.add_elt( METHODS_XMLNS, "pn" );

    int const fmt_opts = method::M_LCROSS | method::M_EXTERNAL;

    int sp = meth.length()%2 == 0 ? meth.symmetry_point() : -1;
    if ( sp == -1 ) {
      pn_elt.add_elt( METHODS_XMLNS, "block" )
        .add_content( meth.format(fmt_opts) );
    } else {
      method b1, b2; 

      copy( meth.begin(), meth.begin() + sp+1, back_inserter( b1 ) );
      copy( meth.begin() + 2*sp+1, meth.begin() + (meth.length()/2 + sp+1), 
	    back_inserter( b2 ) );

      pn_elt.add_elt( METHODS_XMLNS, "symblock" )
        .add_content( b1.format(fmt_opts) );
      pn_elt.add_elt( METHODS_XMLNS, "symblock" )
        .add_content( b2.format(fmt_opts) );
    }
  } // </pn>

  meth_elt.add_elt( METHODS_XMLNS, "lead-head" )
    .add_content( meth.lh().print() );

  { // <classification>
    dom_element cl_elt = meth_elt.add_elt( METHODS_XMLNS, "classification" );

    { // <cc-class>
      dom_element cc_elt = cl_elt.add_elt( METHODS_XMLNS, "cc-class");
      if((methclass & method::M_MASK) != method::M_UNKNOWN)
        cc_elt.add_attr( NULL, "class", 
                         txt_classes[methclass & method::M_MASK] );
      if(methclass & method::M_LITTLE)
        cc_elt.add_attr( NULL, "little", "true" );
      if(methclass & method::M_DIFFERENTIAL)
        cc_elt.add_attr( NULL, "differential", "true" );
    } // </cc-class>
    { // <lhcode>
      dom_element lhcode_elt = cl_elt.add_elt( METHODS_XMLNS, "lhcode" );
      const char *lhcode = meth.lhcode();
      if(lhcode[0] =='\0' || lhcode[0] == 'z')
        lhcode_elt.add_attr( XSI_XMLNS, "xsi:nil", "true" );
      else
        lhcode_elt.add_attr( NULL, "code", lhcode );
    } // </lhcode>
  } // </classification>

  { // <performances>
    dom_element perf_elt;

    if ( entry.has_facet< first_tower_peal >() ) {
      if (!perf_elt) 
	perf_elt = meth_elt.add_elt( METHODS_XMLNS, "performances" );
      add_peal( perf_elt.add_elt( METHODS_XMLNS, "firsttower" ),
		entry.get_facet< first_tower_peal >() );
    }    

    if ( entry.has_facet< first_hand_peal >() ) {
      if (!perf_elt) 
	perf_elt = meth_elt.add_elt( METHODS_XMLNS, "performances" );
      add_peal( perf_elt.add_elt( METHODS_XMLNS, "firsthand" ),
		entry.get_facet< first_hand_peal >() );  
    }
  } // </performances>

  {
    dom_element refs_elt;

    if ( entry.has_facet< rw_ref >() ) {
      if ( !refs_elt ) 
        refs_elt = meth_elt.add_elt( METHODS_XMLNS, "refs" );
      refs_elt.add_elt( METHODS_XMLNS, "rwref" )
        .add_content( entry.get_facet< rw_ref >() );
    }
  }
}

xmlout::xmlout( const string& filename )
  : libout( new impl(filename) )
{}

RINGING_END_NAMESPACE
