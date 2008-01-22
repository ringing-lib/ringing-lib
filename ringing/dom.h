// -*- C++ -*- dom.h - A very lightweight DOM wrapper
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

#ifndef RINGING_DOM_H
#define RINGING_DOM_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class dom_element;

class dom_document {
public:
  enum io { in, out };
  enum filetype { stdio, file, url };
  dom_document( string const& name, io mode, filetype type );
 
  dom_element get_document() const;
  dom_element create_document( char const* ns, char const* qname );

  void finalise();

private:
  class impl;
  scoped_pointer<impl> pimpl;
};

class dom_element {
public:
  dom_element() {}
  operator bool() const { return pimpl; }

  dom_element add_elt( char const* ns, char const* name );
  void add_attr( char const* ns, char const* name, char const* val );
  void add_content( string const& content );

  string get_name() const;
  string get_content() const;

  dom_element get_first_child() const;
  dom_element get_next_sibling() const;

private:
  class impl;
  dom_element( impl* );
  
  friend class dom_document;

  shared_pointer<impl> pimpl;
};

RINGING_END_NAMESPACE

#endif // RINGING_DOM_H

