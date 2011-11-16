// -*- C++ -*- mask.cpp - handle method masks
// Copyright (C) 2002, 2003, 2009, 2011 Richard Smith <richard@ex-parrot.com>

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

// Turn this on for mask debugging:
#define RINGING_DEBUG_FILE 0

#include "args.h"
#include "prog_args.h"
#include "methodutils.h"
#include "mask.h"
#include <string>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <algo.h>
#else
#include <vector>
#include <algorithm>
#endif
#if RINGING_DEBUG_FILE
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#else
#include <iostream> // for cout
#endif
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <assert.h>
#include <string.h>
#else
#include <cctype>
#include <cassert>
#include <cstring> // for strchr
#endif
#include <ringing/row.h>
#include <ringing/extent.h>
#include <ringing/streamutils.h>
#include <ringing/mathutils.h>
#include <ringing/place_notation.h>

#if RINGING_DEBUG_FILE
#define DEBUG( expr ) (void)((cout << expr) << endl)
#else
#define DEBUG( expr ) (void)(false)
#endif

RINGING_USING_NAMESPACE
RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

// return positions of the (lowest, highest) hunt bells
// 0 = moving 1-2
// 1 = moving 2-3 ...
pair< int, int > get_posn2( const arguments &args, int depth )
{
  int div_len( (1 + args.treble_dodges) * 2 );
  int hl_len( args.lead_len / 2 );

  bool first_hl( depth < hl_len );

  int posn = first_hl ? depth : 2*hl_len - depth - 2;
  
  if ( posn == -1 )
    ; // lead-end change
  else if ( posn % div_len == div_len - 1 )
    posn = posn / div_len * 2 + 1;
  else
    posn = posn / div_len * 2;

  if ( args.hunt_bells == 1 )
    return make_pair( posn, posn );

  int a = posn + ( args.hunt_bells - (first_hl ? 1 : 0) ) / 2 * 2;
  int b = posn - ( args.hunt_bells - (first_hl ? 0 : 1) ) / 2 * 2;


  if ( a > args.bells-1 ) a = args.bells-1;
  if ( b < -1 ) b = -2 - b;

  // Can this happen?
  if ( b > a ) swap( a, b );
  if ( a < posn ) a = posn;
  if ( b > posn ) b = posn;

  return make_pair( b, a );
}

struct block 
{
  block() : is_sym(false), got_star(false) {}

  vector< vector<change> > data;
  bool is_sym;
  bool got_star;
  int star_idx;
};
  
class mask_error : public invalid_argument {
public:
  mask_error( const string& s ) : invalid_argument(s) {}
};
  
shared_pointer<block> read_block( const int num,
				  std::string::const_iterator &i, 
				  const std::string::const_iterator &e )
{
  const change cross(num,"X"), dummy;

  while ( i != e && isspace( *i ) ) ++i; // Skip whitespace    

  if ( i == e ) 
    throw mask_error( "Missing block in mask" ); 

  shared_pointer<block> rv( new block );
  rv->is_sym = (*i == '&');
  if ( *i == '&' || *i == '+' )
    {
      ++i;
      while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
      if ( i != e && *i == '.' ) ++i;	 // Skip a "." separator
      while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
    }

  while ( i != e && ( bell::is_symbol(*i) || strchr("Xx-?*(", *i) ) )
    {
      switch (*i)
	{
	case '(':
	  rv->data.push_back( vector<change>() );
	  while ( i != e && *i != ')' )
	    {
	      ++i; // Skip over '|' or '('
	      std::string::const_iterator j( i );
	      while ( j != e && *j != ')' && *j != '|' )
		++j; // Pass over one change.
	      if ( i != j ) 
		rv->data.back().push_back( change( num, string( i, j ) ) );
	      i = j;
	      if ( i == e )
		throw mask_error( "Unterminated '(' in mask" );
	      else if ( *i != '|' && *i != ')' )
		throw mask_error( "Malformed list of alternatives in mask" );
	    }
	  ++i;
	  if ( rv->data.back().empty() )
	    throw mask_error( "Mask contains empty alternative list" );
	  break;

	case 'X': case 'x': case '-':
	  ++i;
	  rv->data.push_back( vector<change>(1u,cross) );
	  break;

	case '*':
	  ++i;
	  rv->data.push_back( vector<change>() ); // a dummy entry
	  // this will be expanded to the correct size later

	  if ( rv->got_star )
	    throw mask_error( "Mask must contain at most one '*'" );

	  rv->got_star = true;
	  rv->star_idx = rv->data.size()-1;
	  break;
	    
	case '?':
	  ++i;
	  rv->data.push_back( vector<change>() );
	  break;

	default:
	  {
	    std::string::const_iterator j( i );
	    while ( j!=e && ( bell::is_symbol(*j) && *j != 'X' && *j != 'x' ) ) 
	      ++j; // Pass over one change.
	    if ( i != j ) 
	      rv->data.push_back
		( vector<change>( 1u, change( num, string( i, j ) ) ) );
	    i = j;
	  }
	  break;
	}
	
      while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
      if ( i != e && *i == '.' ) ++i;	     // Skip a "." separator
      while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
    }
   
  if ( rv->is_sym )
    {
      rv->data.reserve( 2*rv->data.size() );
      copy( rv->data.rbegin() + 1, rv->data.rend(),
	    back_inserter( rv->data ) );
    }

  return rv;
}

