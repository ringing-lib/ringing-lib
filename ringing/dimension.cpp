// dimension.cpp
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <strstream.h>
#else
#include <strstream>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#else
#include <cctype>
#endif
#include <ringing/dimension.h>

RINGING_USING_STD

RINGING_START_NAMESPACE

const string dimension::unit_strings[] = {"pt", "in", "cm", "mm"};
const float dimension::to_points[] = {1, 72, 72/2.54, 72/25.4};
const dimension::unit_names_map dimension::unit_names;

dimension::unit_names_map::unit_names_map()
{
  (*this)["pt"] = points;
  (*this)["points"] = points;
  (*this)["point"] = points;
  (*this)["in"] = inches;
  (*this)["\""] = inches;
  (*this)["inches"] = inches;
  (*this)["inch"] = inches;
  (*this)["cm"] = cm;
  (*this)["mm"] = mm;
}

void dimension::reduce()
{
  int a = n, b = d, r;
  while(b != 0) { r = a % b; a = b; b = r; }
  n /= a; d /= a;
  if(d < 0) { d = -d; n = -n; }
}

ostream& operator<<(ostream& o, const dimension& d)
{
  if(d.d == 0) return o;
  if(d.n == 0) { o << '0'; return o; }
  int r = d.n % d.d;
  int q = (d.n - r) / d.d;
  if(q != 0) o << q;
  if(q != 0 && r != 0) o << ' ';
  if(r < 0) r = -r;
  if(r != 0) o << r << '/' << d.d;
  o << ' ' << dimension::unit_strings[d.u];
  return o;
}

string& dimension::write(string& s) const
{
  ostrstream o;
  o << *this << ends;
  s = o.str(); o.freeze(0); return s;
}

void dimension::read(const char *s)
{
  int a, b, c;
  bool negative = false;

  // Minus sign first
  while(isspace(*s)) s++;
  if(*s == '-') { negative = true; s++; }

  // Now a number
  while(isspace(*s)) s++;
  if(!isdigit(*s)) throw bad_format();
  a = *s++ - '0';
  while(isdigit(*s)) a = 10*a + (*s++ - '0');
  if(*s == '.') { // We have a number in decimal format
    s++;
    b = 1;
    while(isdigit(*s)) { a = 10*a + (*s++ - '0'); b = 10*b; }
    read_units(s);
    n = negative ? -a : a; d = b;
  } else { // Look for a fractional part
    while(isspace(*s)) s++;
    if(isdigit(*s)) { // Found a fractional part
      b = *s++ - '0';
      while(isdigit(*s)) b = 10*b + (*s++ - '0');
      while(isspace(*s)) s++;
      if(*s++ != '/') throw bad_format();
      while(isspace(*s)) s++;
      if(!isdigit(*s)) throw bad_format();
      c = *s++ - '0';
      while(isdigit(*s)) c = 10*c + (*s++ - '0');
      read_units(s);
      n = a * c + b; d = c;
      if(negative) n = -n;
    } else if(*s == '/') { // Just a fractional part
      s++;
      while(isspace(*s)) s++;
      if(!isdigit(*s)) throw bad_format();
      c = *s++ - '0';
      while(isdigit(*s)) c = 10*c + (*s++ - '0');
      read_units(s);
      n = negative ? -a : a; d = c;     
    } else { // Just an integer
      read_units(s);
      n = negative ? -a : a; d = 1;
    }
  }
  reduce();
}

void dimension::read_units(const char *s)
{
  while(isspace(*s)) s++;
  const char *t = s; while(!isspace(*t)) t++;
  map<string, units>::const_iterator i = unit_names.find(string(s,t));
  if(i != unit_names.end())
    u = (*i).second;
  else
    throw bad_format();
}

RINGING_END_NAMESPACE
