// -*- C++ -*- method.h - Classes for dealing with methods
// Copyright (C) 2001 Martin Bright <M.Bright@dpmms.cam.ac.uk>

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

#ifndef RINGING_METHOD_H
#define RINGING_METHOD_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <list.h>
#else
#include <list>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/stuff.h>

RINGING_USING_STD

RINGING_START_NAMESPACE

// method - A method.
class method : public vector<change> {
private:
  string myname;		// The name of the method, without Major etc. 

  static const char *txt_classes[11]; // Bob, Place etc.
  static const char *txt_stages[14];  // Minimum, Doubles etc.

public: 

  enum m_class {
    M_UNKNOWN,
    M_PRINCIPLE,
    M_BOB,
    M_PLACE,
    M_TREBLE_BOB,
    M_SURPRISE,
    M_DELIGHT,
    M_TREBLE_PLACE,
    M_ALLIANCE,
    M_HYBRID,
    M_SLOW_COURSE,
    M_MASK = 0x0f,
    M_DOUBLE = 0x40,
    M_LITTLE = 0x80
  };
  
  static const char *txt_double; // Double
  static const char *txt_little; // Little

  const char *name(void) const	// Get name
    { return myname.c_str(); }
  void name(const char *n)	// Set name
    { myname = n; }
  char *fullname(char *c) const; // Return the full name
  
  static const char *stagename(int n); // Get the name of this stage
  static const char *classname(int cl) // Get the name of the class
    { return txt_classes[cl & M_MASK]; }

  method(int l, int b, char *n = "Untitled") : vector<change>(l)
    { (*this)[0] = change(b); name(n); }
  // Make a method from place notation
  method(char *pn, int b, char *n = "Untitled");
  ~method() {}

  int length() const { return size(); }
  int bells() const { return empty() ? 0 : (*this)[0].bells(); }
  row lh() const { 
    vector<change>::const_iterator i;
    row r(bells()); r.rounds();
    for(i=begin(); i != end(); i++) r *= *i;
    return r;
  }
  int issym(void) const;	// Is it symmetrical?
  int isdouble(void) const;	// Is it double?
  int isregular(void) const 	// Is it regular?
    { return !!lh().ispblh(); }
  int huntbells(void) const;	// Number of hunt bells
  int leads(void) const 	// Number of leads in a plain course
    { return lh().order(); }
  int issym(bell b) const;	// Is this bell's path symmetrical?
  int isplain(bell b=0) const;	// Does this bell plain hunt?
  int hasdodges(bell b) const;	// Does this bell ever dodge?
  int hasplaces(bell b) const;	// Does this bell make internal places?
  int methclass(void) const; // What sort of method is it?
  char *lhcode(void) const;	 // Return the lead head code
};

RINGING_END_NAMESPACE

#endif



