// -*- C++ -*- search.cpp - the actual search algorithm
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
#include "methodutils.h"
#include "search.h"
#include "falseness.h"
#include "format.h"
#include "prog_args.h"
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <algo.h>
#else
#include <vector>
#include <algorithm>
#endif
#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#else
#include <cassert>
#endif
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/extent.h>


RINGING_USING_NAMESPACE
RINGING_USING_STD

class searcher
{
private:
  friend void run_search( const arguments &args, const method &initm );

  searcher( const arguments &args, const method &initm )
    : args(args),
      bells( args.bells ),
      div_len( (1 + args.treble_dodges) * 2 ),
      hl_len( args.lead_len / 2 ),
      search_count( 0ul )
  {
    m.reserve( 2 * hl_len );
    first_meth.reserve( initm.size() );
    copy( initm.rbegin(), initm.rend(),
	  back_inserter( first_meth ) );
  }

  void general_recurse();

  inline void call_recurse( const change &ch );
  inline int get_posn();

  inline bool is_cyclic_hl( const row& hl );
  inline bool is_rev_cyclic_hl( const row& hl );
  inline bool is_regular_hl( const row& hl );
  inline void swap_overlap( change &ch, const pair< int, int > &posn );

  void new_midlead_change();
  void new_principle_change();
  void double_existing();

  bool try_halflead_change( const change &ch );
  bool try_leadend_change( const change &ch );
  bool try_leadend_sym_change( const change &ch );
  bool try_halflead_sym_change( const change &ch );
  bool try_midlead_change( const change &ch );
  bool try_quarterlead_change( const change &ch );
  bool try_offset_start_change( const change &ch);

  bool try_principle_symmetry();

  void found_method();
  void maybe_found_method();

  bool is_acceptable_leadhead( const row &lh );

private:
  const arguments &args; 
  const int bells;
  const int div_len;
  const int hl_len;

  unsigned long search_count;

  vector< change > first_meth;  // To allow searches to be resumed
  method m;
};

void run_search( const arguments &args, const method &initm )
{
  searcher s( args, initm );
  s.general_recurse();
  assert( s.m.length() == 0 );
  if ( args.count )
    {
      if ( args.status ) clear_status();
      if ( !args.quiet ) cout << "\n";
      output_count( s.search_count );
    }
}

void searcher::found_method()
{
  // Add it to the histogram
  if ( args.histogram )
    {
      args.H_fmt.add_method_to_stats( m );
    }

  if ( !args.quiet )
    {
      if ( args.status )
	clear_status();

      args.R_fmt.print_method( m, cout ); 
    }

  ++search_count;
}

bool searcher::try_principle_symmetry()
{
  if ( args.skewsym )
    if ( ! has_rotational_symmetry(m) )
      return false;

  return true;
}

void searcher::maybe_found_method()
{
  if ( !args.hunt_bells )
    if ( ! try_principle_symmetry() )
      return;

  if ( args.require_offset_cyclic )
    {
      row r(bells);
      for (int i=0; i< div_len-2; ++i) r *= m[i];

      // Generate the row 13456782 (or similar)
      string s; s.reserve(bells);
      { 
	for (int i=0; i<args.hunt_bells; ++i)
	  s.append(1u, bell(i).to_char() );
      }
      {
	for (int i=args.hunt_bells+1; i<bells; ++i)
	  s.append(1u, bell(i).to_char() );
      }
      s.append(1u, bell(args.hunt_bells).to_char() );

      const row k(s);
      const row rlh( r * k * r.inverse() );

      const row lh( m.lh() );

      // Is lh a power of rlh?
      bool ok(false);
      r = row(); 
      do 
	{
	  r *= rlh;
	  if (r == lh) ok = true;
	} 
      while ( !r.isrounds() );

      if (!ok)
	return;
    }

  if ( args.true_course )
    {
      vector< row > rows( 1, row(bells) );
  
      do for ( method::const_iterator i(m.begin()), e(m.end()); i != e; ++i )
	rows.push_back( rows.back() * *i );
      while ( !rows.back().isrounds() );

      rows.pop_back();

      sort( rows.begin(), rows.end() );
      if ( adjacent_find( rows.begin(), rows.end() ) != rows.end() )
	return;
    }

  else if ( args.true_lead )
    {
      vector< row > rows( 1, row(bells) );
  
      for ( method::const_iterator i(m.begin()), e(m.end()-1); i != e; ++i )
	rows.push_back( rows.back() * *i );

      sort( rows.begin(), rows.end() );
      if ( adjacent_find( rows.begin(), rows.end() ) != rows.end() )
	return;
    }

  if ( args.require_CPS && !is_cps( m ) )
    return;

  if ( args.true_extent && !might_support_extent(m) )
    return;

  if ( !args.require_expr.null() && !args.require_expr.b_evaluate(m) )
    return;

  found_method();
}

