// -*- C++ -*- row_wildcard.cpp - Represents a set of rows, e.g. '1???5678'
// Copyright (C) 2001, 2008, 2009, 2010 Mark Banner <mark@standard8.co.uk> and
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

#include <ringing/row_wildcard.h>
#include <ringing/bell.h>

#include <string>
#include <cassert>

RINGING_START_NAMESPACE

RINGING_USING_STD

row_wildcard::row_wildcard( string const& pattern, unsigned bells )
{
  this->set(pattern, bells);
}

bool row_wildcard::set( string const& pattern, unsigned bells )
{
  pat = pattern;  
  b = bells; // Set this now in case of an exception from check_expression

  bool isvalid = ( check_expression() && set_bells(bells) );
#if !RINGING_USE_EXCEPTIONS
  // Can't do match if we don't use exceptions, so
  // just set the expression to "", hopefully the user will catch this.
  if (!isvalid) this->set("", 0);
#endif
  return isvalid;
}

bool row_wildcard::set_bells( unsigned bells )
{
  b = bells;

  if (b) 
    for (string::const_iterator i = pat.begin(); i != pat.end(); i++)
      if (bell::is_symbol(*i) && bell::read_char(*i) >= b )
        RINGING_THROW_OR_RETURN( 
          invalid_pattern(pat, string("Bell out of range: ") + *i ), false );

  return true;
}

RINGING_START_ANON_NAMESPACE

// Count 1 for each '?', bell symbol, or [...]; and none for a '*'.
unsigned int count_bells(const string &s)
{
  unsigned int total = 0;
  string::const_iterator i;
  bool in_brackets = false;
  for (i = s.begin(); i != s.end(); i++)
    {
      if (*i == '?')
	{
	  total++;
	}
      else if (*i == '[')
	{
	  in_brackets = true;
	}
      else if (*i == ']')
	{
	  total++; // [...] = 1 bell.
	  in_brackets = false;
	}
      else if ( !in_brackets && bell::is_symbol(*i) )
	{
	  total++;
	}
    }
  return total;
}

string expand_star( string const& src, unsigned star_pos, unsigned count )
{
  string expanded( src, 0, star_pos );
  expanded.append( count, '?' );
  expanded.append( src, star_pos + 1, src.size() - star_pos - 1);
  return expanded;
}

RINGING_END_ANON_NAMESPACE

void row_wildcard::copy_choices( string& dest, 
                                 string const& src, unsigned& pos ) const
{
  assert( src[pos] == '[' );
  unsigned p_end = src.find( ']', pos );
  if ( p_end == string::npos ) 
    RINGING_THROW_OR_EXPR( 
      row_wildcard::invalid_pattern( pat, "Missing ]" ),
      p_end = src.size()-1 );

  dest.append( src, pos, p_end-pos+1 );
  pos = p_end;
}

bool row_wildcard::copy_if_choices_contains( string& dest, 
                                             string const& src, unsigned& pos,
                                             char sym ) const
{
  assert( src[pos] == '[' );
  unsigned p_end = src.find( ']', pos );
  if ( p_end == string::npos ) 
    RINGING_THROW_OR_EXPR( 
      row_wildcard::invalid_pattern( pat, "Missing ]" ),
      p_end = src.size()-1 );

  unsigned p_sym = src.find( sym, pos );
  if ( p_sym > p_end ) return false;
  dest += sym;
  pos = p_end;
}

bool row_wildcard::copy_choices_intersection( string& dest, 
                                              string const& a, unsigned& ai,
                                              string const& b, unsigned& bi )
                                                                         const
{
  assert( a[ai] == '[' && b[bi] == '[' );
  dest += '['; 

  unsigned b_init = bi;
  bool had_one = false;
  for ( ++ai; a[ai] != ']'; ++ai ) {
    bi = b_init;
    if ( copy_if_choices_contains( dest, b, bi, a[ai] ) ) 
      had_one = true;
  }

  dest += ']';
  return had_one;
}


