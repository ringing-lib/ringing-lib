// method.cc - routines for methods, positions and calls

#ifdef __GNUG__
#pragma implementation
#endif

#include "method.h"
#include "stuff.h"
#include <string.h>
#include <stdio.h>		// I want sprintf() !

// *********************************************************************    
// *                    Functions for class method                     *
// *********************************************************************

// A few static strings

const char *method::txt_classes[11] = {
  "",
  "",
  "Bob",
  "Place",
  "Treble Bob",
  "Surprise",
  "Delight",
  "Treble Place",
  "Alliance",
  "Hybrid",
  "Slow Course"
  };

const char *method::txt_stages[10] = {
  "Singles",
  "Minimus",
  "Doubles",
  "Minor",
  "Triples",
  "Major",
  "Caters",
  "Royal",
  "Cinques",
  "Maximus"
  };

const char *method::txt_double = "Double";
const char *method::txt_little = "Little";

// Constructor to make a method from place notation
// The place notation should be expanded, i.e. no symmetrical bits
method::method(char *pn, int b, char *n)
{
  name(n);
  
  // First find out the number of bells
  char *c;
/*  int a, b = 0;
  for(c = pn; *c != '\0'; c++)
    if(b < (a = row::c_to_b(*c) + 1)) b = a;
*/
  // Work out how long it is
  int l = 0;
  c = pn;
  if(*c != 'X' && *c != 'x' && *c != '-' && *c != '.') l = 1;
  for(; *c != '\0'; c++) {
    if(*c == '.') l++;
    if(*c == 'X' || *c == 'x' || *c == '-') l += 2;
  }

  // Get a temporary block of changes
  changes ch(l, b);

  // Set up the place notation
  char t[32];
  int i;
  c = pn;
  for(i = 0; i < l; i++) {
    if(*c == 'X' || *c == '-') {
      t[0] = 'X'; t[1] = '\0';
      c++;
    } else {
      char *s = t;
      while(*c != '\0' && *c != 'X' && *c != '-' && *c != '.')
	*s++ = *c++;
      *s = '\0';
      if(*c == '.') c++;
    }
    ch[i] = t;
  }

  // Copy the changes into this method
  *(changes *)this = ch;
}

// issym : Find out whether the method is symmetrical
// This means symmetrical, not counting the half-lead or lead end.
int method::issym(void) const
{
  if(length() & 1) return 0;	// Must have an even length
  for(int i = 0; i < length() / 2 - 1; i++)
    if((*this)[i] != (*this)[length() - i - 2]) return 0;
  return 1;
}

// isdouble : Find out whether the method is double
// This doesn't necessarily mean it's symmetrical
// (e.g. Double Oxford Bob Triples)
int method::isdouble(void) const
{
  if(length() & 1) return 0;	// Must have even length
  for(int i = 0; i < length() / 2; i++)
    if((*this)[i] != (*this)[length() / 2 + i].reverse()) return 0;
  return 1;
}

// huntbells : Find the number of hunt bells
int method::huntbells(void) const
{
  int n = 0;
  row r = asrow();
  for(int i = 0; i < bells(); i++)
    if(r[i] == i) n++;
  return n;
}

// issym : Is this bell's path symmetrical?
int method::issym(int b) const
{
  if(length() & 1) return 0;	// Must have even length
  for(int i = 0; i < length() / 2 - 1; i++) {
    if((*this)[i].findswap(b-1) != (*this)[length() - i - 2].findswap(b-1))
      return 0;
    if((*this)[i].findswap(b) != (*this)[length() - i - 2].findswap(b))
      return 0;
    b *= (*this)[i];
  }
  if(b * (*this)[length()/2-1] != b) // Must make a place at the half lead
    return 0;
  return 1;
}

// isplain : Does this bell plain hunt?
int method::isplain(int b) const
{
  int j = b;
  int dir = j & 1;		// Odd bells out, even bells in
  int dirchanges = 0;		// Count changes of direction
  for(int i = 0; i < length(); i++) {
    if((*this)[i].findswap(j-1+dir)) return 0; // No, that's a dodge.
    if((*this)[i].findswap(j-dir)) // Carry on in the same direction
      if(dir) j--; else j++;
    else {			// Make a place and change direction
      dir ^= 1;
      dirchanges++;
    }
  } 
  return (j == b && dirchanges == 2);
}

// hasdodges : Does this bell do any dodges?
int method::hasdodges(int b) const
{
  int i, j = b, j1 = -1, j2 = -1;
  for(i = 0; i < length(); i++) {
    j2 = j1; j1 = j;
    j *= (*this)[i];
    if(j == j2 && j != j1) return 1;
  }
  return 0;
}

// hasplaces : Does this bell make any internal places?
int method::hasplaces(int b) const
{
  int i, j = b, j1;
  for(i = 0; i < length(); i++) {
    j1 = j;
    j *= (*this)[i];
    if(j == j1 && j > 0 && j < bells())
      return 1;
  }
  return 0;
}
    