// Convert a '*' to a sequences of '?'s
void expand_block( vector< vector< change > >& block1, 
		   bool star_sym, size_t star_index, size_t expand_by )
{
  if ( star_sym ) 
    assert( block1.size() % 2 );

  // A block like &-8-8-*
  if ( star_sym && star_index == (block1.size() + 1) / 2 - 1 )
    {
      expand_by += 1;
	
      if ( expand_by < 0 )
	throw mask_error( "The specified mask was too long" );
	
      else if ( expand_by % 2 == 0)
	throw mask_error( "A symmetry-point '*' cannot be used to fill an "
			  "even number of chanmes" );
	
      else if ( expand_by > 1 )
	block1.insert( block1.begin() + star_index, 
		       expand_by - 1, vector<change>() );
    }
  // A block like &3.*.5.1.5
  else if ( star_sym )
    {
      expand_by += 2;
	
      if ( expand_by < 0 )
	throw mask_error( "The specified mask was too long" );
	
      else if ( expand_by % 2 == 1 )
	throw mask_error( "A symmetric '*' cannot be used to fill an "
			  "odd number of chanmes" );
	
      else if ( expand_by == 0 )
	{
	  block1.erase( block1.begin() + block1.size() - 1 - star_index );
	  block1.erase( block1.begin() + star_index );
	}
      else if ( expand_by > 3 )
	{
	  block1.insert( block1.begin() + block1.size() - 1 - star_index,
			 expand_by / 2 - 1, vector<change>() );
	  block1.insert( block1.begin() + star_index,
			 expand_by / 2 - 1, vector<change>() );
	}
    }
  // An asymmetric block
  else 
    {
      expand_by += 1;
	
      if ( expand_by < 0 )
	throw mask_error( "The specified mask was too long" );

      else if ( expand_by == 0 )
	block1.erase( block1.begin() + star_index );

      else if ( expand_by > 1 )
	block1.insert( block1.begin() + star_index,
		       expand_by - 1, vector<change>() );
    }
}

