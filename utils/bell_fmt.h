// -*- C++ -*- bell_calc.h - class to syntax highlight a bell
// Copyright (C) 2008, 2009, 2010, 2011, 2021
// Richard Smith <richard@ex-parrot.com>

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

#ifndef UTILS_BELL_FMT_INCLUDED
#define UTILS_BELL_FMT_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <iosfwd>
#include <map>

#include <ringing/row.h>

RINGING_USING_NAMESPACE
RINGING_USING_STD

class bell_fmt {
public:
  void set_colours(string const& r, string const& g, string const& b);
  bool empty() const { return fmts.empty(); }

private:
  struct fmt {
    int colour;
    bool bold;
  };

  class do_fmt {
  public:
    do_fmt(ostream& os, bell_fmt const& bf) : os(&os), bf(&bf) {}

    ostream& operator<<(bell b) const;
    ostream& operator<<(row const& r) const;

  private:
    ostream* os;
    bell_fmt const* bf;
  };

  friend RINGING_API bell_fmt::do_fmt 
  operator<<(ostream& os, const bell_fmt& f) {
    return do_fmt(os, f);
  }

  void set_colour(string const& str, int colour);

  map<bell, fmt> fmts;
};

#endif // UTILS_BELL_FMT_INCLUDED
