// -*- C++ -*- mask.cpp - handle method masks
// Copyright (C) 2002 Richard Smith <richard@ex-parrot.com>

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


RINGING_USING_NAMESPACE
RINGING_USING_STD

bool parse_mask( arguments &args, const arg_parser &ap )
{
  std::string::const_iterator i( args.mask.begin() ), e( args.mask.end() );
  const int num( args.bells );

  const change cross(num,"X"), dummy;

  while ( i != e && isspace( *i ) ) ++i; // Skip whitespace

  vector<change> block0, block1, block2;
  size_t star_index;
  bool star_sym;

  while ( i != e ) // iterate over each block in turn
    {
      vector<change> block;

      bool symblock( *i == '&' );
      if ( symblock )
	{
	  ++i;
	  while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
	  if ( i != e && *i == '.' ) ++i;	 // Skip a "." separator
	  while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
	}

      // Get a change
      bool got_star(false);
      while ( i != e && ( isalnum(*i) || *i == '-' 
			  || *i == '?' || *i == '*' ) )
	{
	  if (*i == 'X' || *i == 'x' || *i == '-') 
	    {
	      ++i;
	      block.push_back(cross);
	    }	  
	  else if (*i == '*')
	    {
	      ++i;
	      block.push_back(dummy);

	      if ( got_star || block1.size() )
		{
		  ap.error( "Mask must contain at most one '*'" );
		  return false;
		}

	      star_index = block.size()-1;
	      star_sym = symblock;
	      got_star = true;
	    }
	  else if (*i == '?')
	    {
	      ++i;
	      block.push_back(dummy);
	    }
	  else
	    {
	      std::string::const_iterator j( i );
	      while ( j != e && ( isalnum(*j) && *j != 'X' && *j != 'x' ) ) 
		++j; // Pass over one change.
	      if ( i != j ) block.push_back( change( num, string( i, j ) ) );
	      i = j;
	    }

	  while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
	  if ( i != e && *i == '.' ) ++i;	 // Skip a "." separator
	  while ( i != e && isspace( *i ) ) ++i; // Skip whitespace
	}

      // Now output the block
      {
	vector<change> &blockx
	  = got_star ? block1 : block1.empty() ? block0 : block2;

	copy( block.begin(), block.end(), back_inserter( blockx ) );
	if ( symblock ) 
	  {
	    block.pop_back();
	    copy( block.rbegin(), block.rend(), back_inserter( blockx ) );
	  }
      }

      if ( i != e ) 
	{
	  if ( *i != ',' ) throw place_notation::invalid(); 
	  ++i; // Skip a "," separator
	  while ( i != e && isspace(*i) ) ++i; // Skip whitespace
	}
    }

  if ( block1.size() )
    {
      if ( star_sym ) assert( block1.size() % 2 );


      size_t desired_size( args.lead_len );
      size_t total_size( block0.size() + block1.size() + block2.size() );

      if ( star_sym && star_index == (block1.size() + 1) / 2 - 1 )
	{
	  total_size -= 1;

	  if ( total_size > desired_size )
	    {
	      ap.error( "The specified mask was too long" );
	      return false;
	    }
	  else if ( (desired_size - total_size) % 2 == 0)
	    {
	      ap.error( "A symmetry-point '*' cannot be used to fill an "
			"even number of chanmes" );
	      return false;
	    }
	  else if ( desired_size - total_size > 1 )
	    {
	      block1.insert( block1.begin() + star_index,
			     (desired_size - total_size - 1), 
			     change() );
	    }
	}
      else if ( star_sym )
	{
	  total_size -= 2;

	  if ( total_size > desired_size )
	    {
	      ap.error( "The specified mask was too long" );
	      return false;
	    }
	  else if ( (desired_size - total_size) % 2 == 1 )
	    {
	      ap.error( "A symmetric '*' cannot be used to fill an "
			"odd number of chanmes" );
	      return false;
	    }
	  else if ( desired_size == total_size )
	    {
	      block1.erase( block1.begin() + block1.size() - 1 - star_index );
	      block1.erase( block1.begin() + star_index );
	    }
	  else if ( desired_size - total_size > 3 )
	    {
	      block1.insert( block1.begin() + block1.size() - 1 - star_index,
			     (desired_size - total_size) / 2 - 1,
			     change() );
	      block1.insert( block1.begin() + star_index,
			     (desired_size - total_size) / 2 - 1,
			     change() );
	    }
	}
      else 
	{
	  total_size -= 1;

	  if ( total_size > desired_size )
	    {
	      ap.error( "The specified mask was too long" );
	      return false;
	    }
	  else if ( desired_size == total_size )
	    {
	      block1.erase( block1.begin() + star_index );
	    }
	  else if ( desired_size - total_size > 1 )
	    {
	      block1.insert( block1.begin() + star_index,
			     desired_size - total_size - 1,
			     change() );
	    }
	}

      assert( block0.size() + block1.size() + block2.size() == desired_size );
    }

  args.required_changes.clear();
  copy( block0.begin(), block0.end(), back_inserter( args.required_changes ) );
  copy( block1.begin(), block1.end(), back_inserter( args.required_changes ) );
  copy( block2.begin(), block2.end(), back_inserter( args.required_changes ) );

  return true;
}

bool is_mask_consistent( arguments &args, const arg_parser &ap )
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

	  change &ch1 = args.required_changes[depth];
	  change &ch2 = args.required_changes
	    [ ( depth > hl_len ? 3 : 1 ) * hl_len 
	      - args.hunt_bells % 2 * 2 - depth ];

	  if ( ch1.bells() && ch2.bells() )
	    {
	      if ( ch1 != ch2.reverse() )
		return false;
	    }
	  else if ( ch1.bells() ) 
	    ch2 = ch1.reverse();
	  else if ( ch2.bells() )
	    ch1 = ch2.reverse();
	}

      if ( args.doubsym )
	{
	  change &ch1 = args.required_changes[depth];
	  change &ch2 = args.required_changes
	    [ (depth + hl_len) % (2 * hl_len) ];

	  if ( ch1.bells() && ch2.bells() )
	    {
	      if ( ch1 != ch2.reverse() )
		return false;
	    }
	  else if ( ch1.bells() ) 
	    ch2 = ch1.reverse();
	  else if ( ch2.bells() )
	    ch1 = ch2.reverse();
	}

      if ( args.sym )
	{
	  if ( 2 * (hl_len - args.hunt_bells % 2) - depth < 0 || 
	       2 * (hl_len - args.hunt_bells % 2) - depth >= 2 * hl_len )
	    continue;

	  change &ch1 = args.required_changes[depth];
	  change &ch2 = args.required_changes
	    [ 2 * (hl_len - args.hunt_bells % 2) - depth ];

	  if ( ch1.bells() && ch2.bells() )
	    {
	      if ( ch1 != ch2 )
		return false;
	    }
	  else if ( ch1.bells() ) 
	    ch2 = ch1;
	  else if ( ch2.bells() )
	    ch1 = ch2;
	}
    }

  return true;
}
