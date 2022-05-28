// -*- C++ -*- music.cpp - Musical Analysis
// Copyright (C) 2001, 2008, 2009, 2010, 2011, 2019, 2020, 2022
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/music.h>
#include <ringing/streamutils.h>
#include <cctype>
#include <cstdio>
#include <cstring>

RINGING_START_NAMESPACE

RINGING_USING_STD

// The music_node class is the internal representation of a collection of 
// patterns which are to matched against a series of rows.  It is a tree-based
// data structure.
//
// No need to mark this as RINGING_API as it is not visible outside of here
class music_node
{
public:
  // Have to know how many bells there are
  music_node(unsigned int b) : bells(b) {} 
  void set_bells(unsigned int b);

  void add(const string &mds, unsigned int i, unsigned int key, 
           unsigned int pos);

  bool match(const row &r, unsigned int pos, vector<music_details> &results, 
             const EStroke &stroke, vector<bell> const& wildcard_matches) const;

  // Helper function to work with cloning_pointer.
  music_node* clone() const { return new music_node(*this); }

private:
  // This subnodes map is the recursive part of this data structures.
  // As we try to match a row against this music node, we recurse down 
  // this tree of subnode, using the current bell as the index into the 
  // map.  A key of 1 matches the treble, 2, the second, etc.  The 
  // special value 0 is a wildcard, matching any one bell.
  typedef map<unsigned int, cloning_pointer<music_node> > BellNodeMap;
  BellNodeMap subnodes;

  // Every entry in the detailsmatch vector represents a pattern which has
  // been matched if we reach this node.  The values are the keys which were 
  // passed to add(), and which are used as offsets into the 'results' vector 
  // during matching to record the number of successful matches of each
  // pattern.
  vector<unsigned int>  detailsmatch;

  unsigned int bells;

  void add_to_subtree(unsigned place, const string &mds, unsigned i, 
                      unsigned key, unsigned pos, bool process_star);
};

