// -*- C++ -*- music.h - things to analyse music
// Copyright (C) 2002 Richard Smith <richard@ex-parrot.com>

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

#ifndef METHSEARCH_MUSIC_INCLUDED
#define METHSEARCH_MUSIC_INCLUDED

#ifdef RINGING_HAS_PRAGMA_INTERFACE
#pragma interface "methsearch/music"
#endif

#include <ringing/common.h>
#include <string>

// Forward declare ringing::method
RINGING_START_NAMESPACE
class method;
RINGING_END_NAMESPACE

RINGING_USING_NAMESPACE
RINGING_USING_STD

class musical_analysis
{
public:
  static void add_pattern( const string &str );
  static int analyse( const method &m );
  static void force_init( int bells );

private:
  class patterns;
  class analyser;
};

#endif // METHSEARCH_MUSIC_INCLUDED
