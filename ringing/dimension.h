// -*- C++ -*- dimension.h - a class representing a length
// Copyright (C) 2001, 2022 Martin Bright <martin@boojum.org.uk>
// and Richard Smith <richard@ex-parrot.com>

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

#ifndef RINGING_DIMENSION_H
#define RINGING_DIMENSION_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <string>
#include <iostream>
#include <map>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class RINGING_API dimension : private RINGING_DETAILS_PREFIX safe_bool {
public:
  enum units {
    points, inches, cm, mm
  };

  int n, d;
  units u;

  class bad_format {};

  dimension() : n(0), d(1), u(points) {}
  dimension(int n) : n(n), d(1), u(points) {}
  dimension(int n, int d, units u) : n(n), d(d), u(u) {}
  
  string& write(string& s) const;
  void read(const char *s);
  void read(const string& s) { read(s.c_str()); }
  void reduce();
  float in_points() const { return to_points[u] * n / d; }
  void set_float(float value, int denom, units uu = points);

  // Safe boolean conversions
  operator safe_bool_t() const { return make_safe_bool(n != 0); }
  bool operator!() const { return !bool( *this ); }


  dimension& operator*=(int i) { n *= i; reduce(); return *this; }
  dimension operator*(int i) const { dimension d(*this); d *= i; return d; }
  dimension& operator/=(int i) { d *= i; reduce(); return *this; }
  dimension operator/(int i) const { dimension d(*this); d /= i; return d; }
  dimension operator-() const { dimension d(*this); d.n = -d.n; return d; }

private:
  class unit_names_map : public map<string, units> {
  public:
    unit_names_map();
  };

  static const unit_names_map unit_names;
  static const string unit_strings[];
  static const float to_points[];

  void read_units(const char *s);

  friend RINGING_API ostream& operator<<(ostream&, const dimension&);
};

ostream& operator<<(ostream& o, const dimension& d);

RINGING_END_NAMESPACE

#endif