inline void searcher::call_recurse( const change &ch )
{
  m.push_back( ch );
  general_recurse();
  m.pop_back();
}

inline int searcher::get_posn()
{
  assert( args.hunt_bells );

  const int depth = m.length();

  int posn = depth >= hl_len ? 2*hl_len - depth - 2 : depth;
  
  if ( posn % div_len == div_len - 1 )
    return posn / div_len * 2 + 1;
  else
    return posn / div_len * 2;
}

inline bool searcher::is_cyclic_hl( const row& hl )
{
  assert( hl[ bells-1 ] == 0 );
  
  for (int i=0; i<bells-2; ++i)
    if ( hl[i-1] % (bells-1) + 1 != hl[i] )
      return false;
  
  return true;
}

inline bool searcher::is_rev_cyclic_hl( const row& hl )
{
  assert( hl[ bells-1 ] == 0 );
  
  for (int i=0; i<bells-2; ++i)
    if ( hl[i+1] % (bells-1) + 1 != hl[i] )
      return false;
  
  return true;
}

bool searcher::is_regular_hl( const row& hl )
{
  static row pblh( row::pblh(bells) );
  row rr( row::reverse_rounds(bells) );
  
  row rhl(rr);
  do {
    if (rhl == hl)
      return true;
    rhl = pblh * rhl;;
  } while (rhl != rr);
  
  {
    char str[] = "U"; str[0] = bell(bells-1).to_char();
    change ch( bells, str );  
    rhl *= ch;  rr *= ch;
  }

  do {
    if (rhl == hl)
      return true;
    rhl = pblh * rhl;;
  } while (rhl != rr);
  
  return false;
}

bool searcher::try_halflead_change( const change &ch )
{
  // Rotationally symmetric single-hunt cyclic methods have cyclic half-leads
  if ( args.skewsym && args.require_cyclic_les && args.hunt_bells == 1 )
    {
      row hl;
      for_each( m.begin(), m.end(), permute(hl) );

      assert( hl[ bells-1 ] == 0 );

      for (int i=1; i<bells-1; ++i)
	if ( hl[i-1] % (bells-1) + 1 != hl[i] )
	  return false;
    }

  if ( args.require_rev_cyclic_hlh || args.require_rev_cyclic_hle ||
       args.require_cyclic_hlh     || args.require_cyclic_hle     ||
       args.require_reg_hls  )
    {
      // Get half lead rows
      row hle; for_each( m.begin(), m.end(), permute(hle) );
      row hlh(hle); hlh *= ch;

      if      ( args.require_rev_cyclic_hlh && is_rev_cyclic_hl(hlh) )
	; // OK

      else if ( args.require_rev_cyclic_hle && is_rev_cyclic_hl(hle) )
	; // OK

      else if ( args.require_cyclic_hlh && is_cyclic_hl(hlh) )
	; // OK

      else if ( args.require_cyclic_hle && is_cyclic_hl(hle) )
	; // OK

      else if ( args.require_reg_hls && 
		is_regular_hl(hlh) && is_regular_hl(hle) )
	; // OK

      else
	return false;
    }

  return true;
}

bool searcher::try_halflead_sym_change( const change &ch )
{
  if ( args.require_single_place_lh_le 
       && ch.count_places() >= 4 )
    return false;

  if ( args.true_trivial 
       && ch.count_places() == bells )
    return false;

  if ( (args.skewsym || args.doubsym) && args.require_limited_le )
    {
      string pn("1");
      pn += bell( args.hunt_bells ).to_char();
      
      change ch2( ch.reverse() );
      if ( ch2 != change( bells, "1" ) && ch2 != change( bells, pn ) )
	return false;
    }

  if ( (args.skewsym || args.doubsym) &&
       args.no_78_pns && !args.require_single_place_lh_le 
       && ch.findplace(1) )
    return false;

  if ( args.sym && args.max_consec_blows
       && is_too_many_places( m, ch, args.max_consec_blows/2+1 ) )
    return false;

  return true;
}

bool searcher::try_offset_start_change( const change &ch) 
{
  const row r( m.lh() * ch );
  
  // Generate the row 13456782 (or similar)
  string s; s.reserve(bells);
  {
    for (int i=0; i<args.hunt_bells; ++i)
      s.append(1u, bell(i).to_char() );
  }
  {
    for (int i=args.hunt_bells+1; i<bells; ++i)
      s.append(1u, bell(i).to_char() );
  }
  s.append(1u, bell(args.hunt_bells).to_char() );

  const row k(s);
  const row lh( r * k * r.inverse() );

  return is_acceptable_leadhead( lh );
}

