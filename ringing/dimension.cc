// dimension.cc

#ifdef __GNUG__
#pragma implementation
#endif

#include <strstream.h>
#include "dimension.h"
#include <ctype.h>

const dimension::unit_names_entry dimension::unit_names[] = {
  {"pt", points},
  {"points", points},
  {"point", points},
  {"in", inches},
  {"\"", inches},
  {"inches", inches},
  {"inch", inches},
  {"cm", cm},
  {"mm", mm}
};
const int dimension::unit_names_size = 9;
const char *dimension::unit_strings[] = {"pt", "in", "cm", "mm"};
const float dimension::to_points[] = {1, 72, 72/2.54, 72/25.4};

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
  char *t; int i;
  for(i = 0; i < unit_names_size; i++) {
    for(t = unit_names[i].s; 
	*t != '\0' && *s != '\0' && *t == tolower(*s);
	s++, t++);
    if(*t == '\0') { u = unit_names[i].u; return; }
  }
  throw bad_format();
}
