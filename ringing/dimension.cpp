// dimension.cpp
// Copyright (C) 2001-2 Martin Bright <martin@boojum.org.uk>

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <map.h>
#else
#include <iostream>
#include <map>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#else
#include <cctype>
#endif
#include <ringing/streamutils.h>
#include <ringing/dimension.h>

RINGING_USING_STD

RINGING_START_NAMESPACE

RINGING_START_ANON_NAMESPACE

class unit_names_map : public map<string, dimension::units> {
public:
  unit_names_map();
};

static const string unit_strings[] = {"pt", "in", "cm", "mm"};
static const float to_points[] = {1, 72, 72/2.54f, 72/25.4f};
static const unit_names_map unit_names = unit_names_map();

unit_names_map::unit_names_map()
{
  (*this)["pt"] = dimension::points;
  (*this)["points"] = dimension::points;
  (*this)["point"] = dimension::points;
  (*this)["in"] = dimension::inches;
  (*this)["\""] = dimension::inches;
  (*this)["inches"] = dimension::inches;
  (*this)["inch"] = dimension::inches;
  (*this)["cm"] = dimension::cm;
  (*this)["mm"] = dimension::mm;
}

RINGING_END_ANON_NAMESPACE

float dimension::in_points() const
{
  return to_points[u] * n / d;
}

void dimension::reduce()
{
  int a = n, b = d, r;
  while(b != 0) { r = a % b; a = b; b = r; }
  n /= a; d /= a;
  if(d < 0) { d = -d; n = -n; }
}

void dimension::set_float(float value, int denom, units uu)
{
  d = denom;
  n = static_cast<int>(value * d); u = uu;
  reduce();
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
  o << ' ' << unit_strings[d.u];
  return o;
}

string& dimension::write(string& s) const
{
  return s = make_string() << *this << ends;
}

#if RINGING_USE_EXCEPTIONS
#  define THROW_OR_RETURN(obj_to_throw, obj_to_return) throw obj_to_throw
#  define THROW_OR_RETURN_VOID(obj_to_throw) throw obj_to_throw
#else
#  define THROW_OR_RETURN(obj_to_throw, obj_to_return) return obj_to_return
#  define THROW_OR_RETURN_VOID(obj_to_throw) return
#endif

void dimension::read(const char *s)
{
  int a, b, c;
  bool negative = false;

  // Minus sign first
  while(isspace(*s)) s++;
  if(*s == '-') { negative = true; s++; }

  // Now a number
  while(isspace(*s)) s++;
  if(!isdigit(*s)) THROW_OR_RETURN_VOID( bad_format() );
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
      if(*s++ != '/') THROW_OR_RETURN_VOID( bad_format() );
      while(isspace(*s)) s++;
      if(!isdigit(*s)) THROW_OR_RETURN_VOID( bad_format() );
      c = *s++ - '0';
      while(isdigit(*s)) c = 10*c + (*s++ - '0');
      read_units(s);
      n = a * c + b; d = c;
      if(negative) n = -n;
    } else if(*s == '/') { // Just a fractional part
      s++;
      while(isspace(*s)) s++;
      if(!isdigit(*s)) THROW_OR_RETURN_VOID( bad_format() );
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
  const char *t = s; while(*t != '\0' && !isspace(*t)) t++;
  map<string, units>::const_iterator i = unit_names.find(string(s,t));
  if(i != unit_names.end())
    u = (*i).second;
  else
    THROW_OR_RETURN_VOID( bad_format() );
}

RINGING_END_NAMESPACE
