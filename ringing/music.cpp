// -*- C++ -*- music.cpp - Musical Analysis
// Copyright (C) 2001, 2008, 2009, 2010, 2011, 2019
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/music.h>
#include <ringing/streamutils.h>
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

  bool match(const row &r, unsigned int pos, vector<music_details> &results, const EStroke &stroke) const;

  // Helper function to work with cloning_pointer.
  music_node* clone() const { return new music_node(*this); }

private:
  BellNodeMap subnodes;
  vector<unsigned int>  detailsmatch;
  unsigned int bells;

  void add_to_subtree(unsigned place, const music_details &md, unsigned i, 
                      unsigned key, unsigned pos, bool process_star);
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
invalid_named_music::invalid_named_music( string const& n, string const& msg )
  :  invalid_argument( "The named music '" + n + "' was invalid: " + msg )
{}
#endif

music_details::music_details(const string &e, int sh, int sb)
{
  set(e, sh, sb);
}
music_details::music_details(const string &e, int s)
{
  set(e, s, s);
}

music_details::~music_details()
{
}

bool music_details::set(const string &e, int s)
{
  return set(e, s, s);
}

bool music_details::set(const string &e, int sh, int sb)
{
  scoreh = sh;  scoreb = sb;

  // Should reset this here as we have changed the expression
  counth = countb = 0;

  return pat.set(e);
}

string const& music_details::get() const
{
  return pat.get();
}

int music_details::possible_score(unsigned int bells) const
{
  // This is the maximum possible score, so we want the higher of
  // scoreh and scoreb.
  return possible_matches(bells) * (scoreh > scoreb ? scoreh : scoreb);
}

unsigned int music_details::possible_matches(unsigned int bells) const
{
  return pat.count(bells);
}

unsigned int music_details::count(const EStroke &stroke) const
{
  switch (stroke) {
    case eHandstroke: return counth;
    case eBackstroke: return countb;
    case eBoth: return counth + countb;
  }
  return 0u;
}

// Return the calculated score
int music_details::total(const EStroke &stroke) const
{
  switch (stroke) {
    case eHandstroke: return counth * scoreh;
    case eBackstroke: return countb * scoreb;
    case eBoth: return counth * scoreh  +  countb * scoreb;
  }
  return 0;
}

#if RINGING_BACKWARDS_COMPATIBLE(0,3,0)
int music_details::raw_score() const
{
  // We have to return /something/ if scoreh != scoreb
  return scoreh == scoreb ? scoreh : 0;
}
#endif

// Clear the Current counts
void music_details::clear()
{
  counth = countb = 0;
}

// Which count to increment
void music_details::increment(const EStroke &stroke)
{
  if (stroke == eBackstroke) countb++;
  else if (stroke == eHandstroke) counth++;
}