// meth_class : What class of method is it?
int method::methclass(void) const
{
  int cl = 0;
  int i, j;
  int hb;

  // Is it double?
  if(isdouble()) cl = M_DOUBLE;

  // Find the first hunt bell
  row lh = asrow();
  for(hb = 0; lh[hb] != hb && hb < bells(); hb++);
  if(hb == bells()) return cl | M_PRINCIPLE;

  // Find how far the treble ever gets
  int tmin = hb, tmax = hb;
  j = hb;
  for(i = 0; i < length(); i++) {
    j *= (*this)[i];
    if(j < tmin) tmin = j;
    if(j > tmax) tmax = j;
  }
  if(tmax < bells()-1 || tmin > 0) cl |= M_LITTLE;
  
  // Is the treble path symmetrical?
  if(!issym(hb)) return cl | M_HYBRID;
 
  // Is it plain hunting?
  if(isplain(hb)) {
    // Check for Slow Course methods
    if(hb == 0 && lh[1] == 1 && issym(1))
      return cl | M_SLOW_COURSE;
    // Now see whether any of the working bells make any dodges
    for(i = 1; i < bells(); i++)
      if(hasdodges(i)) return cl | M_BOB;
    return cl | M_PLACE;
  }
  
  // Now count the number of blows in each position in the half lead,
  // and also note whether the treble makes any places.
  int k, place = 0;
  j = hb;
  buffer count(tmax-tmin+1);
  for(i = tmin;i < tmax;i++) count[i] = 0;
  for(i = 0;i < (length()/2 - 1);i++) {
    count[j-tmin]++;
    k = j;
    j *= (*this)[i];
    if(j == k)
      place = 1;		// Treble contains places
  }
  count[j-tmin]++;

  // Go through and see whether we spent the same amount of time
  // in each position
  j = 1;
  for(i = tmin+1;i < tmax;i++)
    if(count[i] != count[i-1]) j = 0;

  // Same time in each position, and places -> Treble Place
  if(j) {
    if(place)
      return cl | M_TREBLE_PLACE;
    else { // Treble Dodging
      // Look at internal places
      int y = 0, n = 0;
      int step=length() / (tmax-tmin+1);
      for(i = step-1; i < length()/2 - 1; i += step) {
	if((*this)[i].internal()) y = 1; else n = 1;
	if((*this)[length()-i-2].internal()) y = 1; else n = 1;
      }
      if(!y & n) return cl | M_TREBLE_BOB;
      if(y & !n) return cl | M_SURPRISE;
      return cl | M_DELIGHT;
    }
  } else // Must be Alliance.
    return cl | M_ALLIANCE;
}

// stagename : Return the name of a stage
const char *method::stagename(int n)
{
  static char buff[4];
  if(n >= 3 && n <= 12)
    return txt_stages[n-3];
  else {
    sprintf(buff,"%d",n);
    return buff;
  }
}

// fullname : Return the full name of the method
char *method::fullname(char *c) const
{
  *c = 0;
  int cl = methclass();
  strcat(c,myname.c_str());
  strcat(c," ");
  if(cl & M_LITTLE) {
    strcat(c,txt_little);
    strcat(c," ");
  }
  strcat(c,classname(cl));
  if(c[strlen(c)-1] != ' ') strcat(c," ");
  strcat(c,stagename(bells()));
  return c;
}

// Return the lead head code for this method
char *method::lhcode(void) const
{
  static char buff[32];
  int lhplace = 0;

  buff[0] = 'z'; buff[1] = '\0';
  if(!isregular()) return buff;

  // For a normal lead head code to be used, the method must be regular,
  // and have either one or two hunt bells, treble and 2.
  row lh = asrow();
  if(lh[0] != 0 || lh[2] == 2)	// Treble not a hunt bell
    return buff;		// or more than 2 hunt bells
  int n = lh.ispblh();
  if(n == 0) return buff;	// Not regular
  int o;

  if(lh[1] == 1) {		// Two hunt bells
    if((*this)[0] == change(bells(), "X"))
      lhplace = 1;
    else if((*this)[0] == change(bells(), "3"))
      lhplace = 2;
    o = bells()-2;		// Order of the lead head
    n = o - n;
  } else {			// One hunt bell
    if((*this)[length()-1] == change(bells(), "1"))
      lhplace = 1;
    else if((*this)[length()-1] == change(bells(), "12"))
      lhplace = 2;
    o = bells()-1;
  }

  // We also need the lead end place to be 1 or 12 (single hunt)
  // or the first place to be 1 or 3 (twin hunt).
  if(lhplace == 0) return buff;

  // n must be coprime to o, otherwise it's not a proper method
  if(hcf(n,o) > 1) return buff;

  // Now if the order is odd, we're in the a-f, g-m group
  // otherwise we're in the pqrs group

  int m = n;
  if(m > o/2) m = o - m;
  int p = 1;

  // Work out what the letter needs to be (modulo stuff)
  if(o & 1) {
    p = 3;
    if(m <= 3)
      buff[0] = 'a' + m - 1;
    else
      buff[0] = 'c';
  } else
    buff[0] = 'p';

  // Do we need a number too?
  if(m > p) {			// Yes
    // Now this is the way I _think_ it works:  we count upwards,
    // but miss out all those which don't give a proper method, i.e.
    // which aren't coprime to o.
    int i, j=0;
    for(i = p+1; i <= m; i++)
      if(hcf(i,o) == 1) j++;
    sprintf(buff+1, "%d", j);
  }

  // Turn a-c into d-f, p into q if we need to
  if(m != n) {
    if(o & 1)
      buff[0] = 'a' + 'f' - buff[0];
    else
      buff[0] = 'q';
  }

  // Turn a-f into g-m, p-q into r-s if we need to
  if(lhplace == 1) {
    if(o & 1) {
      buff[0] += 'g' - 'a';
      if(buff[0] >= 'i') buff[0]++; // Miss out i
    } else
      buff[0] += 'r' - 'p';
  }
  
  return buff;
}
