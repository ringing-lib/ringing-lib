// -*- C++ -*- pdf_fonts.h - Handling of PDF font metrics
// Copyright (C) 2001 Martin Bright <martin@boojum.org.uk>

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

#ifndef RINGING_PDF_FONTS_H
#define RINGING_PDF_FONTS_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <map.h>
#else
#include <map>
#endif
#include <string>

RINGING_START_NAMESPACE

RINGING_USING_STD
  
class charwidths {
private:
  static map<string, const int*> widths;
  map<string, const int*>::const_iterator current_font; 

  void init();

public:
  charwidths() { if(widths.empty()) init(); }

  const string& font() { return (*current_font).first; }
  bool font(const string& f) 
    { return (current_font = widths.find(f)) != widths.end(); }
  int operator()(unsigned char c) const 
    { return (c >= 32 && c != 255) ? (*current_font).second[c-32] : 0; }
  int operator()(const string& s) const {
    string::const_iterator i;
    int w = 0;
    for(i = s.begin(); i != s.end(); i++) 
      w += (*this)(*i);
    return w;
  }
};

RINGING_END_NAMESPACE

#endif