bool searcher::try_leadend_change( const change &ch )
{
  if ( args.max_consec_blows )
    for ( unsigned int i=0; i<bells; ++i )
      if ( ch.findplace(i) )
	{
	  unsigned int count(2);
	  
	  {
	    for ( int offset = m.length() - 1; 
		  offset >= 0 && count <= args.max_consec_blows; 
		  --offset, ++count )
	      if ( !m[offset].findplace(i) )
		break;
	  }

	  {
	    for ( int offset = 0; 
		  offset < m.length() && count <= args.max_consec_blows; 
		  ++offset, ++count )
	      if ( !m[offset].findplace(i) )
		break;
	  }

	  if ( count > args.max_consec_blows )
	    return false;
	}

  if ( !args.hunt_bells )
    if ( ch == m.front() )
      return false;

  return true;
}

bool searcher::try_leadend_sym_change( const change &ch )
{
  if ( args.require_single_place_lh_le 
       && ch.count_places() >= 4 )
    return false;

  if ( args.true_trivial 
       && ch.count_places() == bells )
    return false;
 
  if ( args.require_limited_le )
    {
      string pn("1");
      pn += bell( args.hunt_bells ).to_char();
      
      if ( ch != change( bells, "1" ) && ch != change( bells, pn ) )
	return false;
    }

  if ( args.no_78_pns && !args.require_single_place_lh_le 
       && ch.findplace(bells-2) )
    return false;

  if ( args.sym && args.max_consec_blows
       && is_too_many_places( m, ch, args.max_consec_blows/2+1 ) )
    return false;

  return true;
}

bool searcher::try_midlead_change( const change &ch )
{
  int depth = m.size();

  if ( args.true_trivial && (args.treble_dodges || !args.hunt_bells) 
       && m.size() && m.back() == ch )
    return false;

  int posn = args.hunt_bells ? get_posn() : 0;

  // The treble is moving between dodging positions
  if ( args.surprise && posn % 2 && depth % hl_len != hl_len-1
       && !ch.internal() )
    return false;

  if ( args.treble_bob && posn % 2 && depth % hl_len != hl_len-1
       && ch.internal() )
    return false;
  
  if ( args.max_places_per_change 
       && ch.count_places() > args.max_places_per_change )
    return false;
  
  if ( args.same_place_parity && args.treble_dodges == 1
       && m.length() % div_len && depth % div_len != div_len - 1 
       && ch.sign() == m.back().sign() )
    return false;
  
  if ( args.max_consec_places && depth % hl_len != hl_len-1
       && has_consec_places( ch, args.max_consec_places ) )
    return false;

  if ( args.max_consec_blows )
    {
      if ( args.sym && args.hunt_bells % 2 == 0 && depth < hl_len 
	   && is_too_many_places( m, ch, args.max_consec_blows/2+1 ) )
	return false;

      else if ( is_too_many_places( m, ch, args.max_consec_blows ) )
	return false;
    }
  
  if ( args.true_half_lead && args.treble_dodges > 1 && posn % 2 == 0
       && is_division_false( m, ch, div_len ) )
    return false;
  
  if ( args.same_place_parity && args.treble_dodges > 1 
       && m.length() % div_len == div_len - 2 
       && division_bad_parity_hack( m, ch, div_len ) )
    return false;

  if ( args.allowed_changes.size() > depth )
    {
      const vector<change>& a = args.allowed_changes[depth];
      if ( a.size() && find( a.begin(), a.end(), ch ) == a.end() )
	return false;
    }

  return true;
}

bool searcher::try_quarterlead_change( const change &ch )
{
  if ( ch != ch.reverse() )
    return false;

  if ( args.max_consec_blows )
    for ( unsigned int i=0; i<bells; ++i )
      if ( ch.findplace(i) )
	{
	  unsigned int count(2);
	  
	  {
	    for ( int offset = m.length()-1;
		  offset >= (m.length()/hl_len)*hl_len 
		    && count <= args.max_consec_blows + 1;
		  --offset, ++count )
	      if ( !m[offset].findplace(i) )
		break;
	  }
	  
	  {
	    for ( int offset = m.length()-1;
		  offset >= (m.length()/hl_len)*hl_len 
		    && count <= args.max_consec_blows + 1;
		  --offset, ++count )
	      if ( !m[offset].reverse().findplace(i) )
		break;
	  }
	  
	  if ( count > args.max_consec_blows )
	    return false;
	}

  return true;
}

