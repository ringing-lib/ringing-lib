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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/music.h>
#include <ctype.h>
#include <stdio.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

// General function to work out if it's a bell or not
// according to exceptions or not.
bool is_bell(const char &c, bell &b)
{
  bool isbell = true;
#if RINGING_USE_EXCEPTIONS
  try
    {
      b.from_char(c);
    }
  catch (const bell::invalid &)
    {
      isbell = false;
    }
#else
  b.from_char(c);
  if (b > bell::MAX_BELLS)
    {
      isbell = false;
    }
#endif
  return isbell;
}

unsigned int count_bells(const string &s)
{
  unsigned int total = 0;
  string::const_iterator i;
  bell b;
  int brackets = 0;
  for (i = s.begin(); i != s.end(); i++)
    {
      if (*i == '?')
	{
	  total++;
	}
      else if (*i == '[')
	{
	  brackets = 1;
	}
      else if (*i == ']')
	{
	  total++; // [] = 1 bell.
	  brackets = 0;
	}
      else if ((brackets == 0) && (is_bell(*i, b)))
	{
	  total++;
	}
    }
  return total;
}

// ********************************************************
// function definitions for MUSIC_DETAILS
// ********************************************************

#if RINGING_USE_EXCEPTIONS
music_details::invalid_regex::invalid_regex() 
  : invalid_argument("Invalid musical expression supplied.") {}
#endif

music_details::music_details(const string &e, const int &s) : string(e)
{
  if ((e != "") && (!check_expression()))
    {
      // Can't do match if we don't use exceptions, so
      // just set the expression to "", hopefully the user will catch this.
      *this = "";
    }
  _score = s;
  _count_handstroke = 0;
  _count_backstroke = 0;
}

music_details::music_details(const char *e, const int &s) : string(e)
{
  if ((string(e) != "") && (!check_expression()))
    {
      // Can't do match if we don't use exceptions, so
      // just set the expression to "", hopefully the user will catch this.
      *this = "";
    }
  _score = s;
  _count_handstroke = 0;
  _count_backstroke = 0;
}

music_details::~music_details()
{
}

bool music_details::set(const string &e, const int &s)
{
  *this = e;
  _score = s;
  // Should reset this here as we have changed the expression
  _count_handstroke = 0;
  _count_backstroke = 0;
  bool isvalid = check_expression();
  if (!isvalid)
    {
      // Can't do match if we don't use exceptions, so
      // just set the expression to "", hopefully the user will catch this.
      *this = "";
    }
  return isvalid;
}

bool music_details::set(const char *e, const int &s)
{
  *this = (string) e;
  _score = s;
  // Should reset this here as we have changed the expression
  _count_handstroke = 0;
  _count_backstroke = 0;
  bool isvalid = check_expression();
  if (!isvalid)
    {
      // Can't do match if we don't use exceptions, so
      // just set the expression to "", hopefully the user will catch this.
      *this = "";
    }
  return isvalid;
}

string music_details::get() const
{
  return *this;
}

int music_details::possible_score(const unsigned int &bells) const
{
  return possible_matches(bells) * _score;
}

unsigned int music_details::possible_matches(const unsigned int &bells) const
{
  int q = 1;
  return possible_matches(bells, 0, *this, q);
}

