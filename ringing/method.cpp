// method.cpp - routines for methods, positions and calls
// Copyright (C) 2001 Martin Bright <martin@boojum.org.uk>

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

#include <ringing/common.h>

#ifdef RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_C_INCLUDES
#include <stdio.h>
#else
#include <cstdio>
#endif
#if RINGING_OLD_INCLUDES
#include <bvector.h>
#endif
#include <ringing/method.h>

RINGING_USING_STD

RINGING_START_NAMESPACE

// *********************************************************************    
// *                    Functions for class method                     *
// *********************************************************************

// A few static strings

const char *method::txt_classes[12] = {
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

const char *method::txt_stages[20] = {
  "Singles",
  "Minimus",
  "Doubles",
  "Minor",
  "Triples",
  "Major",
  "Caters",
  "Royal",
  "Cinques",
  "Maximus",
  "Sextuples",
  "Fourteen",
  "Septuples",
  "Sixteen",
  "Octuples",
  "Eighteen",
  "Nonuples",
  "Twenty",
  "Decuples",
  "Twenty-Two"
  };

const char *method::txt_differential = "Differential";
const char *method::txt_double       = "Double";
const char *method::txt_little       = "Little";

string method::classname(int cl)
{
  string s;      //  12345678901234567890123456789012
  s.reserve(35); // "Differential Little Treble Place" is the longest class
  if ( cl & M_DIFFERENTIAL ) {
    s += txt_differential;
  }
  if ( cl & M_LITTLE ) {
    if (s.size()) s += " ";
    s += txt_little;
  }
  if ( txt_classes[cl & M_MASK][0] ) {
    if (s.size()) s += " ";
    s += txt_classes[cl & M_MASK];
  }
  return s;
}

// issym : Find out whether the method is symmetrical
// This means symmetrical, not counting the half-lead or lead end.
bool method::issym(void) const
{
  if(length() & 1) return 0;	// Must have an even length
  for(int i = 0; i < length() / 2 - 1; i++)
    if((*this)[i] != (*this)[length() - i - 2]) return false;
  return true;
}

// isdouble : Find out whether the method is double
// This doesn't necessarily mean it's symmetrical
// (e.g. Double Oxford Bob Triples)
bool method::isdouble(void) const
{
  if(length() & 1) return 0;	// Must have even length
  for(int i = 0; i < length() / 2; i++)
    if((*this)[i] != (*this)[length() / 2 + i].reverse()) return false;
  return true;
}

// huntbells : Find the number of hunt bells
int method::huntbells(void) const
{
  int n = 0;
  row r = lh();
  for(int i = 0; i < bells(); i++)
    if(r[i] == i) n++;
  return n;
}

// issym : Is this bell's path symmetrical?
bool method::issym(bell b) const
{
  if(length() & 1) return 0;	// Must have even length
  for(int i = 0; i < length() / 2 - 1; i++) {
    if((*this)[i].findswap(b-1) != (*this)[length() - i - 2].findswap(b-1))
      return false;
    if((*this)[i].findswap(b) != (*this)[length() - i - 2].findswap(b))
      return false;
    b *= (*this)[i];
  }
  if(b * (*this)[length()/2-1] != b) // Must make a place at the half lead
    return false;
  return true;
}

// isplain : Does this bell plain hunt?
bool method::isplain(bell b) const
{
  int j = b;
  int dir = j & 1;		// Odd bells out, even bells in
  int dirchanges = 0;		// Count changes of direction
  for(int i = 0; i < length(); i++) {
    if((*this)[i].findswap(j-1+dir)) return false; // No, that's a dodge.
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
bool method::hasdodges(bell b) const
{
  int i;
  bell j = b, j1 = -1, j2 = -1;
  for(i = 0; i < length(); i++) {
    j2 = j1; j1 = j;
    j *= (*this)[i];
    if(j == j2 && j != j1) return true;
  }
  return false;
}

// hasplaces : Does this bell make any internal places?
bool method::hasplaces(bell b) const
{
  int i;
  bell j = b, j1;
  for(i = 0; i < length(); i++) {
    j1 = j;
    j *= (*this)[i];
    if(j == j1 && j > 0 && j < bells()-1)
      return true;
  }
  return false;
}
    
// meth_class : What class of method is it?
int method::methclass(void) const
{
  int cl = 0;
  int i;
  bell j, hb;

  // Find the first hunt bell
  row lhr = lh();
  for(hb = 0; lhr[hb] != hb && hb < bells(); hb = hb + 1);
  
  // Find the size of the first set of working bells    
  int wb=0;
  for(j = 0; lhr[j] == j && j < bells(); j=j+1);
  if ( j < bells() ) {
    i = j;
    do {
      i = lhr[i];
      ++wb;
    } while (i != j);
  }

  if(hb == bells()) {
    // It's a non-hunter.  Is it a principle?
    if (wb == bells())
      return cl | M_PRINCIPLE;
    else      
      return cl | M_DIFFERENTIAL;
  }
  else if (wb < bells()-huntbells())
    cl |= M_DIFFERENTIAL;

  // Find how far the treble ever gets
  bell tmin = hb, tmax = hb;
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
    if(hb == 0 && lhr[1] == 1 && issym(1))
      return cl | M_SLOW_COURSE;
    // Now see whether any of the working bells make any dodges
    for(i = 1; i < bells(); i++)
      if(hasdodges(i)) return cl | M_BOB;
    return cl | M_PLACE;
  }
  
  // Now count the number of blows in each position in the half lead,
  // and also note whether the treble makes any places.
  int place = 0;
  bell k;
  j = hb;
  vector<char> count(tmax-tmin+1);
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
  if ((n >= 3) && (n <= 22))
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
  strcat(c, fullname().c_str());
  return c;
}

string method::fullname() const
{
  string result = myname;
  int cl = methclass();
  if (!myname.empty()) result += ' ';

  if (myname == "Grandsire" || myname == "Reverse Grandsire"
      || myname == "Double Grandsire" || myname == "Little Grandsire" )
    ; // Grandsire and it's related methods do not contain either a class
      // or a Little modifier.
  else if ( bells()%2 && ( myname == "Union" || myname == "Double Union" 
			   || myname == "Reverse Union" ) )
    ; // Nor does Union ...
  else
    {
      const string cn( classname(cl) );
      result += cn;
      if (cn.size()) result += ' ';
    }
  result += stagename(bells());
  return result;
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
  row lhr = lh();
  if(lhr[0] != 0 || lhr[2] == 2)	// Treble not a hunt bell
    return buff;		// or more than 2 hunt bells
  int n = lhr.ispblh();
  if(n == 0) return buff;	// Not regular
  int o;

  if(lhr[1] == 1) {		// Two hunt bells
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
  if(m > p)
    sprintf(buff+1, "%d", m-p);

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

RINGING_END_NAMESPACE
