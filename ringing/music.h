/********************************************************************
 * File            : music.h
 * Last Modified by: Mark Banner
 * Last Modified   : 24/04/01
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
 * All of these currently extend up to 16 bells.
 *
 * ***NOTE*** It assumes that the first row given is a handstroke row
 * therefore you need to bear this in mind when passing arguments in.
 ********************************************************************/
#ifndef __MUSIC_H
#define __MUSIC_H

#ifdef __GNUG__
#pragma interface
#endif

#include <stl.h>
#include <math.h>
#include <string.h>
#include <iostream.h>
#include "method.h"
#include "mslib.h"

// Forward definition for music.
template <class RowIterator>
class music;

// Template definition for the output ostream << operator.
template <class RowIterator>
ostream& operator<< (ostream&, music<RowIterator>&);

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

  // Number of bells present
  int _nobells;

  // Various data items
  static const char _queens_string[][17];
  static const char _kings_string[][17];
  static const char _titums_string[][17];

  // The main processing function
  inline void process_row(const row&, const bool&);
  // Reset the music information
  inline void reset_music(void);

 public:
  music();                           // Default Constructor
  music(RowIterator, RowIterator);   // Processing Constructor
  
  inline void change_rows(RowIterator, RowIterator); // Main Processing function

  // Output function
  friend ostream& operator<< <>(ostream&, music&);

  // Functions for obtaining the results.
  bool queens(void) { return _queens; }
  bool titums(void) { return _titums; }
  bool kings(void) { return _kings; }
  bool reverse_rounds(void) { return _reverse_rounds; }
  int t_tminus1_at_back(void) { return _t_tminus1_at_back; }
  int tminus2_t_at_the_back_hs(void) { return _tminus2_t_at_the_back_hs; }
  int tminus4_tminus2_t_at_the_back_hs(void) { return _tminus4_tminus2_t_at_the_back_hs; }
  int tminus2_t_at_the_back_bs(void) { return _tminus2_t_at_the_back_bs; }
  int tminus4_tminus2_t_at_the_back_bs(void) { return _tminus4_tminus2_t_at_the_back_bs; }
  int rollup_3(void) { return _rollup_3; }
  int rollup_4(void) { return _rollup_4; }
  int reverse_rollup_3(void) { return _reverse_rollup_3; }
  int reverse_rollup_4(void) { return _reverse_rollup_4; }
};

// Various strings for checking for queens, kings and titmums
template <class RowIterator>
const char music<RowIterator>::_queens_string[][17] = {"13524",
						       "135246",
						       "1357246",
						       "13572468",
						       "135792468",
						       "1357924680",
						       "13579E24680",
						       "13579E24680T",
						       "13579EA24680T",
						       "13579EA24680TB",
						       "13579EAC24680TB",
						       "13579EAC24680TBD"};

template <class RowIterator>
const char music<RowIterator>::_kings_string[][17] = {"53124",
						      "531246",
						      "7531246",
						      "75312468",
						      "975312468",
						      "9753124680",
						      "E9753124680",
						      "E9753124680T",
						      "AE9753124680T",
						      "AE9753124680TB",
						      "CAE9753124680TB",
						      "CAE9753124680TBD"};

template <class RowIterator>
const char music<RowIterator>::_titums_string[][17] = {"14253",
						       "142536",
						       "1526374",
						       "15263748",
						       "162738495",
						       "1627384950",
						       "172839405E6",
						       "172839405E6T",
						       "1829304E5T6A7",
						       "1829304E5T6A7B",
						       "19203E4T5A6B7C8",
						       "19203E4T5A6B7C8D"};

// default constructor.
template <class RowIterator>
music<RowIterator>::music()
{
  // Reset the music, and we don't know how many bells there are.
  reset_music();
  _nobells = 0;
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

  // Find out how many bells we have by looking at the first row.
  _nobells = (*first).bells();
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
  // Queens etc
  int half = _nobells / 2;
  half += (_nobells % 2 ? 1 : 0);
  int rr = 0;
  int fr = 0;
  int i = 0;
  
  for (i = 0; i < _nobells; i++)
    {
      if ((r[i] == _nobells - i - 1) && (rr == i))
	{
	  rr++;
	}
    }

  for (i = _nobells - 1; i >= 0; i--)
    {
      if ((r[i] == i) && (fr == _nobells - i - 1))
	{
	  fr++;
	}
    }


  if ((_nobells % 2 == 0) && (back))
    {
      // Check for 65s, 87s, 09s etc at back
      if ((r[_nobells - 1] == (_nobells - 1) - 1) &&
	  (r[_nobells - 2] == (_nobells - 1)))
	{
	  _t_tminus1_at_back++;
	}
    }
  if (_nobells % 2 == 0)
    {
      if ((r[_nobells - 1] == _nobells - 1) &&
	  (r[_nobells - 2] == _nobells - 3))
	{
	  // we have a 46 for example.
	  (back ? _tminus2_t_at_the_back_bs++ : _tminus2_t_at_the_back_hs++);

	  if (r[_nobells - 3] == _nobells - 5)
	    {
	      // we have a 246 for example.
	      (back ? _tminus4_tminus2_t_at_the_back_bs++ : _tminus4_tminus2_t_at_the_back_hs++);
	    }
	}
    }

  if (fr == 4)
    {
      _rollup_4++;
    }
  if (fr == 3)
    {
      _rollup_3++;
    }

  if (rr == _nobells)
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
  buffer row_string(16);
  if (strcmp(_queens_string[_nobells - 5], r.print(row_string)) == 0)
    {
      _queens = true;
    }
  if (strcmp(_kings_string[_nobells - 5],  r.print(row_string)) == 0)
    {
      _kings = true;
    }
  if (strcmp(_titums_string[_nobells - 5], r.print(row_string)) == 0)
    {
      _titums = true;
    }
}

// Output operator. Outputs the variables in a specific order according
// to the specification at the start of this file.
template <class RowIterator>
ostream& operator<< (ostream &o, music<RowIterator> &m)
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

#endif
