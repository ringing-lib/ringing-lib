// -*- C++ -*- method.h - Classes for dealing with methods
// Copyright (C) 2001, 2009, 2010, 2011, 2014, 2020, 2021
// Martin Bright <martin@boojum.org.uk> and
// Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_METHOD_H
#define RINGING_METHOD_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif
#include <string>

#include <ringing/change.h>

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
#include <ringing/row.h>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class row;

// method - A method.
class RINGING_API method : public vector<change> {
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
    M_DIFFERENTIAL = 0x10,
    M_LITTLE = 0x80
  };
  
  static const char *txt_double; // Double
  static const char *txt_little; // Little

  const char *name() const      { return myname.c_str(); }
  void name(const string& n)    { myname = n; }
  string fullname() const;

  static const char *stagename(int n); // Get the name of this stage
  static string classname(int cl); // Get the name of the class

  // Use txt instead of 'Untitled' for unnamed methods
  static void set_untitled(string const& txt);

  explicit method(int l = 0, int b = 0, const char *n = txt_untitled) 
    : vector<change>(l, change(b)), b(b), myname(n) {}

  // Make a method from place notation
  method(const char *pn, int b, const char *n = txt_untitled);
  method(const string& pn, int b, const string& n = txt_untitled);

  explicit method(vector<change> const& ch, const string& n = txt_untitled);

  ~method() {}
  void swap( method& other ) {
    vector<change>::swap(other); 
    RINGING_PREFIX_STD swap(b, other.b);
    RINGING_PREFIX_STD swap(myname, other.myname);
  }      

  void push_back(const change &ch)
    { b = b > ch.bells() ? b : ch.bells(); vector<change>::push_back(ch); }
  void push_back(const string &str)
    { vector<change>::push_back(change(bells(), str)); }

  int length() const { return size(); }
  int bells() const { return b; }
  row lh() const;
  bool issym(void) const;	// Is it palindromic about the usual point?
  bool ispalindromic() const;   // Is it palindromic about any point?
  bool isdouble(void) const;	// Is it double?
  bool isregular(void) const;	// Is it regular?
  int huntbells(void) const;	// Number of hunt bells
  int leads(void) const;	// Number of leads in a plain course
  bool issym(bell b) const;	// Is this bell's path symmetrical?
  bool ispalindromic(bell b) const;   // Is it palindromic about any point?
  bool isplain(bell b=0) const;	// Does this bell plain hunt?
  bool hasdodges(bell b) const;	// Does this bell ever dodge?
  bool hasplaces(bell b) const;	// Does this bell make internal places?
  int methclass(int year=9999) const;    // What sort of method is it?
  char *lhcode(void) const;	// Return the lead head code
  int symmetry_point() const;   // Point of palindromic symmetry (or -1)
  int symmetry_point(bell b) const;   // Point of palindromic symmetry (or -1)
  int maxblows(void) const;     // Counts the maximum blows in one place

  bool is_palindromic_about(int i) const;
  bool is_palindromic_about(bell b, int i) const;

  enum m_format {
    M_DOTS          =   01,  // Include all dots
    M_EXTERNAL      =   02,  // Include all external places
    M_UCROSS        =   04,  // Use an upper-case 'X' for the cross change
    M_LCROSS        =  010,  // Use a lower-case 'x' for the cross change
    M_DASH          =  020,  // Use a dash ('-') for the cross change
    M_SYMMETRY      =  040,  // Separate palindromic methods into two components
    M_FULL_SYMMETRY = 0140,  // As above but also with unconventional sym points
    M_OMIT_LH       = 0200,  // Omit the lead head change.
    M_PLUS          = 0400   // Prefix asymmetric sections with '+'
  };

  string format( int flags = 0 ) const; // Format the place notation for output

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
  char *fullname(char *c) const; // Return the full name
#endif

 private:
  int b;                        // The number of bells
  string myname;		// The name of the method, without Major etc. 

  static const char *txt_untitled;

  static const char *txt_classes[12];  // Bob, Place etc.
  static const char *txt_differential; // Differential
  static const char *txt_stages[20];   // Minimus, Doubles etc.
};

RINGING_END_NAMESPACE

RINGING_DELEGATE_STD_SWAP( method )

#endif