// This is not a general purpose intersection function -- it only 
// parses A and B up to the first '*' and assumes that they are 
// identical after that.  Returns false if the intersection is
// necessarily zero.
bool row_wildcard::make_intersection( string& dest, 
                                      string const& a, string const& b ) const
{
  unsigned ai = 0, bi = 0;
  unsigned an = a.find('*'), bn = b.find('*');
  assert( an != string::npos && bn != string::npos );
  assert( a.substr(an) == b.substr(bn) );

  while ( ai != an || bi != bn ) 
  {
    bool a_wild = ( a[ai] == '*' || a[ai] == '?' );
    bool b_wild = ( b[bi] == '*' || b[bi] == '?' );

    bool a_bell = bell::is_symbol( a[ai] );
    bool b_bell = bell::is_symbol( b[bi] );

    bool a_set  = ( a[ai] == '[' );
    bool b_set  = ( b[bi] == '[' );

    if ( a_wild && b_wild ) 
      dest += '?';

    else if ( a_wild && b_bell )
      dest += b[bi];
    else if ( b_wild && a_bell )
      dest += a[ai];

    else if ( a_bell && a_bell ) {
      if ( a[ai] == b[bi] ) 
        dest += a[ai];
      else return false;
    }

    else if ( a_wild && b_set ) 
      copy_choices( dest, b, bi );  // bi now refers to ']'
    else if ( b_wild && a_set ) 
      copy_choices( dest, a, ai );  // bi now refers to ']'

    else if ( a_bell && b_set ) {
      if ( !copy_if_choices_contains(dest, b, bi, a[ai]) ) 
        return false;
    }
    else if ( b_bell && a_set ) {
      if ( !copy_if_choices_contains(dest, a, ai, b[bi]) ) 
        return false;
    }

    else if ( a_set && b_set ) {
      if ( !copy_choices_intersection(dest, a, ai, b, bi) )
        return false;
    }

    else RINGING_THROW_OR_RETURN( 
      row_wildcard::invalid_pattern( string(), 
        string("Unknown symbol: ") 
          + ( a_set || a_wild || a_bell ? b[bi] : a[ai] ) ),
      false );


    if ( ai < an ) ++ai;
    if ( bi < bn ) ++bi;
  }

  dest += a.substr(an);
  return true;
}


// Function to provide a brief check if an expression is valid/invalid.
bool row_wildcard::check_expression() const
{
  // check all items are valid.
  int sqbrackets = 0;
  for (string::const_iterator i = pat.begin(); i != pat.end(); i++)
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
                  invalid_pattern(pat, 
                    string("A ") + *i + " is not permitted in []s"), 
                  false );
	      break;
	    case '[':
	      if (sqbrackets)
                RINGING_THROW_OR_RETURN( 
                  invalid_pattern(pat, "Nested [s"), false );
	      sqbrackets = 1;
	      break;
	    case ']':
	      if (sqbrackets != 1)
                RINGING_THROW_OR_RETURN( 
                  invalid_pattern(pat, "Unexpected ']'"), false );
	      sqbrackets = 0;
	      break;
	    default:
              RINGING_THROW_OR_RETURN( 
                invalid_pattern(pat, string("Unknown character: ") + *i),
                false );
	    }
	}
    }
  if (sqbrackets != 0)
    RINGING_THROW_OR_RETURN( 
      invalid_pattern(pat, "Unterminated '['" ), false );
  return true;
}

RINGING_ULLONG row_wildcard::count( unsigned bells ) const
{
  int q = 0;
  if (!bells) bells = b;

  if (!bells)
    RINGING_THROW_OR_RETURN(
      invalid_pattern(pat, "Number of bells unspecified"), 0 );
    
  return possible_matches(bells, 0, pat, q);
}

