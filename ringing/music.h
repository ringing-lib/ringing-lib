// -*- C++ -*- music.h - Musical Analysis
// Copyright (C) 2001 Mark Banner <mark@standard8.co.uk>

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
 *    This class provides analysis for music within a set of rows.
 * The analysis is done when the constructor is called, or when
 * change_rows is called. In whichever case, it always resets the
 * totals.
 *
 * The individual rows are not copied into this object.
 ********************************************************************/

#ifndef RINGING_MUSIC_H
#define RINGING_MUSIC_H

#ifdef __GNUG__
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
class music_details : public string
{
public:
  music_details(const string& = "", const int& = 1);
  music_details(const char *, const int& = 1);
  ~music_details();
  
  // Set the expression and score
  // From String
  void Set(const string&, const int& = 1);
  // From Character String
  void Set(const char *, const int& = 1);

  // Return the expression
  string Get() const { return *this; }

  // Return the count
  unsigned int count(const EStroke& = eBoth) const;
  // Return the calculated score
  int total(const EStroke& = eBoth) const;

  // Return the uncalculated score
  int raw_score() const { return _score; }

  friend class music;
  friend class music_node;
private:
  // Clear the current counts
  void clear();
  // Which count to increment
  void increment(const EStroke& = eBackstroke);

  unsigned int _count_handstroke;
  unsigned int _count_backstroke;
  int _score;
};

class music_node
{
public:
  // bell -> node map
  typedef map<unsigned int, music_node*> BellNodeMap;
  typedef BellNodeMap::iterator BellNodeMapIterator;
  // Music Details that finish at this node
  typedef vector<unsigned int> DetailsVector;
  typedef DetailsVector::iterator DetailsVectorIterator;

  // Have to know how many bells there are
  music_node() { bells = 0; }
  music_node(const unsigned int &b = 0) { bells = b; }
  ~music_node();

  void set_bells(const unsigned int &b);

  void add(const music_details &md, const unsigned int &i, const unsigned int &key, const unsigned int &pos);

  void match(const row &r, const unsigned int &pos, vector<music_details> &results, const EStroke &stroke);

#if RINGING_USE_EXCEPTIONS
  struct memory_error : public overflow_error {
    memory_error();
  };
#endif
private:
  BellNodeMap subnodes;
  DetailsVector detailsmatch;
  unsigned int bells;

  void add_to_subtree(const unsigned int &place, const music_details &md, const unsigned int &i, const unsigned int &key, const unsigned int &pos, const bool &process_star);
};

// Main class definition
class music
{
public:
  typedef vector<music_details> mdvector;

  unsigned int specify_music(const music_details&);
  //  bool remove_music(const int&);

  music(const unsigned int &b = 0);         // Default Constructor

  void set_bells(const unsigned int &b);

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

  unsigned int Get_Results(const unsigned int&, const EStroke& = eBoth);
  int Get_Score(const unsigned int&, const EStroke& = eBoth);
  int Get_Score(const EStroke& = eBoth);

private:
  mdvector MusicInfo;
  music_node TopNode;
  int bells;

  // The main processing function
  void process_row(const row&, const bool&);

  // Reset the music information
  void reset_music(void);
};

RINGING_END_NAMESPACE

#endif
