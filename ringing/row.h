// -*- C++ -*- row.h - Classes for rows and changes
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

#ifndef RINGING_ROW_H
#define RINGING_ROW_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include RINGING_STD_HEADER(iostream)
#include <string>
#include RINGING_STD_HEADER(vector)
RINGING_USING_STD

RINGING_START_NAMESPACE

// Declare a couple of random functions for hcf and lcm
int hcf(int a, int b);
int inline lcm(int a, int b) 
{
  // Check for a couple of quick cases
  if(a == 1) return b;
  if(b == 1) return a;
  return a * b / hcf(a,b);
}

class bell {
private:
  unsigned char x;
  static char symbols[];	// Symbols for the individual bells
public:
  bell() : x(0) {}
  bell(int i) : x(i) {}
  void from_char(char c) { 
    for(x = 0; x < 33 && symbols[x] != c; x++);
    if(x == 33) x = 0;
  }
  operator int() const { return x; }
  bell& operator=(int i) { x = i; return *this; }
  char to_char() const { return (x < 33) ? symbols[x] : '*'; }
};
inline ostream& operator<<(ostream& o, const bell b) 
  { return o << b.to_char(); }

class row;

// change : This stores one change
class change {
private:
  int n;			// Number of bells
  vector<bell> swaps;		// List of pairs to swap

public:
  change() : n(0) {}	        //
  change(int num) : n(num) {}   // Construct an empty change
  change(int num, char *pn) { set(num, pn); }
  // Use default copy constructor and assignment

  change& set(int num, char *pn); // Assign from place notation
  int operator==(const change& c) const
  { return (n == c.n) && (n == 0 || swaps == c.swaps); }
  int operator!=(const change& c) const
    { return !(*this == c); }
  change reverse(void) const;		 // Return the reverse

  friend row& operator*=(row& r, const change& c);
  friend bell& operator*=(bell& i, const change& c);

  char *print(char *pn) const;	// Print place notation to a string
  int bells(void) const { return n; } // Return number of bells
  int sign(void) const;		// Return whether it's odd or even
  int findswap(bell which) const; // Check whether a particular swap is done
  int findplace(bell which) const; // Check whether a particular place is made
  int swap(bell which);		// Swap or unswap a pair
  int internal(void) const;	// Does it contain internal places?
};

ostream& operator<<(ostream& o, const change& c); // Write a change to a stream
bell& operator*=(bell& i, const change& c); // Apply a change to a position
inline bell operator*(bell i, const change& c)
{
  bell j = i; j *= c; return j;
}


// row : This stores one row 
class row {
private:
  vector<bell> data;	        // The actual row

public:
  row() {}
  row(int num) : data(num) {}	// Construct an empty row
  row(char *s);			// Construct a row from a string
  // Use default copy constructor and copy assignment

  row& operator=(char *s);	// Assign a string
  int operator==(const row& r) const; // Compare
  int operator!=(const row& r)
    { return !(*this == r); }
  bell operator[](int i) const	// Return one particular bell (not an lvalue).
    { return data[i]; }
  row operator*(const row& r) const; // Transpose one row by another
  row& operator*=(const row& r);
  row operator/(const row& r) const; // Inverse of transposition
  row& operator/=(const row& r);
  friend row& operator*=(row& r, const change& c); // Apply a change to a row
  row operator*(const change& c) const;	
  row inverse(void) const;	// Find the inverse

  char *print(char *s) const;	// Print the row into a string
  int bells(void) const { return data.size(); } // How many bells?
  row& rounds(void);		// Set it to rounds
  static row rounds(int n);	// Return rounds on n bells
  static row pblh(int n, int h=1); // Return ith plain bob lead head on n bells
				// with h hunt bells
  int isrounds(void) const;	// Is it rounds?
  int ispblh(void) const;	// Which plain bob lead head is it?
  int sign(void) const;         // Return whether it's odd or even
  char *cycles(char *result) const; // Express it as a product of disjoint cycles
  int order(void) const;	    // Return the order
  friend ostream& operator<<(ostream&, const row&);
};

ostream& operator<<(ostream& o, const row& r); // Write a row to a stream

// An operator which has to be here
row& operator*=(row& r, const change& c);

// row_block : Stores some rows, along with a pointer to some
// changes from which they can be calculated.
class row_block : public vector<row> {
private:
  const vector<change>& ch;	  // The changes which these rows are based on

  // These are here to stop anybody calling them
  row_block(row_block &b);
  row_block& operator=(row_block &b);

public:
  row_block(const vector<change> &c);	         // Starting from rounds
  row_block(const vector<change> &c, const row &r); // Starting from the given row

  row& set_start(row& r)	// Set the first row
    { (*this)[0] = r; return (*this)[0]; }
  row_block& recalculate(int start = 0); // Recalculate rows from changes
  const vector<change>& get_changes(void) const // Return the changes which we are using
    { return ch; }
};

RINGING_END_NAMESPACE

#endif





