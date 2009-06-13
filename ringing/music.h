// -*- C++ -*- music.h - Musical Analysis
// Copyright (C) 2001, 2008, 2009 Mark Banner <mark@standard8.co.uk> and
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
class RINGING_API music_details : private string
{
public:
  music_details(const string& pattern = "", int score = 1);
  music_details(const char* pattern, int score = 1);
 ~music_details();
  
  // Set the expression and score
  // From String
  bool set(const string& pattern, int score = 1);
  // From Character String
  bool set(const char* pattern, int score = 1);

  // Return the expression
  string get() const;

  // Return the number of possible matches
  unsigned int possible_matches(unsigned int bells) const;
  int possible_score(unsigned int bells) const;

  // Return the count
  unsigned int count(const EStroke& = eBoth) const;
  // Return the calculated score
  int total(const EStroke& = eBoth) const;

  // Return the uncalculated score
  int raw_score() const;

  friend class music;
  friend class music_node;

#if RINGING_USE_EXCEPTIONS
  struct invalid_regex : public invalid_argument {
    invalid_regex( string const& pat, string const& msg );
  };
#endif

  RINGING_FAKE_COMPARATORS(music_details)
  
private:
  // Clear the current counts
  void clear();
  // Which count to increment
  void increment(const EStroke& = eBackstroke);
  bool check_expression() const;
  void check_bells(unsigned int bells) const;
  unsigned int possible_matches(unsigned int bells, unsigned int pos, const string &s, int &q) const;
  
  unsigned int count_handstroke;
  unsigned int count_backstroke;
  int score;
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

  void push_back(const music_details&); // "Specify items" to end of vector
  //  void pop_back(); // Remove items from end of vector

  iterator begin(); // begining of the music_details vector
  const_iterator begin() const;
  iterator end(); // end of the music_details vector
  const_iterator end() const;

  size_type size() const; // number of music_details stored

  void set_bells(unsigned int b); // set the number of bells to match

  // Main Processing function
  template <class RowIterator>
  void process_rows(RowIterator first, RowIterator last, bool backstroke = false)
  {
    // reset row line information
    reset_music();
    
    // Go through each row, noting hand and back.
    for (; first != last; first++) 
      {
	process_row(*first, backstroke);
	backstroke = (backstroke ? false : true);
      }
  }

  // As above, but for a single row
  void process_row( row const& r, bool backstroke = false);

  // Get the total score - individual scores now obtained from accessing
  // the items within the music_details vector.
  int get_score(const EStroke& = eBoth);
  // Get the total matches.
  unsigned int get_count(const EStroke& = eBoth);

  // Total Possible Score
  int get_possible_score();

  // Reset the music information
  void reset_music(void);

private:
  // The music specification details
  mdvector info;
  // The tree containing the structure for matching rows
  cloning_pointer<music_node> top_node;

  unsigned int bells;
};

RINGING_END_NAMESPACE

#endif