bool read_and_expand_blocks( arguments& args, const arg_parser& ap,
                             vector< vector<change> >& above, 
                             vector< vector<change> >& below )
{
  // If lead_len is unspecified, we use length of two (to accommodate -w) 
  // and fix it up when reading this in the search algorithm.
  size_t const lead_len = args.lead_len ? args.lead_len.get() : 2;

  // Block0 are those sections before the section containing a star
  // Block1 is the section containing a star
  // Block2 are those sections after the section containing a star
  // 'b' blocks are below works for above/below sections
  vector< vector<change> > &block0a = above, &block0b = below;
  vector< vector<change> >  block1a,          block1b;
  vector< vector<change> >  block2a,          block2b;

  size_t star_index_a(0u),  star_index_b(0u);
  bool   star_sym_a(false), star_sym_b(false);

  // ----------------------------------
  // Parse the mask
  {
    std::string::const_iterator i( args.mask.begin() ), e( args.mask.end() );
    
    while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
    
    while ( i != e ) // iterate over each block in turn
      {
	// block is the current block being parsed.
	// if currently parsing a below section, blocka contains 
	// the above section

	shared_pointer<block> blka( read_block(args.bells, i, e) ), blkb;
	
	bool got_star(false); // Is block of unknown length?
	
	// Is block an above section?
	if ( i != e && *i == '/' )
	  {
	    if ( !args.hunt_bells )
	      throw mask_error( "Masks for principles may not contain "
				"above and below sections" );

	    ++i; // skip '/'
	    blkb = read_block(args.bells, i, e);
	    
	    if ( i != e && *i == '/' )
	      throw mask_error( "Block may contain at most one '/'" );
	    
	    if ( !blka->got_star && !blkb->got_star )
	      {
		if ( blka->data.size() != blkb->data.size() ) 
		  throw mask_error
		    ("Above and below blocks are of different sizes" );
	      }
	    else if ( blka->got_star && blkb->got_star )
	      {
		if ( block1a.size() )
		  throw mask_error( "Mask must contain at most one section of "
				    "unknown length" );
		
		star_index_a = blka->star_idx;  star_sym_a = blka->is_sym;
		star_index_b = blkb->star_idx;  star_sym_b = blkb->is_sym;
		got_star = true;
	      }
	    else if ( blka->got_star )
	      {
		expand_block( blka->data, blka->is_sym, blka->star_idx,
			      blkb->data.size() - blka->data.size() );
		assert( blka->data.size() == blkb->data.size() );
	      }
	    else if ( blkb->got_star )
	      {
		expand_block( blkb->data, blkb->is_sym, blkb->star_idx,
			      blka->data.size() - blkb->data.size() );
		assert( blka->data.size() == blkb->data.size() );
	      }
	  }
	else
	  {
	    if ( blka->got_star )
	      {
		if ( block1a.size() )
		  throw mask_error( "Mask must contain at most one section of "
				    "unknown length" );
		
		star_index_a = star_index_b = blka->star_idx;  
		star_sym_a   = star_sym_b   = blka->is_sym;
		got_star = true;
	      }
	    
	    blkb = blka;
	  }
	
	// Now append the blocks to the collections of blocks
	{
	  vector< vector<change> > &blockxa
	    = got_star ? block1a : block1a.empty() ? block0a : block2a;
	  
	  copy( blka->data.begin(), blka->data.end(), 
		back_inserter( blockxa ) );
	}
	{
	  vector< vector<change> > &blockxb
	    = got_star ? block1b : block1b.empty() ? block0b : block2b;
	  
	  copy( blkb->data.begin(), blkb->data.end(), 
		back_inserter( blockxb ) );
	}
	
	if ( i != e ) 
	  {
	    if ( *i != ',' ) throw place_notation::invalid(); 
	    ++i; // Skip a "," separator
	    while ( i != e && isspace(*i) ) ++i; // Skip whitespace
	  }
      }
  }

  // ----------------------------------
  // Expand * to a sequence of ?s

  assert( block0a.size()  == block0b.size()  );
  assert( block1a.empty() == block1b.empty() );
  assert( block2a.size()  == block2b.size()  );

  if ( block1a.size() && block1b.size() )
    {
      // We have a section of unknown length

      expand_block( block1a, star_sym_a, star_index_a, lead_len -
		    block0a.size() - block1a.size() - block2a.size() );

      expand_block( block1b, star_sym_b, star_index_b, lead_len -
		    block0b.size() - block1b.size() - block2b.size() );

      assert( block0a.size() + block1a.size() + block2a.size()
	      == size_t(lead_len) );

      assert( block0b.size() + block1b.size() + block2b.size()
	      == size_t(lead_len) );
    }
  else 
    {
      if ( block0a.size() + block2a.size() != size_t(lead_len) )
	throw mask_error( make_string() << "The mask was of the wrong "
			  "length: found " << (block0a.size() + block2a.size())
			  << " changes; expected " << lead_len );
    }
  

  args.allowed_changes.clear();
  copy( block1a.begin(), block1a.end(), back_inserter( block0a ) );
  copy( block1b.begin(), block1b.end(), back_inserter( block0b ) );
  copy( block2a.begin(), block2a.end(), back_inserter( block0a ) );
  copy( block2b.begin(), block2b.end(), back_inserter( block0b ) );

  return true;
}