inline void searcher::swap_overlap( change &ch, const pair< int, int > &posn )
{
  for ( int k = posn.first == -1 ? 1 : posn.first; 
	k <= posn.second && k < bells-1; k += 2 )
    ch.swappair(k);
}


void searcher::new_midlead_change()
{
  const int depth( m.length() );

  const vector< change >& changes_to_try = args.allowed_changes[depth];
  assert( changes_to_try.size() );

  // Code to start at a particular point       
  change first;
  if ( ! first_meth.empty() ) {
    first = change( first_meth.back() ); 
    first_meth.pop_back();
  }

  for ( vector<change>::const_iterator 
	  i( changes_to_try.begin() ), e( changes_to_try.end() ); 
	i != e; ++i )
    {
      if ( first.bells() == 0 || *i >= first )
	{
	  const change& ch = *i;

	  if ( ! try_midlead_change( ch ) )
	    continue;

	  if ( args.skewsym && hl_len % 2 == 0 
	       && depth % hl_len == hl_len / 2 - args.hunt_bells % 2 &&
	       ! try_quarterlead_change( ch ) )
	    continue;

	  if ( depth == hl_len-1 )
	    if ( ! try_halflead_change( ch ) )
	      continue;

	  if ( args.hunt_bells % 2 == 1 && depth == hl_len-1 ||
	       args.hunt_bells % 2 == 0 && depth == hl_len )
	    if ( ! try_halflead_sym_change( ch ) )
	      continue;
	  
	  if ( depth == args.lead_len-1 )
	    if ( ! try_leadend_change( ch ) )
	      continue;
	  
	  if ( args.hunt_bells % 2 == 1 && depth == 2*hl_len-1 ||
	       args.hunt_bells % 2 == 0 && depth == 0 )
	    if ( ! try_leadend_sym_change( ch ) )
	      continue;
	  
	  if ( args.require_offset_cyclic && div_len > 3
	       && depth == div_len-3 )
	    if ( ! try_offset_start_change( ch ) )
	      continue;

	  call_recurse( ch );
	}
    }
}

void searcher::new_principle_change()
{
  if ( args.right_place && bells % 2 == 0 && m.length() % 2 == 0 )
    {
      change ch( bells, "-" );

      if ( first_meth.empty() )
	{
	  call_recurse( ch );
	}
      // Code to start at a particular point
      else if ( first_meth.back() <= ch )
	{
	  first_meth.pop_back();
	  call_recurse( ch );
	}
    }
  else
    {
      int depth( m.length() );

      vector< change > changes_to_try;
      changes_to_try.reserve( fibonacci( bells ) );

      // Choose a change
      for ( changes_iterator i(bells), e; i != e; ++i )
	{
	  change ch(*i); 

	  if ( args.no_78_pns && ch.findplace(bells-2) )
	    continue;

	  if ( args.true_trivial 
	       && ch.count_places() == bells )
	    continue;

	  if ( (args.skewsym || args.doubsym) && args.no_78_pns 
	       && ch.findplace(1) )
	    continue;

	  if ( ! try_midlead_change( ch ) )
	    continue;

	  if ( depth == args.lead_len-1 )
	    if ( ! try_leadend_change( ch ) )
	      continue;

	  changes_to_try.push_back( ch );
	}

      sort( changes_to_try.begin(), changes_to_try.end() );

      // Code to start at a particular point       
      if ( ! first_meth.empty() )
	{
	  change first( first_meth.back() ); 
	  first_meth.pop_back();

	  for ( vector< change >::const_iterator i( changes_to_try.begin() ),
		  e( changes_to_try.end() ); i != e; ++i )
	    if ( *i >= first )
	      call_recurse( *i );
	}
      else for ( vector< change >::const_iterator i( changes_to_try.begin() ),
		   e( changes_to_try.end() ); i != e; ++i )
	call_recurse( *i );
    }
}

