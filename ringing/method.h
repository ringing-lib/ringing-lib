// method.h - Stuff all about methods, positions, calls etc

#ifndef METHLIB_METHOD_H
#define METHLIB_METHOD_H

#ifdef __GNUG__
#pragma interface
#endif

#include <list.h>
#include <string>
#include "row.h"
#include "stuff.h"

// method - A method.
class method : public changes {
private:
  string myname;		// The name of the method, without Major etc. 

// Stop anybody calling these
  method(method& m);
  method& operator=(method& m);

  static const char *txt_classes[11]; // Bob, Place etc.
  static const char *txt_stages[10];  // Minimum, Doubles etc.

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

  method(int l, int b, char *n = "Untitled") : changes(l,b)
    { name(n); }
  // Make a method from place notation
  method(char *pn, int b, char *n = "Untitled");
  ~method() {}

  int issym(void) const;	// Is it symmetrical?
  int isdouble(void) const;	// Is it double?
  int isregular(void) const 	// Is it regular?
    { return !!asrow().ispblh(); }
  int huntbells(void) const;	// Number of hunt bells
  int leads(void) const 	// Number of leads in a plain course
    { return asrow().order(); }
  int issym(int b) const;	// Is this bell's path symmetrical?
  int isplain(int b=0) const;	// Does this bell plain hunt?
  int hasdodges(int b) const;	// Does this bell ever dodge?
  int hasplaces(int b) const;	// Does this bell make internal places?
  int methclass(void) const; // What sort of method is it?
  char *lhcode(void) const;	 // Return the lead head code
};

#endif



