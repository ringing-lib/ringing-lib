// row.h - definitions

#ifndef METHLIB_ROW_H
#define METHLIB_ROW_H

#ifdef __GNUG__
#pragma interface
#endif

#include <iostream.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vector.h>


// Declare a couple of random functions for hcf and lcm
int hcf(int a, int b);
int inline lcm(int a, int b) 
{
  // Check for a couple of quick cases
  if(a == 1) return b;
  if(b == 1) return a;
  return a * b / hcf(a,b);
}

class row;

// change : This stores one change
class change {
private:
  char n;			// Number of bells
  char *swaps;			// List of pairs to swap

  change()  : swaps(NULL) {}	// Only to be called from changes
  void init(int num)		// Only to be called from changes
    { swaps = new char[(n = num)/2+1]; swaps[0] = 100; }
  friend class changes;
  friend class vector<change>;

public:
  change(int num)		// Construct an empty change
    { swaps = new char[(n = num)/2+1]; swaps[0] = 100; }
  change(const change& c);	// Copy constructor
  change(int num, char *pn);	// Construct a change from place notation
  ~change()			// Deallocate everything
    { delete[] swaps; }
  change& operator=(const change& c); // Assignment operator
  change& operator=(char *pn);	// Assign from place notation
  int operator==(const change& c) const; // Compare
  int operator!=(const change& c) const
    { return !(*this == c); }
  change reverse(void) const;		 // Return the reverse

  friend row& operator*=(row& r, const change& c);
  friend int& operator*=(int& i, const change& c);

  char *print(char *pn) const;	// Print place notation to a string
  int bells(void) const { return n; } // Return number of bells
  int sign(void) const;		// Return whether it's odd or even
  int findswap(int which) const; // Check whether a particular swap is done
  int findplace(int which) const; // Check whether a particular place is made
  int swap(int which);		// Swap or unswap a pair
  int internal(void) const;	// Does it contain internal places?
};

ostream& operator<<(ostream& o, const change& c); // Write a change to a stream
int& operator*=(int& i, const change& c); // Apply a change to a position
inline int operator*(int i, const change& c)
{
  int j = i; j *= c; return j;
}

// changes : This stores a block of changes, such as a lead of a method,
//           or a bob or a single.
class changes : protected vector<change> {
public:
  changes(int l, int b) : vector<change>(l) // Allocate all the changes
    { for(int i = 0; i < l; i++) (*this)[i].init(b); }
  changes() {}			// Don't do anything much
  changes(changes& c) : vector<change>(c) {} // Copy constructor
  changes& operator=(changes& c) // Copy assignment
    { vector<change>::operator=(c); return *this; }
  ~changes() {}			// Deallocate everything 

  change& operator[](int i)	// Just return a change
    { return vector<change>::operator[](i); }
  const change& operator[](int i) const
    { return vector<change>::operator[](i); }
  int length(void) const	// Return the number of changes
    { return size(); }
  int bells(void) const		// Return the number of bells
    { return (*this)[0].bells(); }
  row asrow(void) const;	// Express it all as one row
};

// row : This stores one row 
class row {
private:
  char n;			// Number of bells
  char *data;			// The actual row
  static char symbols[];	// Symbols for the individual bells

  row()				// Only to be called from within rows
    { data = NULL; }
  void init(int num)		// Ditto
    { if(data == NULL) { n = num; data = new char[n]; }  }
  friend class rows;
  friend class vector<row>;

public:
  row(int num) : n(num)		// Construct an empty row
    { data = new char[n]; }
  row(char *s);			// Construct a row from a string
  row(const row& r);		// Copy constructor
  ~row()			// Free the stuff we allocated
    { delete[] data; }


  row& operator=(const row& r);	// Copy one row to another
  row& operator=(char *s);	// Assign a string
  int operator==(const row& r) const; // Compare
  int operator!=(const row& r)
    { return !(*this == r); }
  int operator[](int i) const	// Return one particular bell (not an lvalue).
    { return data[i]; }
  row operator*(const row& r) const; // Transpose one row by another
  row& operator*=(const row& r);
  row operator/(const row& r) const; // Inverse of transposition
  row& operator/=(const row& r);
  friend row& operator*=(row& r, const change& c); // Apply a change to a row
  row operator*(const change& c) const;	
  row inverse(void) const;	// Find the inverse

  static char b_to_c(char b);	// Convert a bell to a printable char
  static char c_to_b(char c);	// Convert a char to a bell number
  char *print(char *s) const;	// Print the row into a string
  int bells(void) const { return n; } // Tell how many bells this row has
  row& rounds(void);		// Set it to rounds
  static row rounds(int n);	// Return rounds on n bells
  static row pblh(int n, int h=1); // Return ith plain bob lead head on n bells
				// with h hunt bells
  int isrounds(void) const;	// Is it rounds?
  int ispblh(void) const;	// Which plain bob lead head is it?
  int sign(void) const;         // Return whether it's odd or even
  char *cycles(char *result) const; // Express it as a product of disjoint cycles
  int order(void) const;	    // Return the order
};

ostream& operator<<(ostream& o, const row& r); // Write a row to a stream

// rows : Stores a block of rows
class rows : protected vector<row> {
public:
  rows() {}			// Don't do a lot
  rows(int l, int b) : vector<row>(l) // Allocate and initialise all the rows
    { for(int i = 0; i < l; i++) (*this)[i].init(b); }
  rows(const rows& r) : vector<row>(r) {} // Copy constructor
  rows& operator=(rows& r)	// Copy assignment
    { vector<row>::operator=(r); return *this; }
  ~rows() {}			// Deallocate everything 

  const row& operator[](int i) const // Just return that row
    { return vector<row>::operator[](i); }
  row& operator[](int i)	// Ditto
    { return vector<row>::operator[](i); }
  int length(void) const	// Return the number of rows
    { return size(); }
  int bells(void) const		// Return the number of bells
    { return (*this)[0].bells(); }
};

// An operator which has to be here
row& operator*=(row& r, const change& c);

// row_block : Stores some rows, along with a pointer to some
// changes from which they can be calculated.
class row_block : public rows {
private:
  const changes *ch;			// The changes which these rows are based on

  // These are here to stop anybody calling them
  row_block(row_block &b);
  row_block& operator=(row_block &b);

public:
  row_block(const changes &c);	         // Starting from rounds
  row_block(const changes &c, const row &r); // Starting from the given row

  row& set_start(row& r)	// Set the first row
    { (*this)[0] = r; return (*this)[0]; }
  row_block& recalculate(int start = 0); // Recalculate rows from changes
  const changes& get_changes(void) const // Return the changes which we are using
    { return *ch; }
};

#endif





