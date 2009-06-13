// -*- C++ -*- music.cpp - Musical Analysis
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/music.h>
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#else
#include <cctype>
#include <cstdio>
#include <cstring>
#endif   

RINGING_START_NAMESPACE

RINGING_USING_STD

// No need to mark this as RINGING_API as it is not visible outside of here
class music_node
{
public:
  // bell -> node map
  typedef map<unsigned int, cloning_pointer<music_node> > BellNodeMap;

  // Have to know how many bells there are
  music_node(unsigned int b);
  void set_bells(unsigned int b);

  void add(const music_details &md, unsigned int i, unsigned int key, unsigned int pos);

  void match(const row &r, unsigned int pos, vector<music_details> &results, const EStroke &stroke) const;

  // Helper function to work with cloning_pointer.
  music_node* clone() const { return new music_node(*this); }

private:
  BellNodeMap subnodes;
  vector<unsigned int>  detailsmatch;
  unsigned int bells;

  void add_to_subtree(unsigned int place, const music_details &md, unsigned int i, unsigned int key, unsigned int pos, bool process_star);
};

unsigned int count_bells(const string &s)
{
  unsigned int total = 0;
  string::const_iterator i;
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
      else if ((brackets == 0) && bell::is_symbol(*i))
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
music_details::invalid_regex::invalid_regex( string const& pat, 
                                             string const& msg ) 
  : invalid_argument( "The pattern '" + pat + "' was invalid: " + msg )
{}
#endif

music_details::music_details(const string &e, int s)
{
  set(e, s);
}

music_details::music_details(const char *e, int s) : string(e)
{
  set(e, s);
}

music_details::~music_details()
{
}

bool music_details::set(const string &e, int s)
{
  static_cast<string&>(*this) = e;
  score = s;
  // Should reset this here as we have changed the expression
  count_handstroke = 0;
  count_backstroke = 0;

  bool isvalid = check_expression();
#if !RINGING_USE_EXCEPTIONS
  // Can't do match if we don't use exceptions, so
  // just set the expression to "", hopefully the user will catch this.
  if (!isvalid) this->clear();
#endif
  return isvalid;
}

bool music_details::set(const char *e, int s)
{
  return set(string(e), s);
}

string music_details::get() const
{
  return *this;
}

int music_details::possible_score(unsigned int bells) const
{
  return possible_matches(bells) * score;
}

unsigned int music_details::possible_matches(unsigned int bells) const
{
  int q = 1;
  return possible_matches(bells, 0, *this, q);
}

unsigned int music_details::possible_matches(unsigned int bells, unsigned int pos, const string &s, int &q) const
{
  if (pos >= s.size()) // was >= bells
    {
      return 1;
    }
  else
    {
      if (bell::is_symbol(s[pos]))
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
          RINGING_THROW_OR_RETURN( 
            invalid_regex(*this, string("Unknown character: ") + s[pos]), 0 );
	}
    }
}

// Return the count
unsigned int music_details::count(const EStroke &stroke) const
{
  switch (stroke)
    {
    case eHandstroke:
      return count_handstroke;
    case eBackstroke:
      return count_backstroke;
    case eBoth:
      return count_handstroke + count_backstroke;
    }
  return 0;
}

// Return the calculated score
int music_details::total(const EStroke &stroke) const
{
  return count(stroke) * score;
}

int music_details::raw_score() const
{
  return score;
}

// Clear the Current counts
void music_details::clear()
{
  count_backstroke = 0;
  count_handstroke = 0;
}

// Which count to increment
void music_details::increment(const EStroke &stroke)
{
  if (stroke == eBackstroke)
    count_backstroke++;
    
  else if (stroke == eHandstroke)
    count_handstroke++;
}

void music_details::check_bells(unsigned int b) const
{
#if RINGING_USE_EXCEPTIONS
  for (string::const_iterator i = this->begin(); i != this->end(); i++)
    if (bell::is_symbol(*i) && bell::read_char(*i) >= b )
      RINGING_THROW_OR_RETURN_VOID( 
        invalid_regex(*this, string("Bell out of range: ") + *i ) );
#endif
}

