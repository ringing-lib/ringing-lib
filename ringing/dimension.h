// -*- C++ -*- dimension.h - a class representing a length
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

#ifndef RINGING_DIMENSION_H
#define RINGING_DIMENSION_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#include <string>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <map.h>
#else
#include <iostream>
#include <map>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

class dimension {
public:
  enum units {
    points, inches, cm, mm
  };

  int n, d;
  units u;

  class bad_format {};

  dimension() : n(0), d(1), u(points) {}
  dimension(int i) : n(i), d(1), u(points) {}
  dimension(int i, int j, units k) : n(i), d(j), u(k) {}
  
  string& write(string& s) const;
  void read(const char *s);
  void read(const string& s) { read(s.c_str()); }
  void reduce();
  float in_points() const { return to_points[u] * n / d; }

  bool operator==(int i) const { return (i == 0) && (n == 0); }
  bool operator!=(int i) const { return !(*this == i); }

  dimension& operator*=(int i) { n *= i; reduce(); return *this; }
  dimension operator*(int i) { dimension d = *this; d *= i; return d; }

private:
  class unit_names_map : public map<string, units> {
  public:
    unit_names_map();
  };
  static const unit_names_map unit_names;
  static const string unit_strings[];
  static const float to_points[];

  void read_units(const char *s);

  friend ostream& operator<<(ostream&, const dimension&);
};

ostream& operator<<(ostream& o, const dimension& d);

RINGING_END_NAMESPACE

#endif
