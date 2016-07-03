// method.cpp - routines for methods, positions and calls
// Copyright (C) 2001, 2004, 2008, 2010, 2011, 2014
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_C_INCLUDES
#include <stdio.h>
#include <string.h>
#include <assert.h>
#else
#include <cstdio>
#include <cstring>
#include <cassert>
#endif
#if RINGING_OLD_INCLUDES
#include <bvector.h>
#endif

#include <ringing/change.h>
#include <ringing/mathutils.h>
#include <ringing/method.h>
#include <ringing/place_notation.h>
#include <ringing/row.h>
#include <iterator>

#ifdef _MSC_VER
// Microsoft have unilaterally deprecated snprintf in favour of a non-standard
// extension, snprintf_s.  4996 is the warning about it being deprecated.
// (This is also true of strcat, though in that case the warning is to some 
// extent justifiablable.)
#pragma warning (disable: 4996)
#define snprintf _snprintf
#endif

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
  "Twenty-two"
};

const char *method::txt_differential = "Differential";
const char *method::txt_double       = "Double";
const char *method::txt_little       = "Little";

method::method(const char *pn, int b, const char *n) 
  : b(b) 
{
  name(n);
  interpret_pn(b, pn, pn + strlen(pn),
               back_insert_iterator<vector<change> >(*this));
}

method::method(const string& pn, int b, const string& n) 
  : b(b) 
{
  name(n);
  interpret_pn(b, pn.begin(), pn.end(),
               back_insert_iterator<vector<change> >(*this));
}

row method::lh() const
{
  vector<change>::const_iterator i;
  row r(bells());
  for(i=begin(); i != end(); i++) r *= *i;
  return r;
}

int method::leads(void) const
{ 
  return lh().order(); 
}

bool method::isregular(void) const
{ 
  return lh().ispblh(); 
}

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

bool method::is_palindromic_about(int i) const
{
  const int n( size() );
  for ( int j=1; j<(n%2==0 ? n/2 : n/2+1); ++j ) 
    if ( (*this)[(i+j) % n] != (*this)[(i-j+n) % n] )
      return false;

  return true;
}

