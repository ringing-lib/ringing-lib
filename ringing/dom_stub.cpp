// -*- C++ -*- dom_stub.cpp - Stub DOM wrapper
// Copyright (C) 2008 Richard Smith <richard@ex-parrot.com>.

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

RINGING_START_NAMESPACE

RINGING_USING_STD

class dom_document::impl 
{
};

class dom_element::impl 
{
};

dom_document::dom_document( string const& filename, io mode, filetype type )
{
  throw runtime_error( "XML libraries not supported in this build" );
}

void dom_document::finalise()
{
}

dom_element::dom_element( impl* i )
{
}

dom_element dom_document::get_document() const
{
  return dom_element();
}

dom_element dom_document::create_document( char const* ns, char const* qn )
{
  return dom_element();
}

string dom_element::get_name() const
{
  return string();
}
string dom_element::get_content() const
{
  return string();
}

dom_element dom_element::get_first_child() const
{
  return dom_element();
}

dom_element dom_element::get_next_sibling() const
{
  return dom_element();
}


dom_element dom_element::add_elt( char const* ns, char const* name )
{
  return dom_element();
}

void dom_element::add_content( string const& content )
{
}

void dom_element::add_attr( char const* ns, char const* name, char const* val )
{
}

RINGING_END_NAMESPACE