void searcher::double_existing()
{
  if ( args.max_consec_blows )
    for ( unsigned int i=0; i<bells; ++i )
      if ( m.back().findplace(i) )
	{
	  unsigned int count(2);
	  
	  {
	    for ( int offset = m.length()-2; 
		  offset >= 0 && count <= args.max_consec_blows + 1; 
		  --offset, ++count )
	      if ( !m[offset].findplace(i) )
		break;
	  }
	  
	  {
	    for ( int offset = 0; 
		  offset < m.length() && count <= args.max_consec_blows + 1; 
		  ++offset, ++count )
	      if ( !m[offset].reverse().findplace(i) )
		break;
	  }
	  
	  if ( count > args.max_consec_blows )
	    return;
	}
  
  assert( m.length() == hl_len );

  for ( int depth = m.length(); depth < 2*hl_len-1; ++depth )
    m.push_back( m[ depth-hl_len ].reverse() );

  change ch( m[ hl_len-1 ].reverse() );
  
  // Not sure if this is still necessary ...
  if ( args.max_consec_blows )
    for ( unsigned int i=0; i<bells; ++i )
      if ( ch.findplace(i) )
	{
	  unsigned int count(2);
	  
	  {
	    for ( int offset = m.length()-1;
		  offset >= 0;
		  --offset, ++count )
	      if ( !m[offset].findplace(i) )
		break;
	  }

	  {	  
	    for ( int offset = 0;
		  offset < m.length();
		  ++offset, ++count )
	      if ( !m[offset].findplace(i) )
		break;
	  }
	  
	  if ( count > args.max_consec_blows )
	    goto end_of_function;
	}
  
  m.push_back( ch );

  general_recurse();

 end_of_function:
  while ( m.length() > hl_len )
    m.pop_back();
}

// -----------------------------------------


bool searcher::is_acceptable_leadhead( const row &lh )
{

  if ( ( args.show_all_meths 
	 || lh.cycles().substr( args.hunt_bells * 2 ).find(',')
	 == string::npos ) )
    {
      if ( args.require_pbles ) 
	{
	  if ( is_pble(lh, args.hunt_bells) )
	    return true;
	}
      else if ( args.require_cyclic_les )
	{
	  if ( is_cyclic_le(lh, args.hunt_bells) )
	    return true;
	}
      else
	{
	  return true;
	} 
    }

  return false;
}

void searcher::general_recurse()
{
  const int depth = m.length();

  if ( args.search_limit && search_count == args.search_limit )
    return;

  // Status message
  if ( args.status )
    {
      static int count = 0;
      if ( count % 10000 == 0 )
	output_status( m );
      ++count;
    }


  // Found something
  if ( depth == args.lead_len )
    {
      if ( is_acceptable_leadhead( m.lh() ) )
	maybe_found_method();
    }


  // Symmetry in principles is not handled until later, because we cannot
  // be sure where the symmetry points will be.
  else if ( ! args.hunt_bells )
    {
      new_principle_change();
    }


  // The quarter-lead change in skew-symmetric methods is special.
  // (e.g. it is self-reverse).
  else if ( args.skewsym && hl_len % 2 == 0 
	    && depth % hl_len == hl_len / 2 - args.hunt_bells % 2 )
    {
      new_midlead_change();
    }


  // Maximum symmetry
  else if ( args.skewsym && args.doubsym && args.sym 
	    && depth == hl_len/2 + 1 - args.hunt_bells % 2 )
    {
      while ( m.length() <= hl_len - 1 - args.hunt_bells % 2 )
	{
	  m.push_back( m[ hl_len - args.hunt_bells % 2 * 2 
			  - m.length() ].reverse() );
	}

      general_recurse();

      while ( m.length() > depth )
	m.pop_back();
    }


  // Only rotational symmetry
  else if ( args.skewsym && depth == hl_len/2 + 1 - args.hunt_bells % 2 )
    {
      assert( !args.doubsym && !args.sym );

      while ( m.length() <= hl_len - args.hunt_bells % 2 * 2 )
	{
	  m.push_back( m[ hl_len - args.hunt_bells % 2 * 2 
			  - m.length() ].reverse() );
	}

      general_recurse();

      while ( m.length() > depth )
	m.pop_back();
    }


  // Double symmetry (with or without others)
  else if ( args.doubsym && depth == hl_len )
    {
      double_existing();
    }


  // Only conventional symmetry
  else if ( args.sym && depth == hl_len + 1 - args.hunt_bells % 2 )
    {
      assert( !args.skewsym && !args.doubsym );

      copy( m.rbegin() + 1, m.rend() - (1 - args.hunt_bells % 2), 
	    back_inserter(m) );

      general_recurse();

      while ( m.length() > depth )
	m.pop_back();
    }


  // Only rotational symmetry
  else if ( args.skewsym && depth == 3*hl_len / 2 + 1 - args.hunt_bells % 2 )
    {
      assert( !args.doubsym && !args.sym );

      while ( m.length() < 2*hl_len )
	{
	  m.push_back( m[ 3*hl_len - args.hunt_bells % 2 * 2
			  - m.length() ].reverse() );
	}

      general_recurse();

      while ( m.length() > depth )
	m.pop_back();
    }

  // Need a new change
  else
    {
      new_midlead_change();
    }
}
