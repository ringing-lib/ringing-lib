// -*- C++ -*- row.h - Classes for rows and changes
// Copyright (C) 2001, 2007, 2008, 2009 Martin Bright <martin@boojum.org.uk>
// and Richard Smith <richard@ex-parrot.com>

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


#ifndef RINGING_ROW_H
#define RINGING_ROW_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <ostream.h>
#include <vector.h>
#include <stdexcept.h>
#include <utility.h>
#else
#include <ostream>
#include <vector>
#include <stdexcept>
#include <utility>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#else
#include <cctype>
#endif
#include <string>

#include <ringing/bell.h>
#include <ringing/change.h> // For row_block

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
#include <ringing/change.h> 
#include <ringing/mathutils.h>
#include <ringing/place_notation.h>
#endif

#if _MSC_VER
// Something deep within the STL in Visual Studio decides to 
// turn this warning back on.  
#pragma warning(disable: 4231)
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class change;

// row : This stores one row 
class RINGING_API row {
private:
  vector<bell> data;	        // The actual row

public:
  row() {}
  explicit row(int num);	// Construct rounds on n bells
  row(const char *s);			// Construct a row from a string
  row(const string &s);			// Construct a row from a string
  explicit row(const vector<bell>& d);  // Construct from data
  // Use default copy constructor and copy assignment

  row& operator=(const char *s);	// Assign a string
  row& operator=(const string &s);	// Assign a string
  bool operator==(const row& r) const  // Compare
    { return data == r.data; }
  bool operator!=(const row& r) const
    { return data != r.data; }
  bell operator[](int i) const	// Return one particular bell (not an lvalue).
    { return data[i]; }
  row operator*(const row& r) const; // Transpose one row by another
  row& operator*=(const row& r);
  row operator/(const row& r) const; // Inverse of transposition
  row& operator/=(const row& r);
  friend RINGING_API row& operator*=(row& r, const change& c); // Apply a change to a row
  row operator*(const change& c) const;	
  row inverse(void) const;	// Find the inverse
  row power(int n) const;       // Fidn the nth power of the row

  string print() const;		// Print the row into a string
  int bells(void) const { return data.size(); } // How many bells?
  row& rounds(void);		// Set it to rounds

  static row rounds(const int n) { return row(n); } // Return rounds on n bells

  static row queens(const int n);  // Return queens on n bells
  static row kings(const int n);   // Return kings on n bells
  static row tittums(const int n); // Return tittums on n bells
  static row reverse_rounds(const int n); // Return reverse rounds on n bells

  static row cyclic(int n, int h=1, int c=1); // Return cyclic lead head 
                                   // (13456..2)^c on n bells with h hunt bells
  static row pblh(int n, int h=1); // Return first plain bob lead head on 
                                   // n bells with h hunt bells
  bool isrounds(void) const;	// Is it rounds?
  int ispblh(void) const;	// Which plain bob lead head is it?
  int ispblh(int h) const;	// Which plain bob lh (with h hunts) is it?
  int sign(void) const;         // Return whether it's odd or even
  string cycles() const;        // Express it as a product of disjoint cycles
  int order(void) const;	    // Return the order
  friend RINGING_API ostream& operator<<(ostream&, const row&);
  friend RINGING_API istream& operator>>(istream&, row&);
  void swap(row &other) { data.swap(other.data); }
  void swap(vector<bell>& other) { data.swap(other); validate(); }
  size_t hash() const;

  int find(bell const& b) const;// Finds the bell
  
  struct invalid : public invalid_argument {
    invalid();
    invalid(const string& s);
  };

  // So that we can put rows in containers
  bool operator<(const row& r) const { return data < r.data; }
  bool operator>(const row& r) const { return data > r.data; }
  bool operator<=(const row& r) const { return data <= r.data; }
  bool operator>=(const row& r) const { return data >= r.data; }

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
  char *print(char *s) const;   // This overload is deprecated.
  char *cycles(char *result) const; // This overload is deprecated.
#endif

  typedef vector<bell>::const_iterator const_iterator;
  const_iterator begin() const { return data.begin(); }
  const_iterator end() const { return data.end(); }

private:
  void validate() const;
};

inline ostream& operator<<(ostream& o, const row& r) {
  return o << r.print();
}
RINGING_API istream& operator>>(istream& i, row& r);

// An operator which has to be here
RINGING_API row& operator*=(row& r, const change& c);

struct RINGING_API permuter
{
  typedef row result_type;
  explicit permuter(unsigned n) : r(row::rounds(n)) {}

  const row &operator()(const change &c) { return r *= c; }
  const row &operator()(const row &c) { return r *= c; }

  row const& get() const { return r; }

private:
  row r;
};

struct RINGING_API row_permuter
{
  typedef row result_type;
  explicit row_permuter(row &r) : r(r) {}  

  const row &operator()(const change &c) { return r *= c; }
  const row &operator()(const row &c) { return r *= c; }

private:
  row &r;
};

inline RINGING_API permuter permute(unsigned n) { return permuter(n); }
inline RINGING_API row_permuter permute(row &r) { return row_permuter(r); }

struct RINGING_API post_permuter
{
  typedef row result_type;
  explicit post_permuter(unsigned n) : r(row::rounds(n)) {}

  row operator()(const change &c) { row tmp(r); r *= c; return tmp; }
  row operator()(const row &c) { row tmp(r); r *= c; return tmp; }

  row const& get() const { return r; }

private:
  row r;
};

struct RINGING_API row_post_permuter
{
  typedef row result_type;
  explicit row_post_permuter(row &r) : r(r) {}  

  row operator()(const change &c) { row tmp(r); r *= c; return tmp; }
  row operator()(const row &c) { row tmp(r); r *= c; return tmp; }

private:
  row &r;
};

inline RINGING_API 
post_permuter post_permute(unsigned n) { return post_permuter(n); }
inline RINGING_API 
row_post_permuter post_permute(row &r) { return row_post_permuter(r); }

#if RINGING_AS_DLL
RINGING_EXPLICIT_STL_TEMPLATE vector<row>;
#endif

// row_block : Stores some rows, along with a pointer to some
// changes from which they can be calculated.
class RINGING_API row_block : public vector<row> {
private:
  const vector<change>& ch;	  // The changes which these rows are based on

public:
  row_block(const vector<change> &c);	         // Starting from rounds
  row_block(const vector<change> &c, const row &r); // Starting from the given row

  row& set_start(const row& r)	// Set the first row
    { (*this)[0] = r; return (*this)[0]; }
  row_block& recalculate(int start = 0); // Recalculate rows from changes
  const vector<change>& get_changes(void) const // Return the changes which we are using
    { return ch; }
};
      
RINGING_END_NAMESPACE

// specialise std::swap and std::hash if it exists
RINGING_DELEGATE_STD_SWAP( row )
RINGING_DELEGATE_STD_HASH( row )

#endif