// Function to provide a brief check if an expression is valid/invalid.
bool music_details::check_expression() const
{
  // check all items are valid.
  int sqbrackets = 0;
  for (string::const_iterator i = this->begin(); i != this->end(); i++)
    {
      if (!bell::is_symbol(*i))
	{
	  // Check it is not another valid character
	  switch (*i)
	    {
	    case '?':
	    case '*':
	      if (sqbrackets)
                RINGING_THROW_OR_RETURN( 
                  invalid_regex(*this, 
                    string("A ") + *i + " is not permitted in []s"), 
                  false );
	      break;
	    case '[':
	      if (sqbrackets)
                RINGING_THROW_OR_RETURN( 
                  invalid_regex(*this, "Nested [s"), false );
	      sqbrackets = 1;
	      break;
	    case ']':
	      if (sqbrackets != 1)
                RINGING_THROW_OR_RETURN( 
                  invalid_regex(*this, "Unexpected ']'"), false );
	      sqbrackets = 0;
	      break;
	    default:
              RINGING_THROW_OR_RETURN( 
                invalid_regex(*this, string("Unknown character: ") + *i),
                false );
	    }
	}
    }
  if (sqbrackets != 0)
    RINGING_THROW_OR_RETURN( 
      invalid_regex(*this, "Unterminated '['" ), false );
  return true;
}

// ********************************************************
// function definitions for MUSIC_NODE
// ********************************************************

music_node::music_node(unsigned int b)
{
  bells = b;
}

void music_node::set_bells(unsigned int b)
{
  bells = b;

  // and iterate over each subnode setting it there
  for (BellNodeMap::iterator i=subnodes.begin(), e=subnodes.end(); i!=e; ++i)
    if (i->second)
      i->second->set_bells(b);
}

void music_node::add(const music_details &md, unsigned int i, unsigned int key, unsigned int pos)
{
  // Does this item end here?
  if (i >= md.size())
    {
      detailsmatch.push_back(key);
    }
  else if (pos <= bells)
    {
      if (bell::is_symbol(md[i]))
	{
	  // Simple bell, add it and move on.
	  add_to_subtree(bell::read_char(md[i]) + 1, md, i, key,  pos, false);
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
		  if (bell::is_symbol(md[j]))
		    {
		      add_to_subtree(bell::read_char(md[j]) + 1, md,  
                                     newpos, key, pos, false);
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

void music_node::add_to_subtree(unsigned int place, const music_details &md, unsigned int i, unsigned int key, unsigned int pos, bool process_star)
{
  cloning_pointer<music_node>& j = subnodes[place];
  if (!j) j.reset( new music_node(bells) );

  if (process_star)
    {
      // We are to process star data star.
      if (bells - pos == count_bells(md.substr(i, md.size() - i)) + 1 &&
	  md.find('*', i + 1) >= md.size())
	// There are now only numbers to go.
	j->add(md, i + 1, key, pos + 1);
      else
	// We haven't got to the last * position yet, so carry on.
	j->add(md, i, key, pos + 1);
    }
  else
    {
      // Not a star, so move on as normal.
      j->add(md, i + 1, key, pos + 1);
    }
}

void music_node::match(const row &r, unsigned int pos, 
                       vector<music_details> &results, 
                       const EStroke &stroke) const
{
  for ( vector<unsigned int>::const_iterator 
          i = detailsmatch.begin(), e = detailsmatch.end();  i != e;  ++i ) 
    results[*i].increment(stroke);

  // Try all against ? or *
  BellNodeMap::const_iterator j = subnodes.find(0);
  if (j != subnodes.end())
    j->second->match(r, pos + 1, results, stroke);
    
  // Now try the actual number
  j = subnodes.find(r[pos] + 1);
  if (j != subnodes.end())
    j->second->match(r, pos + 1, results, stroke);
}

// ********************************************************
// function definitions for MUSIC
// ********************************************************

// default constructor.
music::music(unsigned int b) : top_node( new music_node(b) ), bells(b)
{
  // Reset the music
  reset_music();
}

// Specify the music and add it into the search structure
void music::push_back(const music_details &md)
{
  if (bells)
    md.check_bells(bells);

  info.push_back(md);
  top_node->add(md, 0, info.size() - 1, 0);
}

music::iterator music::begin()
{
  return info.begin();
}

music::const_iterator music::begin() const
{
  return info.begin();
}

music::iterator music::end()
{
  return info.end();
}

music::const_iterator music::end() const
{
  return info.end();
}

music::size_type music::size() const
{
  return info.size();
}

void music::set_bells(unsigned int b)
{
  if (bells != b) {
    for (iterator i=info.begin(), e=info.end(); i!=e; ++i)
      i->check_bells(b);

    top_node->set_bells(b);
    bells = b;
  }
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
void music::process_row(const row &r, bool back)
{
  top_node->match(r, 0, info, back ? eBackstroke : eHandstroke);
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