unsigned int music_details::possible_matches(const unsigned int &bells, const unsigned int &pos, const string &s, int &q) const
{
  if (pos >= s.size()) // was >= bells
    {
      return 1;
    }
  else
    {
      bell b;
      if (is_bell(s[pos], b))
	{
	  return possible_matches(bells, pos + 1, s, q);
	}
      else if (s[pos] == '?')
	{
	  unsigned int count = possible_matches(bells, pos + 1, s, q) * q;
	  q++;
	  return count;
	}
      else if (s[pos] == '[')
	{
	  // Replace the [...] with each item in it and 
	  // pass through again.
	  unsigned int lastpos = s.find(']', pos + 1);
	  unsigned int total = 0;
	  unsigned int origq = q;
	  for (unsigned int i = pos + 1; i < lastpos; i++)
	    {
	      string modified(s, 0, pos);
	      unsigned int newpos = modified.size();
	      // Only do this if the bell isn't in the string
	      // already.
	      if (modified.find(s[i]) > modified.size())
		{
		  modified += s[i];
		  modified += s.substr(lastpos + 1, s.size() - lastpos - 1);
		  // ensure q is reset
		  q = origq;
		  total += possible_matches(bells, newpos, modified, q);
		}
	    } 
	  return total;
	}
      else if (s[pos] == '*')
	{
	  if ((s.size() == pos + 1) || 
	      (s.find('*', pos + 1) > s.size() - pos))
	    {
	      // Just 1 star, therefore replace with maximum ?
	      string modified(s, 0, pos);
	      // Now add ?
	      for (unsigned int i = 0; i < bells - count_bells(s.substr(0, pos)) - count_bells(s.substr(pos + 1, s.size() - pos - 1)); i++)
		{
		  modified += '?';
		}
	      // Now add rest of string
	      modified += s.substr(pos + 1, s.size() - pos - 1);
	      return possible_matches(bells, pos, modified, q);
	    }
	  else
	    {
	      // More than 1 star. Replace string with 0, 1, 2... '?'
	      // and calculate at each stage
	      unsigned int total = 0;
	      unsigned int origq = q;
	      for (unsigned int i = 0; i <= bells - count_bells(s.substr(pos + 1, s.size() - pos - 1)) - count_bells(s.substr(0, pos)); i++)
		{
		  string modified(s, 0, pos);
		  for (unsigned int j = 0; j < i; j++)
		    {
		      modified += '?';
		    }

		  // Now add rest of string
		  modified += s.substr(pos + 1, s.size() - pos - 1);
		  // reset q to ensure we start with the same each time.
		  q = origq;
		  total += possible_matches(bells, pos, modified, q);
		}
	      return total;
	    }
	}
      else
	{
#if RINGING_USE_EXCEPTIONS
	  throw invalid_regex();
#endif
	  return 0;
	}
    }
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

int music_details::raw_score() const
{
  return _score;
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

// Function to provide a brief check if an expression is valid/invalid.
bool music_details::check_expression()
{
  // check all items are valid.
  std::string::iterator i;
  bell b;
  bool valid = true;
  int sqbrackets = 0;
  for (i = this->begin(); i != this->end(); i++)
    {
      if (!is_bell(*i, b))
	{
	  // Check it is not another valid character
	  switch (*i)
	    {
	    case '?':
	    case '*':
	      if (sqbrackets != 0)
		{
#if RINGING_USE_EXCEPTIONS
		  throw invalid_regex();
#else
		  return false;
#endif
		}
	      break;
	    case '[':
	      if (sqbrackets != 0)
		{
#if RINGING_USE_EXCEPTIONS
		  throw invalid_regex();
#else
		  return false;
#endif
		}
	      sqbrackets = 1;
	      break;
	    case ']':
	      if (sqbrackets != 1)
		{
#if RINGING_USE_EXCEPTIONS
		  throw invalid_regex();
#else
		  return false;
#endif
		}
	      sqbrackets = 0;
	      break;
	    default:
	      valid = false;
	    }
	  if (!valid)
	    {
#if RINGING_USE_EXCEPTIONS
	      throw invalid_regex();
#else
	      return false;
#endif
	      break;
	    }
	}
    }
  if (sqbrackets != 0)
    {
#if RINGING_USE_EXCEPTIONS
      throw invalid_regex();
#else
      return false;
#endif
    }
  return valid;
}

// ********************************************************
// function definitions for MUSIC_NODE
// ********************************************************

#if RINGING_USE_EXCEPTIONS
music_node::memory_error::memory_error() 
  : overflow_error("Not enough memory to allocate to music_node item") {}
#endif

music_node::music_node()
{
  bells = 0;
}

music_node::music_node(const unsigned int &b)
{
  bells = b;
}

music_node::~music_node()
{
  BellNodeMapIterator i;
  for (i = subnodes.begin(); i != subnodes.end(); i++)
    {
      if (i->second != NULL)
	{
	  delete i->second;
	}
    }
}

void music_node::set_bells(const unsigned int &b)
{
  bells = b;
  // for each subnode
  BellNodeMapIterator i;
  for (i = subnodes.begin(); i != subnodes.end(); i++)
    {
      if (i->second != NULL)
	i->second->set_bells(b);
    }
}

void music_node::add(const music_details &md, const unsigned int &i, const unsigned int &key, const unsigned int &pos)
{
  // Does this item end here?
  if (i >= md.size())
    {
      detailsmatch.push_back(key);
    }
  else if (pos <= bells)
    {
      bell b;
      if (is_bell(md[i], b))
	{
	  // Simple bell, add it and move on.
	  add_to_subtree(b + 1, md, i, key,  pos, false);
	}
      else
	{
	  if (md[i] == '?')
	    {
	      add_to_subtree(0, md, i, key, pos, false);
	    }
	  else if (md[i] == '[')
	    {
	      unsigned int j = i;
	      // 'remove' the [] section from the string
	      unsigned int newpos = md.find(']', i + 1);
	      j++;
	      // We have an option, so add to each tree until ] occurs.
	      while (md[j] != ']')
		{
		  if (is_bell(md[j], b))
		    {
		      add_to_subtree(b + 1, md, newpos, key, pos, false);
		    }
		  // else ignore it for now...
		  j++;
		}
	      // ok, that's all for here.
	    }
	  else if (md[i] == '*')
	    {
	      if (md.size() == i + 1)
		{
		  // no more bells to go, don't bother adding to the subtree.
		  // just add here
		  detailsmatch.push_back(key);
		}
	      else
		{
		  // There are more to go
		  // Any of them '*'s?
		  if (md.find('*', i + 1) >= md.size())
		    {
		      // Deal with the only * to go in the add_to_subtree
		      // function.
		      add_to_subtree(0, md, i, key, pos, true);
		    }
		  else
		    {
		      // We have *456*
		      // This functionality to be implemented.
		      // First ignore the star and just move on.
		      add(md, i + 1, key, pos);
		      // Now deal with the star
		      if (md.size() - i >= pos)
			add_to_subtree(0, md, i, key, pos, true);
		      else
			add_to_subtree(0, md, i, key, pos, false);
		    }
		}
	    }
	}
    }
}

void music_node::add_to_subtree(const unsigned int &place, const music_details &md, const unsigned int &i, const unsigned int &key, const unsigned int &pos, const bool &process_star)
{
  BellNodeMap::iterator j = subnodes.find(place);
  if (j == subnodes.end())
    {
      music_node *mn = new music_node(bells);
      if (mn != NULL)
	j = (subnodes.insert(make_pair(place, mn))).first;
      else
	{
#if RINGING_USE_EXCEPTIONS
	  throw memory_error();
	  return;
#else
	  cerr << "Not enough memory to allocate to new music_node\n";
	  return;
#endif
	}
    }
  if (process_star)
    {
      // We are to process star data star.
      if ((bells - pos == count_bells(md.substr(i, md.size() - i)) + 1) &&
	  (md.find('*', i + 1) >= md.size()))
	{
	  // There are now only numbers to go.
	  j->second->add(md, i + 1, key, pos + 1);
	}
      else
	{
	  // We haven't got to the last * position yet, so carry on.
	  j->second->add(md, i, key, pos + 1);
	}
    }
  else
    {
      // Not a star, so move on as normal.
      j->second->add(md, i + 1, key, pos + 1);
    }
}

void music_node::match(const row &r, const unsigned int &pos, vector<music_details> &results, const EStroke &stroke)
{
  DetailsVectorIterator i;
  for (i = detailsmatch.begin(); i < detailsmatch.end(); i++)
    {
      results[*i].increment(stroke);
    }
  // Try all against ? or *
  BellNodeMapIterator j = subnodes.find(0);
  if (j != subnodes.end())
    {
      j->second->match(r, pos + 1, results, stroke);
    }
  // Now try the actual number
  j = subnodes.find(r[pos] + 1);
  if (j != subnodes.end())
    {
      j->second->match(r, pos + 1, results, stroke);
    }
}

// ********************************************************
// function definitions for MUSIC
// ********************************************************

// default constructor.
music::music(const unsigned int &b) : TopNode(b), bells(b)
{
  // Reset the music
  reset_music();
}

// Specify the music and add it into the search structure
void music::push_back(const music_details &md)
{
  MusicInfo.push_back(md);
  TopNode.add(md, 0, MusicInfo.size() - 1, 0);
}

music::iterator music::begin()
{
  return MusicInfo.begin();
}

music::const_iterator music::begin() const
{
  return MusicInfo.begin();
}

music::iterator music::end()
{
  return MusicInfo.end();
}

music::const_iterator music::end() const
{
  return MusicInfo.end();
}

music::size_type music::size() const
{
  return MusicInfo.size();
}

void music::set_bells(const unsigned int &b)
{
  TopNode.set_bells(b);
  bells = b;
}

// reset_music - clears all the music information entries.
void music::reset_music(void)
{
  music::iterator i;
  for (i = begin(); i != end(); i++)
    i->clear();
}

// process_row - works out if a certain row is considered musical,
// and increments or changes the appriopriate variable.
void music::process_row(const row &r, const bool back)
{
  if (back)
    TopNode.match(r, 0, MusicInfo, eBackstroke);
  else
    TopNode.match(r, 0, MusicInfo, eHandstroke);
}

// Return the total score for all items
int music::get_score(const EStroke &stroke)
{
  int total = 0;
  music::const_iterator i;
  for (i = begin(); i != end(); i++)
    total += i->total(stroke);
  return total;
}

// Return the total matches for all items
unsigned int music::get_count(const EStroke &stroke)
{
  unsigned int count = 0;
  music::const_iterator i;
  for (i = begin(); i != end(); i++)
    count += i->count(stroke);
  return count;
}

int music::get_possible_score()
{
  int total = 0;
  music::const_iterator i;
  for (i = begin(); i != end(); i++)
    total += i->possible_score(bells);
  return total;
}

RINGING_END_NAMESPACE