unsigned int count_bells(const string &s)
{
  unsigned int total = 0;
  string::const_iterator i;
  int brackets = 0;
  for (i = s.begin(); i != s.end(); i++) {
    if (*i == '?')
      total++;
    else if (*i == '[')
      brackets = 1;
    else if (*i == ']') {
      total++; // [...] = 1 bell.
      brackets = 0;
    }
    else if (brackets == 0 && bell::is_symbol(*i))
      total++;
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
  
vector<bell> music_details::last_wildcard_matches() const 
{ 
  return last_wildcards; 
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

void music_node::set_bells(unsigned int b)
{
  bells = b;

  // and iterate over each subnode setting it there
  for (BellNodeMap::iterator i=subnodes.begin(), e=subnodes.end(); i!=e; ++i)
    if (i->second)
      i->second->set_bells(b);
}

// "mds" is the pattern as a string, and "i" is the current character offset
// into it (as we are called recursively).  "key" is used to identify which
// pattern matched, if we have several patterns.  "pos" is the logical offset
// in terms of bells.
void music_node::add(const string &mds, unsigned int i, 
                     unsigned int key, unsigned int pos)
{
  // If we've reached the end of the string, add the key, which will tell
  // match() that the pattern identified by the key has matched.
  if (i >= mds.size()) 
    detailsmatch.push_back(key);

  // If we're trying to match more bells than there are, it must fail,
  // so we may as well stop now.
  else if (pos > bells)
    return;

  // Look at the next token in pattern and generate a subnode for it.  
  else {
    char const* p = mds.c_str() + i;

    // Simple bell (possibly in extended form), read it, add it and move on.
    if (bell::is_symbol(*p) || *p == '{') {
      char const* endp = p;
      bell b( bell::read_extended(p, &endp) );
      add_to_subtree(b+1, mds, i + (endp-p) - 1, key,  pos, false);
    }

    // A single bell wildcard: add it to the 0 wildcard map position
    else if (*p == '?') 
      add_to_subtree(0, mds, i, key, pos, false);
      
    // A set of alternatives: add each one individually
    else if (*p == '[') {
      unsigned int newpos = mds.find(']', i + 1);
      char const* q = p+1;

      while (*q != ']') {
        if (bell::is_symbol(*q) || *q == '{') {
          char const* endp = q;
          bell b( bell::read_extended(q, &endp) );
          add_to_subtree(b+1, mds, newpos, key, pos, false);
          q = endp;
        }
        else throw music_details::invalid_regex(mds, 
          make_string() << "Invalid character in []: " << *q );
      }
    }

    // An variable length wildcard
    else if (*p == '*') {
      // If there's nothing more in the string, just put the instruction
      // to match the row here.
      if (i+1 == mds.size())
        detailsmatch.push_back(key);

      // Is this the last * in the pattern?  If so, it is handled by
      // add_to_subtree().
      else if (mds.find('*', i + 1) == string::npos) 
        add_to_subtree(0, mds, i, key, pos, true);
          
      else {
        // We have something like '*456*'
        // First ignore the star and just move on.
        add(mds, i + 1, key, pos);
        // Now deal with the star
        if (mds.size() - i >= pos)
          add_to_subtree(0, mds, i, key, pos, true);
        else
          add_to_subtree(0, mds, i, key, pos, false);
      }
    }
    else throw music_details::invalid_regex(mds, 
      make_string() << "Invalid character in pattern: " << *p );
  }
}

void music_node::add_to_subtree(unsigned int place, const string &mds, unsigned int i, unsigned int key, unsigned int pos, bool process_star)
{
  cloning_pointer<music_node>& j = subnodes[place];
  if (!j) j.reset( new music_node(bells) );

  if (process_star)
    {
      // We are to process star data star.
      if (bells - pos == count_bells(mds.substr(i, mds.size() - i)) + 1 &&
	  mds.find('*', i + 1) >= mds.size())
	// There are now only numbers to go.
	j->add(mds, i + 1, key, pos + 1);
      else
	// We haven't got to the last * position yet, so carry on.
	j->add(mds, i, key, pos + 1);
    }
  else
    {
      // Not a star, so move on as normal.
      j->add(mds, i + 1, key, pos + 1);
    }
}

bool music_node::match(const row &r, unsigned int pos, 
                       vector<music_details> &results, const EStroke &stroke, 
                       vector<bell> const& wilds) const
{
  bool matched = false;

  for ( vector<unsigned int>::const_iterator 
          i = detailsmatch.begin(), e = detailsmatch.end();  i != e;  ++i ) {
    // Append any bells that we haven't yet matched due to a wildcard
    vector<bell> wilds2(wilds); 
    for (unsigned k = pos; k < r.bells(); ++k)
      wilds2.push_back(r[k]);

    results[*i].increment(stroke);
    results[*i].last_wildcards = wilds2;
    matched = true;
  }

  // Is there a wildcard subnode at index 0?
  BellNodeMap::const_iterator j = subnodes.find(0);
  if (j != subnodes.end()) {
    // Records the bell(s) that matched the wildcard
    vector<bell> wilds2(wilds); 
    wilds2.push_back(r[pos]);

    matched = j->second->match(r, pos + 1, results, stroke, wilds2) || matched;
  }
    
  // Is there a subnode matching this specific bell?
  j = subnodes.find(r[pos] + 1);
  if (j != subnodes.end())
    matched = j->second->match(r, pos + 1, results, stroke, wilds) || matched;

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

void music::clear(unsigned int new_b) 
{
  if (new_b) b = new_b;
  top_node.reset( new music_node(b) );
  reset_music();
}

// Specify the music and add it into the search structure
void music::push_back(const music_details &md)
{
  if (b)
    md.check_bells(b);

  info.push_back(md);
  top_node->add(md.get(), 0, info.size() - 1, 0);
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
void music::reset_music()
{
  music::iterator i;
  for (i = begin(); i != end(); i++)
    i->clear();
}

// process_row - works out if a certain row is considered musical,
// and increments or changes the appriopriate variable.
bool music::process_row(const row &r, bool back)
{
  for (music_details& md : info)
    md.last_wildcards.clear();

  return top_node->match(r, 0, info, back ? eBackstroke : eHandstroke,
                         vector<bell>());
}

// Return the total score for all items
int music::get_score(const EStroke &stroke) const
{
  int total = 0;
  for (music_details const& md : info)
    total += md.total(stroke);
  return total;
}

// Return the total matches for all items
unsigned int music::get_count(const EStroke &stroke) const
{
  unsigned int count = 0;
  for (music_details const& md : info)
    count += md.count(stroke);
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

music music::make_runs_match( int b, int n, match_pos pos, int dir, 
                              int sh, int sb ) {
  music ret(b);

  string prefix( pos != at_front ? "*" : "" ); 
  string suffix( pos != at_back  ? "*" : "" ); 

  if (dir >= 0)
    for ( int i=0; i<=b-n; ++i ) {
      make_string os;
      os << prefix;
      for ( int j=0; j<n; ++j ) os << bell( i+j );
      os << suffix;
      ret.push_back(music_details(os, sh, sb));
    }

  if (dir <= 0) 
    for ( int i=0; i<=b-n; ++i ) {
      make_string os;
      os << prefix;
      for ( int j=n-1; j>=0; --j ) os << bell( i+j );
      os << suffix;
      ret.push_back(music_details(os, sh, sb));
    }

  return ret;
}

RINGING_START_ANON_NAMESPACE

static int read_len( int bells, int offset, string const& name ) {
  char *endp;
  int n = strtol( name.c_str() + offset, &endp, 10 );
  if ( strcmp(endp, "-runs") )
    RINGING_THROW_OR_RETURN_VOID(
      invalid_named_music( name, "Unable to read run length" ) );

  if ( n > bells )
    RINGING_THROW_OR_RETURN_VOID( 
      invalid_named_music( name,
        "Cannot have runs longer than the total number of bells" ) );
  if ( n < 3 )
    RINGING_THROW_OR_RETURN_VOID( 
      invalid_named_music( name,
        "Can only search for runs of three or more bells" ) );

  return n;
}

RINGING_END_ANON_NAMESPACE

music music::make_cru_match(int bells, int sh, int sb) {
  music ret(bells);
  if (bells >= 6)
    for ( int i = 3; i < 6; ++i ) 
      for ( int j = 3; j < 6; ++j ) {
  	if ( i == j ) 
  	  continue;
  	
  	make_string os;
  	os << '*' << bell(i) << bell(j) ;
  	for ( int k = 6; k < bells; ++k )
  	  os << bell(k);
  	
  	ret.push_back(music_details(os, sh, sb));
      }
  return ret;
}

void music::add_named_music( string const& n, int sh, int sb ) {
  if ( n == "rounds" )
    push_back
      ( music_details( row(b).print(), sh, sb ) );

  else if ( n == "queens" )
    push_back
      ( music_details( row::queens(b).print(), sh, sb ) );

  else if ( n == "kings" )
    push_back
      ( music_details( row::kings(b).print(), sh, sb ) );

  else if ( n == "tittums" )
    push_back
      ( music_details( row::tittums(b).print(), sh, sb ) );

  else if ( n == "reverse-rounds" )
    push_back
      ( music_details( row::reverse_rounds(b).print(), sh, sb ) );

  else if ( n == "CRUs" )
    append( music::make_cru_match( b, sh, sb ) );
  
  else if ( n.length() > 11 && n.substr(0,6) == "front-" 
            && n.find("-runs") != string::npos )
    append( music::make_runs_match( b, read_len(b, 6, n), 
                                    music::at_front, 0, sh, sb ) );

  else if ( n.length() > 10 && n.substr(0,5) == "back-" 
            && n.find("-runs") != string::npos )
    append( music::make_runs_match( b, read_len(b, 5, n),
                                    music::at_back, 0, sh, sb ) );

  else if ( n.length() > 5 && n.find("-runs") != string::npos )
    append( music::make_runs_match( b, read_len(b, 0, n), 
                                    music::anywhere, 0, sh, sb ) );

  else {
    RINGING_THROW_OR_RETURN_VOID(
      invalid_named_music( n, "Unknown named music type" ) );
  }
}

void music::add_scored_music_string( string const& s )
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
      add_named_music( pattern.substr( 1, pattern.size()-2 ), 
                       scoreh, scoreb );

    else
      push_back( music_details( pattern, scoreh, scoreb ) );
}


RINGING_END_NAMESPACE