void select_changes_above( const arguments& args, vector<change>& changesa,
                           int i )
{
  int active_above;
  {
    const pair< int, int > posn( get_posn2(args, i) );
    if ( posn.second == args.treble_back - args.treble_front )
      active_above = args.bells - args.treble_back;
    else
      active_above = args.bells-2 - (posn.second + args.treble_front-1);
    if (active_above < 0) active_above=0;
  }
  DEBUG("At position " << i << ", " << active_above << " bells active above");

  changesa.reserve( fibonacci(active_above) );

  // Choose the work above the treble
  for ( changes_iterator 
          j(active_above, args.bells-active_above, args.bells), e; 
        j != e; ++j )
    {
      change a(*j); 

      // Handle -w
      if ( args.right_place && i % 2 == args.bells % 2
           && args.bells - a.count_places() != active_above )
        continue;
      
      // Handle -f
      if ( args.no_78_pns && active_above > 1 && a.findplace(args.bells-2) )
        continue;

      changesa.push_back(a);
    }
}

void select_changes_below( const arguments& args, vector<change>& changesb,
                           int i )
{
  int active_below = args.treble_front-1;
  {
    const pair< int, int > posn( get_posn2(args, i) );
    active_below += posn.first == -1 ? 0 : posn.first;
  }
  DEBUG("At position " << i << ", " << active_below << " bells active below");

  changesb.reserve( fibonacci(active_below) );
    
  for ( changes_iterator j(active_below, 0, args.bells), e; j != e; ++j )
    {
      change b(*j); 
      
      // Handle -w
      if ( args.right_place && i % 2 == 0
           && args.bells - b.count_places() != active_below )
        continue;

      // Handle -kf or -df
      if ( (args.skewsym || args.doubsym || args.mirrorsym) 
           && args.no_78_pns 
           && active_below > 1 && b.findplace(1) )
        continue;
      
      changesb.push_back(b);
    }
}

void select_changes_prin( const arguments& args, vector<change>& changes, 
                          int i )
{
  if ( args.right_place && args.bells % 2 == 0 && i % 2 == 0 )
    {
      // Handle -w
      changes.push_back( change( args.bells, "-" ) );
    }

  else 
    {
      changes.reserve( fibonacci( args.bells ) );
    
      for ( changes_iterator j(args.bells), e; j != e; ++j )
        {
          change ch(*j);
   
          // Handle -Fx
          if ( args.true_trivial && ch.count_places() == args.bells )
            continue;
  
          // Handle --changes
          if ( args.changes.size() && args.include_changes
               != ( args.changes.find(ch) != args.changes.end() ) )
            continue;
    
          // Handle -f
          if ( args.no_78_pns && ch.findplace(args.bells-2) )
            continue;
    
          // Handle -kf or -df
          if ( (args.skewsym || args.doubsym) && args.no_78_pns 
               && ch.findplace(1) )
            continue;
    
          // Handle -l
          if ( args.max_places_per_change 
               && ch.count_places() > args.max_places_per_change )
            continue;
    
          // Handle -j
          if ( args.max_consec_places
               && has_consec_places( ch, args.max_consec_places ) )
            continue;
    
          // Handle --mirror
          if ( args.mirrorsym && ch != ch.reverse() )
            continue;
    
          changes.push_back(ch);
        }
   }
}

change merge_changes( const arguments& args,
                      const change& a, const change& b, 
		      const pair<int, int>& posn ) 
{
  change c( a.bells() );

  for ( int i=0; i<posn.first && i < a.bells()-1; ++i )
    if ( b.findswap(i) )
      c.swappair(i++);

  for ( int i=posn.first; i<posn.second && i < a.bells()-1; ++i )
    c.swappair(i++);

  for ( int i=posn.second; i < a.bells()-1; ++i )
    if ( a.findswap(i) )
      c.swappair(i++);

  return c;
}

