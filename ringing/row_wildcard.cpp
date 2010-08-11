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

RINGING_END_ANON_NAMESPACE

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

unsigned row_wildcard::count( unsigned bells ) const
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
// encountered, we rewrite it as a sequence of '?'s; if multiple '*'s are
// present, we try each possibility separate (but see below for discussion).
// Q keeps track of the number of '?'s already found; we multiply by Q each
// time we find one with the overall result of Q! for a series of Q '?'s.
//
unsigned int row_wildcard::possible_matches
  ( unsigned int bells, unsigned int pos, const string &s, int &q ) const
{
  if (pos >= s.size())
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
	  int new_q = ++q;
	  return possible_matches(bells, pos + 1, s, q) * new_q;
	}
      else if (s[pos] == '[')
	{
	  // Replace the [...] with each item in it and 
	  // pass through again.
	  unsigned int lastpos = s.find(']', pos + 1);
	  unsigned int total = 0;
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
          // The largest number of '?'s that this could represent.
          const unsigned n = bells - count_bells(s);

	  if ( s.size() == pos + 1 || s.find('*', pos + 1) == string::npos )
	    {
	      // Just 1 star, therefore replace with maximum ?
	      string modified( s, 0, pos );
              modified.append( n, '?' );
	      modified.append( s, pos + 1, s.size() - pos - 1 );

	      return possible_matches(bells, pos, modified, q);
	    }
	  else
	    {
       	      // More than 1 star. Replace string with 0, 1, 2... '?'
	      // and calculate at each stage

              // FIXME:  This code is buggy:  on five bells, '*[35]*' matches
              // 240 different rows!  The problem is because we fail to
              // handle the overlap between '*5*' and '*3*'.  Both correctly 
              // match 120 times, but that does not mean that '*[53]*' should
              // match 120+120 times.

	      unsigned int total = 0;

  	      for (unsigned i = 0; i <= n; ++i )
		{
		  string modified( s, 0, pos );
		  modified.append( i, '?' );
                  modified.append( s, pos + 1, s.size() - pos - 1);

	          int orig_q = q;
		  total += possible_matches(bells, pos, modified, q);
		  q = orig_q;
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
