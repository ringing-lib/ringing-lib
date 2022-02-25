// -*- C++ -*- method_stream.cpp - lightweight output of methods
// Copyright (C) 2021, 2022 Richard Smith <richard@ex-parrot.com>.

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <iostream>

#include <ringing/method_stream.h>
#include <ringing/litelib.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class method_stream::impl : public libout::interface {
public:
  explicit impl(bool inc_bells, name_form form) 
    : inc_bells(inc_bells), form(form) {}
  
  virtual void append( library_entry const& entry ) {
    if (inc_bells) cout << entry.bells() << ":";
    cout << entry.pn();
    if (form != none) cout << "\t";
    switch (form) {
      case payload_or_name: 
        if (entry.has_facet<litelib::payload>())
          cout << entry.get_facet<litelib::payload>();
        else
          cout << entry.name();
        break;
      case payload:
        if (entry.has_facet<litelib::payload>())
          cout << entry.get_facet<litelib::payload>();
        break;
      case name:
        cout << entry.name();
        break;
      case full_title:
        cout << entry.fullname();
        break;
    }
    cout << endl;
  }

private:
  bool inc_bells;
  name_form form;
};

method_stream::method_stream(bool inc_bells, name_form form) 
  : libout( new impl(inc_bells, form) ) 
{}

RINGING_END_NAMESPACE

