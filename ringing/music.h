// music.h - Musical Analysis
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
#include RINGING_STD_HEADER(iostream)
#include <string>
#include RINGING_LOCAL_HEADER(row)
RINGING_USING_STD

RINGING_START_NAMESPACE

// Forward definition for music.
template <class RowIterator>
class music;

// Template definition for the output ostream << operator.
template <class RowIterator>
ostream& operator<< (ostream&, const music<RowIterator>&);

// Main class definition
template <class RowIterator>
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
  inline void process_row(const row&, const bool&);
  // Reset the music information
  inline void reset_music(void);

 public:
  music();                           // Default Constructor
  music(RowIterator, RowIterator);   // Processing Constructor

  // Main Processing function
  inline void change_rows(RowIterator, RowIterator);

  // Output function
  friend ostream& operator<< <>(ostream&, const music<RowIterator>&);

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

// default constructor.
template <class RowIterator>
music<RowIterator>::music()
{
  // Reset the music
  reset_music();
}

// Constructor - call change_rows to do the analysis straight away.
template <class RowIterator>
music<RowIterator>::music(RowIterator first, RowIterator last)
{
  change_rows(first, last);
}

// change_rows - reset music and analyse the new rows.
template <class RowIterator>
void music<RowIterator>::change_rows(RowIterator first, RowIterator last)
{
  RowIterator i;

  // reset row line information
  reset_music();

  bool back = false; // start at hand, false = hand.

  // Go through each row, noting hand and back.
  for (i = first; i < last; i++)
    {
    process_row(*i, back);
    back = (back ? false : true);
    }
}

// reset_music - clears all the music information entries.
template <class RowIterator>
void music<RowIterator>::reset_music(void)
{
  // Find the music
  _queens = false;
  _titums = false;
  _kings = false;
  _t_tminus1_at_back = 0;
  _tminus2_t_at_the_back_hs = 0;
  _tminus4_tminus2_t_at_the_back_hs = 0;
  _tminus2_t_at_the_back_bs = 0;
  _tminus4_tminus2_t_at_the_back_bs = 0;
  _reverse_rounds = false;
  _rollup_3 = 0;
  _rollup_4 = 0;
  _reverse_rollup_3 = 0;
  _reverse_rollup_4 = 0;
}

// process_row - works out if a certain row is considered musical,
// and increments or changes the appriopriate variable.
template <class RowIterator>
void music<RowIterator>::process_row(const row &r, const bool &back)
{
  // Find out how many bells we have by looking at the first row.
  int nobells = r.bells();

  int i = 0;
  // For queens, titums, kings etc.
  int half = nobells / 2;
  half += (nobells % 2 ? 1 : 0);
  // rr for examining for reverse rollups off the front.
  int rr = 0;
  // fr for examining for rollups on the back.
  int fr = 0;
  // q for examining for queens
  int q = 0;
  // k for examining for kings
  int k = 0;
  // t for examining for titums
  int t = 0;
  
  // Some algorithms to work out for ANY number of bells.
  // First we look at queens, kings, titums and reverse rollups/rounds.
  for (i = 0; i < nobells; i++)
    {
      if ((r[i] == nobells - i - 1) && (rr == i))
	{
	  rr++;
	}
      t += (i % 2 == 0 ? (r[i] == i / 2) : (r[i] == (i / 2) + half)); 
      if (i < half)
	{
	  if (r[i] == (i * 2))
	    {
	      q++;
	    }
	  if (r[i] == (half - i - 1) * 2)
	    {
	      k++;
	    }
	}
      else
	{
	  if (r[i] == ((i - half + 1) * 2) - 1)
	    {
	      // we can increment kings AND queens here.
	      q++;
	      k++;
	    }
	}
    }

  // Now we check for rollups on the back.
  for (i = nobells - 1; i >= 0; i--)
    {
      if ((r[i] == i) && (fr == nobells - i - 1))
	{
	  fr++;
	}
    }

  if ((nobells % 2 == 0) && (back))
    {
      // Check for 65s, 87s, 09s etc at back
      if ((r[nobells - 1] == (nobells - 1) - 1) &&
	  (r[nobells - 2] == (nobells - 1)))
	{
	  _t_tminus1_at_back++;
	}
    }

  // Now look for 246s, 46s etc.
  if (nobells % 2 == 0)
    {
      if ((r[nobells - 1] == nobells - 1) &&
	  (r[nobells - 2] == nobells - 3))
	{
	  // we have a 46 for example.
	  (back ? _tminus2_t_at_the_back_bs++ : _tminus2_t_at_the_back_hs++);

	  if (r[nobells - 3] == nobells - 5)
	    {
	      // we have a 246 for example.
	      (back ? _tminus4_tminus2_t_at_the_back_bs++ : _tminus4_tminus2_t_at_the_back_hs++);
	    }
	}
    }

  // Now find out excatly what we have.
  if (fr == 4)
    {
      _rollup_4++;
    }
  if (fr == 3)
    {
      _rollup_3++;
    }

  if (rr == nobells)
    {
      _reverse_rounds = true;
    }
  if (rr == 4)
    {
      _reverse_rollup_4++;
    }
  if (rr == 3)
    {
      _reverse_rollup_3++;
    }
  if (q == nobells)
    {
      _queens = true;
    }
  if (k == nobells)
    {
      _kings = true;
    }
  if (t == nobells)
    {
      _titums = true;
    }
}

// Output operator. Outputs the variables in a specific order according
// to the specification at the start of this file.
template <class RowIterator>
ostream& operator<< (ostream &o, const music<RowIterator> &m)
{
  o << (m._queens         ? "q" : " ");
  o << (m._kings          ? "k" : " ");
  o << (m._titums         ? "t" : " ");
  o << (m._reverse_rounds ? "r" : " ");
  o << "," << m._rollup_3;
  o << "," << m._rollup_4;
  o << "," << m._reverse_rollup_3;
  o << "," << m._reverse_rollup_4;
  o << "," << m._tminus2_t_at_the_back_hs;
  o << "," << m._tminus2_t_at_the_back_bs;
  o << "," << m._tminus4_tminus2_t_at_the_back_hs;
  o << "," << m._tminus4_tminus2_t_at_the_back_bs;      
  o << "," << m._t_tminus1_at_back;
  return o;
}

RINGING_END_NAMESPACE

#endif
