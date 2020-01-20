// -*- C++ -*- music.h - Musical Analysis
// Copyright (C) 2001, 2008, 2009, 2010, 2011, 2020
// Mark Banner <mark@standard8.co.uk> and
// Richard Smith <richard@ex-parrot.com>.

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

/********************************************************************
 * Description:
 * The analysis is done when the constructor is called, or when
 * change_rows is called. In whichever case, it always resets the
 * totals.
 *
 * The individual rows are not copied into this object.
 ********************************************************************/

#ifndef RINGING_MUSIC_H
#define RINGING_MUSIC_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <iostream.h>
#include <map.h>
#include <stdexcept.h>
#else
#include <iostream>
#include <map>
#include <stdexcept>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/row_wildcard.h>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

class music;
class music_node;

enum EStroke
{
  eHandstroke,
  eBackstroke,
  eBoth
};

// Class to store details of the expressions and count.
class RINGING_API music_details
{
public:
  music_details(const string& pattern, int scoreh, int scoreb);
  music_details(const string& pattern = "", int score = 1);
 ~music_details();
  
  // Set the expression and score
  bool set(const string& pattern, int scoreh, int scoreb);
  bool set(const string& pattern, int score = 1);

  // Return the expression
  string const& get() const;

  // Return the maximum number of possible matches
  unsigned int possible_matches(unsigned int bells) const;
  int possible_score(unsigned int bells) const;

  // Return the number of matching rows
  unsigned int count(const EStroke& = eBoth) const;
  // Return the calculated score
  int total(const EStroke& = eBoth) const;

  // Get the bells that matched against a ? or * in the last pattern.
  vector<bell> last_wildcard_matches() const;

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
  // Return the uncalculated score
  int raw_score() const;
#endif

  friend class music;
  friend class music_node;

  typedef row_wildcard::invalid_pattern invalid_regex;

  RINGING_FAKE_COMPARATORS(music_details)
  
private:
  // Clear the current counts
  void clear();
  // Which count to increment
  void increment(const EStroke& = eBackstroke);
  bool check_bells(unsigned b) const;
  
  row_wildcard pat;
  unsigned int counth, countb;
  int scoreh, scoreb;
  vector<bell> last_wildcards;
};

// Main class definition
class RINGING_API music
{
public:
  // Various defintions for accessing the music_details structure.
  typedef vector<music_details> mdvector;
  typedef vector<music_details>::iterator iterator;
  typedef vector<music_details>::const_iterator const_iterator;
  typedef vector<music_details>::size_type size_type;

  music(unsigned int b = 0);         // Default Constructor
  music(unsigned int b, music_details const& md);

  void push_back(const music_details&); // "Specify items" to end of vector
  //  void pop_back(); // Remove items from end of vector

  iterator begin(); // begining of the music_details vector
  const_iterator begin() const;
  iterator end(); // end of the music_details vector
  const_iterator end() const;

  size_type size() const; // number of music_details stored

  void set_bells(unsigned int b);

  // Main Processing function
  template <class RowIterator>
  void process_rows(RowIterator first, RowIterator last, 
                    bool backstroke = false)
  {
    reset_music();
    
    for ( ; first != last; first++, backstroke = !backstroke) 
	process_row(*first, backstroke);
  }

  // As above, but for a single row.
  // Returns true if it matched a row.
  bool process_row( row const& r, bool backstroke = false);

  // Get the total score - individual scores now obtained from accessing
  // the items within the music_details vector.
  int get_score(const EStroke& = eBoth);
  // Get the total number of matching rows.
  unsigned int get_count(const EStroke& = eBoth);

  // Total Possible Score
  int get_possible_score();

  // Reset the music information
  void reset_music(void);

  unsigned int bells() const { return b; }
  
private:
  // The music specification details
  mdvector info;
  // The tree containing the structure for matching rows
  cloning_pointer<music_node> top_node;

  unsigned int b;
};

#if RINGING_USE_EXCEPTIONS
struct invalid_named_music : public invalid_argument {
  invalid_named_music( string const& n, string const& msg );
};
#endif

// These take a string such as "rounds" or "4-runs" and convert it to
// one or more pattern which is added to the music class.
RINGING_API void add_named_music( music& m, string const& n, int score = 1 );
RINGING_API void add_named_music( music& m, string const& n, int sh, int sb );

// Take a string of the form 
//
//   [ scoreh ["," scoreb ] ":" ] { "<" name ">" | pattern }
//
// and adds it to the music class.  If scoreb is omitted, it defaults
// to scoreh; if scoreh is omitted, it defaults to 1.  Scores must be 
// integers (base 10) with an optional +/- sign.  Named music is parsed
// using add_named_music.  Patterns are parsed using the music_details
// class.
RINGING_API void add_scored_music_string( music& m, string const& n );

RINGING_END_NAMESPACE

#endif