void merge_changes( const arguments& args, vector<change>& result,
                    const vector<change>& above, const vector<change>& below,
                    int i )
{
  result.reserve( above.size() * below.size() );
  pair< int, int > posn( get_posn2(args, i) );
  if ( posn.first == -1 ) { posn.first = 0; }
  if ( posn.second == args.treble_back - args.treble_front ) --posn.second;
  posn.first  += args.treble_front - 1;
  posn.second += args.treble_front;

  // POSN now contains the position of the lowest and highest hunts
  // For example, in twin-hunt doubles, we have:
  // (0,1), (0,2), (0,3), (1,4), (2,4), (3,4), (2,4), (1,4), (0,3), (0,2)

  // If we're leading or lying, then handle that.
  // XXX:  This doesn't work when number of hunts > 1/2 number of bells
  if ( posn.first == args.treble_front-1
       && i % (2 * args.treble_dodges + 2) == 2 * args.treble_dodges + 1 ) 
    posn.first++;

  if ( posn.second == args.treble_back-1 
       && (    (args.treble_back - args.treble_front) % 2 == 1
            && i % (2 * args.treble_dodges + 2) == 2 * args.treble_dodges + 1 
            || (args.treble_back - args.treble_front) % 2 == 0
            && i % 2 == 0 ) ) 
    posn.second--;

  // POSN now contains the position of the lowest and highest moving hunts
  DEBUG( "At position " << i << " hunts in range " 
           << posn.first << "-" << posn.second );

  for ( int ia=0, na=above.size(); ia < na; ++ia )
    for ( int ib=0, nb=below.size(); ib < nb; ++ib )
      {
	const change ch( merge_changes( args, above[ia], below[ib], posn ) );
        DEBUG( "Merging " << below[ib] << " and " << above[ia] << " gives "
                 << ch );

        // Handle --changes
        if ( args.changes.size() && args.include_changes
             != ( args.changes.find(ch) != args.changes.end() ) )
          continue;
	
	// Handle -l
	if ( args.max_places_per_change 
	     && ch.count_places() > args.max_places_per_change )
	  continue;

	// Handle -j
	if ( args.max_consec_places
	     && has_consec_places( ch, args.max_consec_places ) )
	  continue;

        // Handle --mirror
        if ( args.mirrorsym && ch != ch.reverse() )
          continue;
	
	result.push_back(ch);
      }
}
                         

vector<change> reverse_allowed( const vector<change>& ch )
{
  vector<change> rv; rv.reserve(ch.size());
  for ( vector<change>::const_iterator i(ch.begin()), e(ch.end());
	i != e;  ++i )
    rv.push_back( i->reverse() );
  sort( rv.begin(), rv.end() );
  return rv;
}

bool unordered_equal( const vector<change>& a, const vector<change>& b )
{
  for ( vector<change>::const_iterator i(a.begin()), e(a.end());
	i != e;  ++i )
    if ( find( b.begin(), b.end(), *i ) == b.end() )
      return false;
  return true;
}

