// -*- C++ -*- music.cpp - Musical Analysis
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include <ringing/music.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

// default constructor.
music::music()
{
  // Reset the music
  reset_music();
}

// reset_music - clears all the music information entries.
void music::reset_music(void)
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
void music::process_row(const row &r, const bool &back)
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
ostream& operator<< (ostream &o, const music &m)
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
