// dimension.h
//
// This class represents a length with a dimension.
// The length is a rational number.

#ifndef METHLIB_DIMENSION_H
#define METHLIB_DIMENSION_H

#ifdef __GNUG__
#pragma interface
#endif

#include <string>
#include <iostream.h>

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
  struct unit_names_entry { char *s; units u; };
  static const unit_names_entry unit_names[];
  static const int unit_names_size;
  static const char *unit_strings[];
  static const float to_points[];

  void read_units(const char *s);

  friend ostream& operator<<(ostream&, const dimension&);
};

ostream& operator<<(ostream& o, const dimension& d);

#endif