bool music_details::check_bells( unsigned bells ) const
{
  // Logically this is a const function -- we don't care
  // about the value of row_wildcard::bells(), so setting
  // it here doesn't change the visible properties of the 
  // music_details class.
  return const_cast< music_details * >(this)->pat.set_bells( bells );
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

void music_node::add(const music_details &md, unsigned int i, 
                     unsigned int key, unsigned int pos)
{
  string const& mds = md.get();

  // Does this item end here?
  if (i >= mds.size()) 
    detailsmatch.push_back(key);
  
  else if (pos <= bells) {
    char const* p = mds.c_str() + i;

    // Simple bell, add it and move on.
    if (bell::is_symbol(*p) || *p == '{') {
      char const* endp = p;
      bell b( bell::read_extended(p, &endp) );
      add_to_subtree(b+1, md, i + (endp-p) - 1, key,  pos, false);
    }

    else if (*p == '?') 
      add_to_subtree(0, md, i, key, pos, false);
      
    else if (*p == '[') {
      unsigned int newpos = mds.find(']', i + 1);
      char const* q = p+1;

      while (*q != ']') {
        if (bell::is_symbol(*q) || *q == '{') {
          char const* endp = q;
          bell b( bell::read_extended(q, &endp) );
          add_to_subtree(b+1, md, newpos, key, pos, false);
          q = endp;
        }
        else throw music_details::invalid_regex(mds, 
          make_string() << "Invalid character in []: " << *q );
      }
    }

    else if (*p == '*') {
      if (mds.size() == i + 1)
        // no more bells to go, don't bother adding to the subtree.
        // just add here
        detailsmatch.push_back(key);

      // There are more to go
      // Any of them '*'s?
      else if (mds.find('*', i + 1) >= mds.size()) 
        // Deal with the only * to go in the add_to_subtree
        // function.
        add_to_subtree(0, md, i, key, pos, true);
          
      else {
        // We have something like '*456*'
        // This functionality to be implemented.
        // First ignore the star and just move on.
        add(md, i + 1, key, pos);
        // Now deal with the star
        if (mds.size() - i >= pos)
          add_to_subtree(0, md, i, key, pos, true);
        else
          add_to_subtree(0, md, i, key, pos, false);
      }
    }
    else throw music_details::invalid_regex(mds, 
      make_string() << "Invalid character in pattern: " << *p );
  }
}

void music_node::add_to_subtree(unsigned int place, const music_details &md, unsigned int i, unsigned int key, unsigned int pos, bool process_star)
{
  cloning_pointer<music_node>& j = subnodes[place];
  if (!j) j.reset( new music_node(bells) );

  string const& mds = md.get();
  if (process_star)
    {
      // We are to process star data star.
      if (bells - pos == count_bells(mds.substr(i, mds.size() - i)) + 1 &&
	  mds.find('*', i + 1) >= mds.size())
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

bool music_node::match(const row &r, unsigned int pos, 
                       vector<music_details> &results, 
                       const EStroke &stroke) const
{
  bool matched = false;

  for ( vector<unsigned int>::const_iterator 
          i = detailsmatch.begin(), e = detailsmatch.end();  i != e;  ++i ) {
    results[*i].increment(stroke);
    matched = true;
  }

  // Try all against ? or *
  BellNodeMap::const_iterator j = subnodes.find(0);
  if (j != subnodes.end())
    matched = j->second->match(r, pos + 1, results, stroke) || matched;
    
  // Now try the actual number
  j = subnodes.find(r[pos] + 1);
  if (j != subnodes.end())
    matched = j->second->match(r, pos + 1, results, stroke) || matched;

  return matched;
}

// ********************************************************
// function definitions for MUSIC
// ********************************************************

// default constructor.
music::music(unsigned int b) 
  : top_node( new music_node(b) ), b(b)
{
  reset_music();
}

music::music(unsigned int b, music_details const& md) 
  : top_node( new music_node(b) ), b(b)
{
  reset_music();
  push_back(md);
}

// Specify the music and add it into the search structure
void music::push_back(const music_details &md)
{
  if (b)
    md.check_bells(b);

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

void music::set_bells(unsigned int new_b)
{
  if (b != new_b) {
    for (iterator i=info.begin(), e=info.end(); i!=e; ++i)
      i->check_bells(new_b);

    top_node->set_bells(new_b);
    b = new_b;
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
bool music::process_row(const row &r, bool back)
{
  return top_node->match(r, 0, info, back ? eBackstroke : eHandstroke);
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
    total += i->possible_score(b);
  return total;
}

RINGING_START_ANON_NAMESPACE

static void check_n( int bells, int n, string const& name )
{
  if ( n > bells )
    {
      RINGING_THROW_OR_RETURN_VOID( 
        invalid_named_music( name,
          "Cannot have runs longer than the total number of bells" ) );
    }
  else if ( n < 3 )
    {
      RINGING_THROW_OR_RETURN_VOID( 
        invalid_named_music( name,
          "Can only search for runs of three or more bells" ) );
    }
}


static void init_front_n_runs( music& m, int n, int sh, int sb, 
                               string const& name )
{
  const int bells = m.bells();
  check_n(bells, n, name);

  for ( int i=0; i<=bells-n; ++i )
    {
      make_string os;
      for ( int j=0; j<n; ++j ) os << bell( i+j );
      os << '*';
      m.push_back(music_details(os, sh, sb));
    }

  for ( int i=0; i<=bells-n; ++i )
    {
      make_string os;
      for ( int j=n-1; j>=0; --j ) os << bell( i+j );
      os << '*';
      m.push_back(music_details(os, sh, sb));
    }
}

static void init_back_n_runs( music& m, int n, int sh, int sb, 
                              string const& name )
{
  const int bells = m.bells();
  check_n(bells, n, name);

  for ( int i=0; i<=bells-n; ++i )
    {
      make_string os;
      os << '*';
      for ( int j=0; j<n; ++j ) os << bell( i+j );
      m.push_back(music_details(os, sh, sb));
    }

  for ( int i=0; i<=bells-n; ++i )
    {
      make_string os;
      os << '*';
      for ( int j=n-1; j>=0; --j ) os << bell( i+j );
      m.push_back(music_details(os, sh, sb));
    }
}

static void init_n_runs( music& m, int n, int sh, int sb, string const& name )
{
  const int bells = m.bells();
  check_n(bells, n, name);

  for ( int i=0; i<=bells-n; ++i )
    {
      make_string os;
      os << '*';
      for ( int j=0; j<n; ++j ) os << bell( i+j );
      os << '*';
      m.push_back(music_details(os, sh, sb));
    }

  for ( int i=0; i<=bells-n; ++i )
    {
      make_string os;
      os << '*';
      for ( int j=n-1; j>=0; --j ) os << bell( i+j );
      os << '*';
      m.push_back(music_details(os, sh, sb));
    }
}

static void init_crus( music& m, int sh, int sb )
{
  const int bells = m.bells();

  if (bells >= 6)
    for ( int i = 3; i < 6; ++i ) 
      for ( int j = 3; j < 6; ++j )
      {
  	if ( i == j ) 
  	  continue;
  	
  	make_string os;
  	os << '*' << bell(i) << bell(j) ;
  	for ( int k = 6; k < bells; ++k )
  	  os << bell(k);
  	
  	m.push_back(music_details(os, sh, sb));
      }
}

RINGING_END_ANON_NAMESPACE

RINGING_API void add_named_music( music& mu, string const& n, int score )
{
  return add_named_music( mu, n, score, score );
}

RINGING_API void add_named_music( music& mu, string const& n, int sh, int sb )
{
  const int bells = mu.bells();

  if ( n == "rounds" )
    mu.push_back
      ( music_details( row( bells ).print(), sh, sb ) );

  else if ( n == "queens" )
    mu.push_back
      ( music_details( row::queens( bells ).print(), sh, sb ) );

  else if ( n == "kings" )
    mu.push_back
      ( music_details( row::kings( bells ).print(), sh, sb ) );

  else if ( n == "tittums" )
    mu.push_back
      ( music_details( row::tittums( bells ).print(), sh, sb ) );


  else if ( n == "reverse-rounds" )
    mu.push_back
      ( music_details( row::reverse_rounds( bells ).print(), sh, sb ) );

  else if ( n == "CRUs" )
    init_crus( mu, sh, sb );
  
  else if ( n.length() > 11 && n.substr(0,6) == "front-" 
            && n.find("-runs") != string::npos )
    {
      char *endp;
      int l = strtol( n.c_str() + 6, &endp, 10 );
      if ( strcmp(endp, "-runs") )
        RINGING_THROW_OR_RETURN_VOID(
          invalid_named_music( n, "Unknown type of front-run" ) );

      init_front_n_runs( mu, l, sh, sb, n );
    }

  else if ( n.length() > 10 && n.substr(0,5) == "back-" 
            && n.find("-runs") != string::npos )
    {
      char *endp;
      int l = strtol( n.c_str() + 5, &endp, 10 );
      if ( strcmp(endp, "-runs") )
        RINGING_THROW_OR_RETURN_VOID(
          invalid_named_music( n, "Unknown type of back-run" ) );

      init_back_n_runs( mu, l, sh, sb, n );
    }

  else if ( n.length() > 5 && n.find("-runs") != string::npos )
    {
      char *endp;
      int l = strtol( n.c_str(), &endp, 10 );
      if ( strcmp(endp, "-runs") )
        RINGING_THROW_OR_RETURN_VOID(
          invalid_named_music( n, "Unknown type of run" ) );

      init_n_runs( mu, l, sh, sb, n );
    }

  else 
    {
      RINGING_THROW_OR_RETURN_VOID(
        invalid_named_music( n, "Unknown named music type" ) );
    }
}

RINGING_API void add_scored_music_string( music& mu, string const& s )
{
  int scoreh = 1, scoreb = 1;
  string pattern = s;

  if ( pattern.size() > 2 && 
       ( isdigit(pattern[0]) || pattern[0] == '+' || pattern[0] == '-' ) )
    {
      string::size_type comma = string::npos;
      for ( string::size_type j = 0; j < pattern.size(); ++j )
        if ( pattern[j] == ':' )
          {
            if (comma == string::npos) 
              scoreh = scoreb = atoi( pattern.c_str() );
            else
              scoreb = atoi( pattern.substr(comma+1, j-comma-1).c_str() );
            pattern = pattern.substr(j+1);
            break;
          }
        else if ( pattern[j] == ',' )
          {
            if (comma != string::npos) 
              throw runtime_error("Multiple commas in music score");
            scoreh = atoi( pattern.c_str() );
            comma = j;
          }
        else if ( !isdigit( pattern[j] ) && pattern[j] != '-' )
          break;
    }

    if ( pattern.size() > 2 
         && pattern[0] == '<' && pattern[ pattern.size()-1 ] == '>' )
      add_named_music( mu, pattern.substr( 1, pattern.size()-2 ), 
                       scoreh, scoreb );

    else
      mu.push_back( music_details( pattern, scoreh, scoreb ) );
}


RINGING_END_NAMESPACE