bool is_mask_consistent( arguments &args, 
			 const vector<vector<change> > &above,
			 const vector<vector<change> > &below )
{
  assert( args.lead_len );
  const int hl_len = args.lead_len / 2;

  for ( int depth=0; depth < 2*hl_len; ++depth )
    {
      if ( args.skewsym )
	{
	  const int depth2( ( depth > hl_len ? 3 : 1 ) * hl_len
			    - args.hunt_bells % 2 * 2 - depth );

	  if ( depth2 < 0 || depth2 >= hl_len*2 || depth2 == depth )
	      continue;

	  // FIXME Quarter-lead change

	  int other_index( ( depth > hl_len ? 3 : 1 ) * hl_len 
			   - args.hunt_bells % 2 * 2 - depth );

	  const vector<change> &ch1a = above[depth];
	  const vector<change> &ch1b = below[depth];
	  const vector<change> &ch2a = above[other_index];
	  const vector<change> &ch2b = below[other_index];

	  if ( ch1a.size() && ch2b.size() &&
	       !unordered_equal( ch1a, reverse_allowed(ch2b) ) )
	    return false;

	  if ( ch1b.size() && ch2a.size() &&
	       !unordered_equal( ch1b, reverse_allowed(ch2a) ) )
	    return false;
	}

      if ( args.doubsym )
	{
	  int other_index( (depth + hl_len) % (2 * hl_len) );

	  const vector<change> &ch1a = above[depth];
	  const vector<change> &ch1b = below[depth];
	  const vector<change> &ch2a = above[other_index];
	  const vector<change> &ch2b = below[other_index];

	  if ( ch1a.size() && ch2b.size() &&
	       !unordered_equal( ch1a, reverse_allowed(ch2b) ) )
	    return false;

	  if ( ch1b.size() && ch2a.size() &&
	       !unordered_equal( ch1b, reverse_allowed(ch2a) ) )
	    return false;
	}

      if ( args.sym )
	{
	  if ( 2 * (hl_len - args.hunt_bells % 2) - depth < 0 || 
	       2 * (hl_len - args.hunt_bells % 2) - depth >= 2 * hl_len )
	    continue;

	  int other_index( 2 * (hl_len - args.hunt_bells % 2) - depth );
          assert( other_index >= 0 ); assert( other_index <= above.size() );

	  const vector<change> &ch1a = above[depth];
	  const vector<change> &ch1b = below[depth];
	  const vector<change> &ch2a = above[other_index];
	  const vector<change> &ch2b = below[other_index];

	  if ( ch1a.size() && ch2a.size() &&
	       !unordered_equal( ch1a, ch2a ) )
	    return false;

	  if ( ch1b.size() && ch2b.size() &&
	       !unordered_equal( ch1b, ch2b ) )
	    return false;
	}
    }

  return true;
}

void changes_have_double_symmetry( arguments& args )
{
  vector<change> asymmetries;
  for (set<change>::const_iterator 
         i=args.changes.begin(), e=args.changes.end(); i != e; ++i )
    if (args.changes.find(i->reverse()) == args.changes.end())
      asymmetries.push_back(*i);

  for (vector<change>::const_iterator
         i=asymmetries.begin(), e=asymmetries.end(); i != e; ++i )
    if (args.include_changes)
      args.changes.erase(*i);
    else
      args.changes.insert(i->reverse());
}

RINGING_END_ANON_NAMESPACE

void restrict_changes( arguments& args )
{
  if (args.changes.size()) {
    if (args.skewsym || args.doubsym)
      changes_have_double_symmetry(args);
    if (args.changes.empty())
      throw mask_error("The --changes argument is not compatible "
                       "with the symmetries requested\n");
  }
}

bool parse_mask( arguments &args, const arg_parser &ap )
{
  vector< vector<change> > above, below;

  if ( !read_and_expand_blocks( args, ap, above, below ) )
    return false;

  if ( args.lead_len && !is_mask_consistent(args, above, below) )
    throw mask_error
      ( "Some of the required changes specified are inconsistent "
	"with the specified symmetries" );

  // ----------------------------------
  // Expand ? to appropriate alternative lists
  size_t const lead_len = args.lead_len ? args.lead_len.get() : 2;
  assert( above.size() == size_t(lead_len) );
  assert( below.size() == size_t(lead_len) );


  for ( int i=0; i<lead_len; ++i )
    {
      args.allowed_changes.push_back( vector<change>() );
      vector<change>& changes_to_try = args.allowed_changes.back();

      if ( args.hunt_bells )
	{
	  vector<change>& changesa = above[i];
	  vector<change>& changesb = below[i];
 
          // A ? above or below the treble
          if ( changesa.empty() ) 
            select_changes_above( args, changesa, i );
          if ( changesb.empty() ) 
            select_changes_below( args, changesb, i );
 
          merge_changes( args, changes_to_try, changesa, changesb, i );
	}
      else // principles
	{
	  if ( above[i].empty() )
	    select_changes_prin( args, changes_to_try, i );
	  else
	    copy( above[i].begin(), above[i].end(), 
	          back_inserter( changes_to_try ) ); 
	}

      sort( changes_to_try.begin(), changes_to_try.end(), &compare_changes );
      changes_to_try.erase
	( unique( changes_to_try.begin(), changes_to_try.end() ),
	  changes_to_try.end() );

      if ( changes_to_try.empty() )
	throw mask_error( make_string() << "No such method can exist: "
			  "There no possible changes at position " << i );
    }

  return true;
}