// This function is subtle: counting the number of possible matches
// is harder than it might seem.  E.g. we need to be able to 
// determine that [12][23][34][14] only actually has two matches --
// once the first bell has been chosen, the rest is forced.
//
// The function is called recursively to iterate through the pattern string
// which is passed in S, position by position, where POS is the offset into
// S of the character we're considering.   When a '[...]' is encountered,
// recurse try each possibility in turn, thus the leading substring of S
// -- i.e. the section [0, POS) -- never contains a [].  When a '*' is 
// encountered, we rewrite it as a sequence of '?'s.  If multiple '*'s are
// present, we try each possibility separately -- and, critically, subtract
// the double-counting due to a single row matching multiple times (see 
// comment below for discussion).  Q keeps track of the number of '?'s 
// already found; we multiply by Q each time we find one with the overall 
// result of Q! for a series of Q '?'s.
//
RINGING_ULLONG row_wildcard::possible_matches
  ( unsigned int bells, unsigned int pos, const string &s, int &q ) const
{
  if (pos >= s.size())
    {
      // Elsewhere (and traditionally in the music_details class) we
      // consider a overly-short pattern without a star (e.g. '5') to
      // have an implicit trailing '*'.  For consistency, lets do that here.
      if ( count_bells(s) < bells )
        return possible_matches( bells, pos, s + '*', q );
      else
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
	  int new_q = ++q;
	  return possible_matches(bells, pos + 1, s, q) * new_q;
	}
      else if (s[pos] == '[')
	{
	  RINGING_ULLONG total = 0;

	  // Replace the [...] with each item in it and 
	  // pass through again.
	  unsigned int lastpos = s.find(']', pos + 1);
	  for (unsigned int i = pos + 1; i < lastpos; i++)
	    {
	      string modified(s, 0, pos);
	      unsigned int newpos = modified.size();
	      // Only do this if the bell isn't in the string already,
              // and if it isn't already in the alternatives group.  Thus,
              // [5][5]* will never match anything, and [55]* doesn't double
              // count rows beginning with 5.
	      if (modified.find(s[i]) == string::npos &&
                  s.find(s[i],pos) == i)
		{
		  modified += s[i];
		  modified += s.substr(lastpos + 1, s.size() - lastpos - 1);

	          int orig_q = q;
		  total += possible_matches(bells, newpos, modified, q);
		  q = orig_q;
		}
	    }

	  return total;
	}
      else if (s[pos] == '*')
	{
          // Two consecutive '*'s should be replaced with a single one
          // as the code to handle the intersection between moving sections
          // is inefficient and this is one of the worst cases.
          if ( s.size() > pos + 1 && s[pos+1] == '*' ) 
            {
              string modified( s, 0, pos );
              modified.append( s, pos + 1, s.size() - pos - 1 );
 
              return possible_matches( bells, pos, modified, q );
            }

          // The largest number of '?'s that this could represent.
          const unsigned n = bells - count_bells(s);

	  if ( s.size() == pos + 1 || s.find('*', pos + 1) == string::npos )
	    {
	      // Pattern has just 1 star, so replace with maximum '?'
	      return possible_matches(bells, pos, expand_star(s, pos, n), q);
	    }
	  else
	    {
       	      // More than 1 star. Replace string with 0, 1, 2... '?'
	      // and calculate at each stage
              //
              // However, just doing this is not sufficient as it would
              // have '*[35]*' match 240 different rows on five bells.
              // The problem arises because we fail to handle the overlap 
              // between '*5*' and '*3*'.  Both correctly match 120 times, 
              // but that does not mean that '*[53]*' should match 120+120 
              // times.
              //
              // When we expand the first '*' in '*[35]*' we sum the matches
              // '[35]*' + '?[35]*' + '??[35]*' + '???[35]*' + '????[35]*'.
              // However, we also need to subtract the intersections
              // between each of the 5*4/2=10 pairs.  For example, the first
              // intersection '[35]*' AND '?[35]*' is '[35][35]*' which 
              // matches 2x6=12 times.  The other nine intersections 
              // contribute the same giving a total of 240-10*12 = 120.
              //
              // However, as a useful optimisation, we can observe that
              // the intersections are all zero iff the fragment of the
              // pattern between the two '*'s includes a fixed bell.   For
              // example, '*5*3*1*' has a fixed bell between each pair of 
              // '*'s and so is correctly calculated (20 on five bells)
              // without calculating the intersection.  As calculating
              // the intersection is O(BELLS^2), this is best avoided 
              // where possible.
              //
              bool need_intersection = true;
              {
                size_t frag_end = s.find('*', pos + 1);
                bool in_bracket = false;
                bool had_fixed_bell = false;
                for ( unsigned i = pos+1; i < frag_end; ++i )
                  if ( s[i] == '[' ) 
                    in_bracket = true;
                  else if ( s[i] == ']' ) 
                    in_bracket = false;
                  else if ( !in_bracket && bell::is_symbol(s[i]) ) 
                    need_intersection = false;
              }

	      RINGING_ULLONG total = 0;

  	      for ( unsigned i = 0; i <= n; ++i )
		{
		  string modified = expand_star(s, pos, i);

	          int orig_q = q;
		  total += possible_matches(bells, pos, modified, q);
		  q = orig_q;

                  for ( unsigned j = 0; need_intersection && j < i; ++j )
                    {
                      string intersect;
                      if ( make_intersection
                             ( intersect, modified, expand_star(s,pos,j) ) )
                        total -= possible_matches( bells, pos, intersect, q );

                      q = orig_q;
                    }
		}

	      return total;
	    }
	}
      else
	{
          RINGING_THROW_OR_RETURN( 
            invalid_pattern(pat, string("Unknown character: ") + s[pos]), 0 );
	}
    }
}

#if RINGING_USE_EXCEPTIONS
row_wildcard::invalid_pattern::invalid_pattern( string const& pat, 
                                                string const& msg ) 
  : invalid_argument( "The pattern '" + pat + "' was invalid: " + msg )
{}
#endif

RINGING_END_NAMESPACE
