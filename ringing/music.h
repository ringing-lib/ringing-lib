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
 * Description     :
 *    This class provides analysis for music within a set of rows.
 * The analysis is done when the constructor is called, or when
 * change_rows is called. In whichever case, it always resets the
 * totals.
 *
 * The individual rows are not copied into this object.
 *
 * Results can either be obtained from an individual function or in
 * a string according to the following order:
 *
 * qktr,1,2,3,4,5,6,7,8,9
 *
 * Letters are displayed only when the following music is in the
 * touch:
 *  q = queens
 *  k = kings
 *  t = titums
 *  r = reverse rounds
 *
 * Otherwise numbers provide the following information
 *  1 = Number of 3 bell rollups in the touch (i.e.  678 on the back)
 *  2 = Number of 4 bell rollups in the touch (i.e. 5678 on the back)
 *  3 = Number of reverse 3 bell rollups off the front (i.e. 654 or 876)
 *  4 = Number of reverse 4 bell rollups off the front (i.e. 6543 or 8765)
 *  5 = Number of 46s or 68s etc at handstroke (note: this also adds in
 *      total for item 7).
 *  6 = Number of 46s or 68s etc at backstroke (note: this also adds in
 *      total for item 8).
 *  7 = Number of 246s or 468s etc at handstroke
 *  8 = Number of 246s or 468s etc at backstroke
 *  9 = Number of 65s or 87s at backstroke (will be zero for odd nos
 *      of bells)
 *
 * All of these should extend up to any number of bells.
 *
 * ***NOTE*** It assumes that the first row given is a handstroke row
 * therefore you need to bear this in mind when passing arguments in.
 ********************************************************************/

#ifndef RINGING_MUSIC_H
#define RINGING_MUSIC_H

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <iostream.h>
#else
#include <iostream>
#endif
#include <string>
#include <ringing/row.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

// Forward definition for music.
class music;

// Template definition for the output ostream << operator.
ostream& operator<< (ostream&, const music&);

// Main class definition
class music
{
 private:
  // Music items
  bool _queens;
  bool _titums;
  bool _kings;
  bool _reverse_rounds;
  int _t_tminus1_at_back;
  int _tminus2_t_at_the_back_hs;
  int _tminus4_tminus2_t_at_the_back_hs;
  int _tminus2_t_at_the_back_bs;
  int _tminus4_tminus2_t_at_the_back_bs;
  int _rollup_3;
  int _rollup_4;
  int _reverse_rollup_3;
  int _reverse_rollup_4;

  // The main processing function
  public:void process_row(const row&, const bool&);
  // Reset the music information
  private:void reset_music(void);
 
 public:
  music();                           // Default Constructor

  template <class RowIterator>       // Processing Constructor
  music(RowIterator first, RowIterator last, bool backstroke = false)
    { change_rows(first, last, backstroke);  }

  // Main Processing function
  template <class RowIterator>
  inline void change_rows(RowIterator first, RowIterator last, bool backstroke = false)
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

  // Output function
  friend ostream& operator<<(ostream&, const music&);

  // Functions for obtaining the results.
  bool queens(void) const { return _queens; }
  bool titums(void) const { return _titums; }
  bool kings(void) const { return _kings; }
  bool reverse_rounds(void) const { return _reverse_rounds; }
  int t_tminus1_at_back(void) const { return _t_tminus1_at_back; }
  int tminus2_t_at_the_back_hs(void) const { return _tminus2_t_at_the_back_hs; }
  int tminus4_tminus2_t_at_the_back_hs(void) const { return _tminus4_tminus2_t_at_the_back_hs; }
  int tminus2_t_at_the_back_bs(void) const { return _tminus2_t_at_the_back_bs; }
  int tminus4_tminus2_t_at_the_back_bs(void) const { return _tminus4_tminus2_t_at_the_back_bs; }
  int rollup_3(void) const { return _rollup_3; }
  int rollup_4(void) const { return _rollup_4; }
  int reverse_rollup_3(void) const { return _reverse_rollup_3; }
  int reverse_rollup_4(void) const { return _reverse_rollup_4; }
};

RINGING_END_NAMESPACE

#endif