// issym : Find out whether the method is symmetrical
// This means symmetrical, not counting the half-lead or lead end.
bool method::issym(void) const
{
  const int n( size() );
  if(n & 1) return 0;	// Must have an even length
  return is_palindromic_about(n/2-1);
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

bool method::is_palindromic_about(bell b, int i) const
{
  const int n( size() );

  // Trace path of bell, b, to the prospective symmetry point, i
  for ( int j=0; j < (i+1) % n; ++j )
    b *= (*this)[j%n];

  // Does bell b make a place at the symmetry point?
  if(b * (*this)[i] != b) 
    return false;

  // Iterate outwards in both directions from the symmetry point
  for ( int j=1; j<(n%2==0 ? n/2 : n/2+1); ++j ) {
    change const& c1 = (*this)[(i+j) % n];    
    change const& c2 = (*this)[(i-j+n) % n];
    // Do both changes affect bell b in the same way?
    bell b1 = b * c1,  b2 = b * c2; 
    if ( b1 != b2 ) return false;
    b = b1;
  }

  // We're now at the other symmetry point.  Is there a place there too?
  if(b * (*this)[i+(n%2==0 ? n/2 : n/2+1)] != b) 
    return false;

  return true;
}

// issym : Is this bell's path symmetrical?
bool method::issym(bell b) const
{
  const int n( size() );
  if(n & 1) return 0;	// Must have even length
  return is_palindromic_about(b, n/2-1);
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
  for(hb = 0; hb < bells() && lhr[hb] != hb; ++hb);
  
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
      return cl | M_PRINCIPLE | M_DIFFERENTIAL;
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

  // Is the treble path palindromic about two changes?
  if (length() % 2) return cl | M_HYBRID;  
  int sym_point = symmetry_point(hb);
  if(sym_point == -1) return cl | M_HYBRID;
 
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
  for(i = 0; i<=sym_point; ++i) j *= (*this)[i];
  for(i = sym_point+1;i <= sym_point+length()/2-1; i++) {
    count[j-tmin]++;
    k = j;
    j *= (*this)[i%length()];
    if(j == k)
      place = 1;		// Treble contains places
  }
  count[j-tmin]++;

  // Go through and see whether we spent the same amount of time
  // in each position
  j = 1;
  for(i = tmin+1;i <= tmax;i++)
    if(count[i-tmin] != count[i-1-tmin]) j = 0;

  // Same time in each position, and places -> Treble Place
  if(j) {
    if(place)
      return cl | M_TREBLE_PLACE;
    else { // Treble Dodging
      // Treble confined to two places -> Treble Bob
      if(tmax - tmin == 1) return cl | M_TREBLE_BOB;
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
  static const size_t bufsize = 8;
  static char buff[bufsize];
  if ((n >= 3) && (n <= 22))
    return txt_stages[n-3];
  else {
    snprintf(buff,bufsize,"%d",n);
    return buff;
  }
}

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
// fullname : Return the full name of the method
char *method::fullname(char *c) const
{
  *c = 0;
  strcat(c, fullname().c_str());
  return c;
}
#endif

string method::fullname() const
{
  string result = myname;
  int cl = methclass();
  if (!myname.empty()) result += ' ';

  if (myname == "Grandsire" || myname == "Reverse Grandsire"
      || myname == "Double Grandsire" || myname == "Little Grandsire" )
    ; // Grandsire and its related methods do not contain either a class
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
  static const size_t bufsize = 32;
  static char buff[bufsize];
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
    snprintf(buff+1, bufsize-1, "%d", m-p);

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


int method::symmetry_point() const
{
  const int n( size() );
  assert( n%2 == 0 );
  for ( int i=0; i < (n%2==0 ? n/2 : n); ++i )
    // Try m[i] as the sym point
    if (is_palindromic_about(i))
      return i;
  return -1;
}

int method::symmetry_point(bell b) const
{
  const int n( size() );
  assert( n%2 == 0 );
  for ( int i=0; i < (n%2==0 ? n/2 : n); ++i )
    // Try m[i] as the sym point
    if (is_palindromic_about(b, i))
      return i;
  return -1;
}

RINGING_START_ANON_NAMESPACE

void do_single_compressed_pn( string &out, const change &ch, 
			      int flags, bool &might_need_dot, bool &first )
{
  const int n = ch.bells();
  if ( ch.count_places() == 0 )
    {
      if ( (flags & method::M_DOTS) && !first ) out += '.';

      out += (flags & method::M_UCROSS) ? 'X' 
	   : (flags & method::M_LCROSS) ? 'x' 
	   : (flags & method::M_DASH)   ? '-'
	   : 'X'; // Default is same as in change::print()

      might_need_dot = false;
    } 
  else 
    {
      string p( ch.print() );
      
      if ( !(flags & method::M_EXTERNAL) && p.size() > 1 )
	{
	  if ( p[0] == bell(0).to_char() )
	    p = p.substr(1);
	  if ( !p.empty() && p[ p.size()-1 ] == bell( n-1 ).to_char() )
	    p = p.substr( 0, p.size() - 1 );
	  if ( p.empty() )
	    p = bell( 0 ).to_char();
	}

      if ( ( might_need_dot || (flags & method::M_DOTS) ) && !first ) 
	out += '.';
      out += p;
      might_need_dot = true;
    }
  first = false;
}

void do_compressed_section( string& out, method const& m, 
                            unsigned int begin, unsigned int end,
                            int flags )
{
  bool might_need_dot(false), first(true);
  for ( unsigned int i=begin; i<end; ++i )
    do_single_compressed_pn( out, m[i], flags, might_need_dot, first );
}

RINGING_END_ANON_NAMESPACE

string method::format( int flags ) const
{
  string out;

  int sym = size() / 2 - 1;
  if ( (flags & M_SYMMETRY) && size() > 2 
       && ( issym() || (flags & M_FULL_SYMMETRY) 
                       && !(flags & M_OMIT_LH)
                       && (sym = symmetry_point()) != -1 ) ) {
    if (sym != 0) out += '&';
    else if (flags & M_PLUS) out += '+';
    do_compressed_section( out, *this, 0, sym+1, flags );
    if (!(flags & M_OMIT_LH)) {
      out += ',';
      if (sym != size()/2 - 1) out += '&';
      else if (flags & M_PLUS) out += '+';
      do_compressed_section( out, *this, 2*sym+1, size()/2+sym+1, flags );
    }
  } else {
    if (flags & M_PLUS) out += '+';
    do_compressed_section( out, *this, 0, 
                           size() - ( (flags & M_OMIT_LH) ? 1 : 0 ),  flags );
  }

  return out;
}

// Counts the maximum number of consecutive blows in one place
int method::maxblows(void) const
{
  int maxn = 0;

  for ( const_iterator b(begin()), i(b), e(end()); i != e; ++i )
    for ( int p(0); p < bells(); ++p )
      if ( i->findplace(p) )
	{
	  // It's part of a longer run.
	  if ( i != b && (i-1)->findplace(p) )
	    continue;

	  // Find the size of this run of blows
	  int n=2;
	  {
	    for ( const_iterator j(i); j != e; ++j )
	      if ( j->findplace(p) )
		++n;
	      else
		goto end_run;
	  }

	  {
	    // And handle runs that continue across the lead-end
	    for ( const_iterator j(b); j != i; ++j )
	      if ( j->findplace(p) )
		++n;
	      else
		goto end_run;
	  }

	end_run:
	  if ( n > maxn )
	    maxn = n;
	}

  return maxn - 1;
}

bool method::ispalindromic() const
{
  return symmetry_point() != -1;
}

bool method::ispalindromic(bell b) const
{
  return symmetry_point(b) != -1;
}

RINGING_END_NAMESPACE
