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

// ********************************************************
// function definitions for MUSIC_DETAILS
// ********************************************************

music_details::music_details(const string &e, const int &s) : string(e)
{
  _score = s;
  _count_handstroke = 0;
  _count_backstroke = 0;
}

music_details::music_details(const char *e, const int &s) : string(e)
{
  _score = s;
  _count_handstroke = 0;
  _count_backstroke = 0;
}

music_details::~music_details()
{
}

void music_details::Set(const string &e, const int &s)
{
  *this = e;
  _score = s;
  // Should reset this here as we have changed the expression
  _count_handstroke = 0;
  _count_backstroke = 0;
}

void music_details::Set(const char *e, const int &s)
{
  *this = (string) e;
  _score = s;
  // Should reset this here as we have changed the expression
  _count_handstroke = 0;
  _count_backstroke = 0;
}

// Return the count
unsigned int music_details::count(const EStroke &stroke) const
{
  switch (stroke)
    {
    case eHandstroke:
      return _count_handstroke;
      break;
    case eBackstroke:
      return _count_backstroke;
      break;
    case eBoth:
      return _count_handstroke + _count_backstroke;
      break;
    default:
      return 0;
    }
  return 0;
}

// Return the calculated score
int music_details::total(const EStroke &stroke) const
{
  return count(stroke) * _score;
}

// Clear the Current counts
void music_details::clear()
{
  _count_backstroke = 0;
  _count_handstroke = 0;
}

// Which count to increment
void music_details::increment(const EStroke &stroke)
{
  if (stroke == eBackstroke)
    {
      _count_backstroke++;
    }
  else if (stroke == eHandstroke)
    {
      _count_handstroke++;
    }
}

// ********************************************************
// function definitions for MUSIC_NODE
// ********************************************************

void music_node::set_bells(const unsigned int &b)
{
  bells = b;
  // for each subnode
  BellNodeMap::iterator i;
  for (i = subnodes.begin(); i != subnodes.end(); i++)
    {
      i->second.set_bells(b);
    }
}

void music_node::add(const music_details &md, const unsigned int &i, const unsigned int &key)
{
  // Does this item end here?
  if (i >= md.size())
    {
      detailsmatch.push_back(key);
    }
  else
    {
      bell b;
      bool isbell = true;
      try
	{
	  b.from_char(md[i]);
	}
      catch (exception &e)
	{
	  isbell = false;
	}
      if (isbell)
	{
	  add_to_subtree(b + 1, md, i, key);
	}
      else
	{
	  if (md[i] == '?')
	    {
	      add_to_subtree(0, md, i, key);
	    }
	}
    }
}

void music_node::match(const row &r, const unsigned int &pos, vector<music_details> &results, const EStroke &stroke)
{
  DetailsVectorIterator i;
  for (i = detailsmatch.begin(); i < detailsmatch.end(); i++)
    {
      results[*i].increment(stroke);
    }
  // Try all against ?
  BellNodeMapIterator j = subnodes.find(0);
  if (j != subnodes.end())
    {
      j->second.match(r, pos + 1, results, stroke);
    }
  // Now try the actual number
  j = subnodes.find(r[pos] + 1);
  if (j != subnodes.end())
    {
      j->second.match(r, pos + 1, results, stroke);
    }
}

void music_node::add_to_subtree(const unsigned int &pos, const music_details &md, const unsigned int &i, const unsigned int &key)
{
  BellNodeMap::iterator j = subnodes.find(pos);
  if (j == subnodes.end())
    {
      music_node mn(bells);
      subnodes.insert(make_pair(pos, mn));
      j = subnodes.find(pos);
    }
  j->second.add(md, i + 1, key);
}

// ********************************************************
// function definitions for MUSIC
// ********************************************************

// default constructor.
music::music(const unsigned int &b) : TopNode(b)
{
  bells = 0;

  // Reset the music
  reset_music();
}

unsigned int music::specify_music(const music_details &md)
{
  MusicInfo.push_back(md);
  TopNode.add(md, 0, MusicInfo.size() - 1);
  return MusicInfo.size() - 1;
}

void music::set_bells(const unsigned int &b)
{
  bells = b;
  TopNode.set_bells(b);
}

// reset_music - clears all the music information entries.
void music::reset_music(void)
{
  mdvector::iterator i;
  for (i = MusicInfo.begin(); i != MusicInfo.end(); i++)
    i->clear();
}

// process_row - works out if a certain row is considered musical,
// and increments or changes the appriopriate variable.
void music::process_row(const row &r, const bool &back)
{
  if (back)
    TopNode.match(r, 0, MusicInfo, eBackstroke);
  else
    TopNode.match(r, 0, MusicInfo, eHandstroke);
}

unsigned int music::Get_Results(const unsigned int &i, const EStroke &stroke)
{
  return MusicInfo[i].count(stroke);
}

int music::Get_Score(const unsigned int &i, const EStroke &stroke)
{
  return MusicInfo[i].total(stroke);
}

int music::Get_Score(const EStroke &stroke)
{
  int total = 0;
  mdvector::iterator i;
  for (i = MusicInfo.begin(); i != MusicInfo.end(); i++)
    total += i->total(stroke);
  return total;
}

RINGING_END_NAMESPACE
