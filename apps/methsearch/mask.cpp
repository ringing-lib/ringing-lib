// -*- C++ -*- mask.cpp - handle method masks
// Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

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
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <assert.h>
#else
#include <cctype>
#include <cassert>
#endif
#include <ringing/row.h>
#include <ringing/extent.h>


RINGING_USING_NAMESPACE
RINGING_USING_STD

RINGING_START_ANON_NAMESPACE

pair< int, int > get_posn2( arguments &args, int depth )
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


  if ( a > args.bells-1 ) a = -2 + 2 * args.bells - a;
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
  mask_error( const char* s ) : invalid_argument(s) {}
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

  while ( i != e && ( isalnum(*i) || *i == '-' 
		      || *i == '?' || *i == '*' || *i == '(' ) )
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
	    while ( j != e && ( isalnum(*j) && *j != 'X' && *j != 'x' ) ) 
	      ++j; // Pass over one change.
	    if ( i != j ) 
	      rv->data.push_back
		( vector<change>( 1u, change( num, string( i, j ) ) ) );
	    i = j;
	  }
	  break;
	}
	
      while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
      if ( i != e && *i == '.' ) ++i;	 // Skip a "." separator
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

void expand_block( vector< vector< change > >& block1, 
		   bool star_sym, int star_index, int expand_by )
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
  // A block like &*.5.1.5
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

change merge_changes( const change& a, const change& b, 
		      const pair<int, int>& posn ) 
{
  change c( a.bells() );

  {
    for ( int i=0; i< (posn.first == -1 ? 1 : posn.first) - 1; ++i )
      if ( b.findswap(i) )
	c.swappair(i);
  }
  {
    for ( int i=(posn.first == -1 ? 1 : posn.first); 
	  i <= posn.second && i < a.bells()-1; i+=2 )
      c.swappair(i);
  }
  {
    for ( int i=posn.second+2; i < a.bells()-1; ++i )
      if ( a.findswap(i) )
	c.swappair(i);
  }

  return c;
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
  const int hl_len = args.bells * (1 + args.treble_dodges);

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

RINGING_END_ANON_NAMESPACE

bool parse_mask( arguments &args, const arg_parser &ap )
{
  // Block0 are those sections before the section containing a star
  // Block1 is the section containing a star
  // Block2 are those sections after the section containing a star
  // 'b' blocks are below works for above/below sections
  vector< vector<change> > block0a, block0b;
  vector< vector<change> > block1a, block1b;
  vector< vector<change> > block2a, block2b;

  size_t star_index_a, star_index_b;
  bool   star_sym_a,   star_sym_b;

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

  if ( block1a.size() )
    {
      expand_block( block1a, star_sym_a, star_index_a, args.lead_len -
		    block0a.size() - block1a.size() - block2a.size() );

      assert( block0a.size() + block1a.size() + block2a.size()
	      == args.lead_len );
    }

  if ( block1b.size() )
    {
      expand_block( block1b, star_sym_b, star_index_b, args.lead_len -
		    block0b.size() - block1b.size() - block2b.size() );

      assert( block0b.size() + block1b.size() + block2b.size()
	      == args.lead_len );
    }

  args.allowed_changes.clear();
  copy( block1a.begin(), block1a.end(), back_inserter( block0a ) );
  copy( block1b.begin(), block1b.end(), back_inserter( block0b ) );
  copy( block2a.begin(), block2a.end(), back_inserter( block0a ) );
  copy( block2b.begin(), block2b.end(), back_inserter( block0b ) );
  assert( block0a.size() == args.lead_len );
  assert( block0b.size() == args.lead_len );

  if (!is_mask_consistent(args, block0a, block0b))
    throw mask_error
      ( "Some of the required changes specified are inconsistent "
	"with the specified symmetries" );

  // ----------------------------------
  // Expand ? to appropriate alternative lists


  for ( int i=0, n=args.lead_len; i<n; ++i )
    {
      args.allowed_changes.push_back( vector<change>() );
      vector<change>& changes_to_try = args.allowed_changes.back();

      // TODO  Errors if not compatible with this:
      if ( args.right_place && args.bells % 2 == 0 && i % 2 == 0 )
	{
	  changes_to_try.push_back( change( args.bells, "-" ) );
	  continue;
	}

      const pair< int, int > posn( get_posn2(args, i) );
      
      vector<change>& changesa = block0a[i];
      vector<change>& changesb = block0b[i];

      if ( changesa.empty() ) 
	{
	  int active_above( posn.second == args.bells-1 
			    ? 0 : args.bells-2 - posn.second );

	  changesa.reserve( fibonacci(active_above) );

	  // Choose the work above the treble
	  for ( changes_iterator 
		  j(active_above, args.bells-active_above, args.bells), e; 
		j != e; ++j )
	    {
	      if ( args.right_place && args.bells % 2 == 1 
		   && posn.second % 2 == 1
		   && args.bells - j->count_places() != active_above )
		continue;
	      
	      change above(*j); 

	      if ( args.no_78_pns && posn.second < args.bells-3 && 
		   above.findplace(args.bells-2) )
		continue;

	      changesa.push_back(above);
	    }
	}

      if ( changesb.empty() )
	{
	  int active_below( posn.first == -1 ? 0 : posn.first );

	  for ( changes_iterator j(active_below, 0, args.bells), e; 
		j != e; ++j )
	    {
	      if ( args.right_place && args.bells % 2 == 1 
		   && posn.second % 2 == 0
		   && args.bells - j->count_places() != active_below )
		continue;

	      change below(*j); 
	      
	      if ( (args.skewsym || args.doubsym) && args.no_78_pns 
		   && posn.first > 1 && below.findplace(1) )
		continue;

	      changesb.push_back(below);
	    }
	}

      // Merge changes, and push onto args.allowed_changes

      changes_to_try.reserve( changesa.size() * changesb.size() );

      for ( int ia=0, na=changesa.size(); ia < na; ++ia )
      for ( int ib=0, nb=changesb.size(); ib < nb; ++ib )
	{
	  const change ch( merge_changes( changesa[ia], changesb[ib], posn ) );

	  if ( args.max_places_per_change 
	       && ch.count_places() > args.max_places_per_change )
	    continue;

	  changes_to_try.push_back(ch);
	}

      sort( changes_to_try.begin(), changes_to_try.end() );
      changes_to_try.erase
	( unique( changes_to_try.begin(), changes_to_try.end() ),
	  changes_to_try.end() );

      if ( changes_to_try.empty() ) {
	cout << "Error: " << i << endl;
	throw mask_error( "No such method can possibly exist" );
      }
    }

  return true;
}
