// -*- C++ -*- search.cpp - the actual search algorithm
// Copyright (C) 2002, 2003, 2004, 2005 Richard Smith <richard@ex-parrot.com>

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

#include "methodutils.h"
#include "search.h"
#include "falseness.h"
#include "expression.h"
#include "prog_args.h"
#include "format.h" // for clear_status
#include "output.h"
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <algo.h>
#else
#include <vector>
#include <algorithm>
#endif
#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#include <math.h>
#else
#include <cassert>
#include <cmath>
#endif
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/extent.h>
#include <ringing/proof.h>


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
  bool is_limited_le( const change& ch );

  void new_midlead_change();
  void double_existing();

  bool try_halflead_change( const change &ch );
  bool try_leadend_change( const change &ch );
  bool try_leadend_sym_change( const change &ch );
  bool try_halflead_sym_change( const change &ch );
  bool try_midlead_change( const change &ch );
  bool try_quarterlead_change( const change &ch );
  bool try_offset_start_change( const change &ch);
  bool try_with_limited_le( const change& ch );
  bool try_principle_symmetry();

  bool is_acceptable_method();
  void found_method();

  bool is_acceptable_leadhead( const row &lh );

private:
  const arguments &args; 
  const int bells;
  const size_t div_len;
  const size_t hl_len;

  unsigned long search_count;

  vector< change > first_meth;  // To allow searches to be resumed
  method m;
};

void run_search( const arguments &args, const method &initm )
{
  searcher s( args, initm );
  s.general_recurse();
  assert( s.m.length() == 0 );
  if ( args.count || args.raw_count )
    {
      if ( args.status && args.outfile.empty() ) clear_status();
      if ( !args.quiet ) cout << "\n";
      if ( args.raw_count ) output_raw_count( cout, s.search_count );
      else if ( args.count ) output_count( cout, s.search_count );
    }
}

void searcher::found_method()
{
  if ( !args.outputs.empty() ) {
    method_properties props(m);

    if ( !args.quiet && args.status && args.outfile.empty() )
      clear_status();

    args.outputs.append( props );
  }

  ++search_count;
}

bool searcher::try_principle_symmetry()
{
  if ( args.skewsym || args.doubsym || args.sym )
    {
      string const sym( method_symmetry_string(m) );

      if ( args.skewsym && sym.find('R') == string::npos )
	return false;

      if ( args.sym && sym.find('P') == string::npos )
	return false;

      if ( args.doubsym && sym.find('G') == string::npos )
	return false;
    }

  return true;
}

bool searcher::try_with_limited_le( const change& ch )
{
  change orig( m.back() );
  m.pop_back();

  size_t depth = m.length();
  assert( depth == size_t(args.lead_len-1) );

  if ( !try_midlead_change(ch) )
    {
      m.push_back(orig);
      return false;
    }
  
  if ( ! try_leadend_change( ch ) )
    {
      m.push_back(orig);
      return false;
    }

  if ( args.hunt_bells % 2 == 1 && depth == 2*hl_len-1 ||
       args.hunt_bells % 2 == 0 && depth == 0 )
    if ( ! try_leadend_sym_change( ch ) )
      {
	m.push_back(orig);
	return false;
      }

  m.push_back(ch);
  bool ok = is_acceptable_method();
  m.back() = orig;
  return ok;
}

bool searcher::is_acceptable_method()
{
  if ( ! is_acceptable_leadhead( m.lh() ) )
    return false;

  if ( !args.hunt_bells )
    if ( ! try_principle_symmetry() )
      return false;

  if ( args.hunt_bells && args.require_offset_cyclic )
    {
      // Offset cyclic methods are started from the last backstroke
      // snap and have cyclic rows at these points.

      // r is the row at the point of cylicity.
      row r(bells);
      for (size_t i=0; i< div_len-2; ++i) r *= m[i];

      // This is the lead head predicted by assuming cyclicity
      const row rlh( r * row::cyclic( bells, args.hunt_bells ) * r.inverse() );

      // And this is the actual lead head
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
	return false;
    }

  if ( !args.hunt_bells && args.require_offset_cyclic )
    {
      bool ok(false);
      const int n( m.size() );

      // Offset cyclic principles can be started anywhere.
      // Try all possibilities until we find one we like.
      for ( int o=0; o<n; ++o )
	{
	  row r(bells);
	  for ( int i=0; i<n; ++i )
	    r *= m[ (i+o)%n ];

	  ok = is_cyclic_le(r, args.hunt_bells);

	  if (ok) break;
	}

      if (!ok) return false;
    }

  if ( args.true_course )
    {
      prover p( (int) ceil( (double) (bells - args.hunt_bells) * args.lead_len
			    / (double) factorial(bells) ) );

      row r(bells);  
      do for ( method::const_iterator i(m.begin()), e(m.end()); 
	       p.truth() && i != e; ++i )
	p.add_row( r *= *i );
      while ( !r.isrounds() && p.truth() );

      if ( !p.truth() ) 
	return false;
    }

  else if ( args.true_lead )
    {
      prover p( (int) ceil( (double) (bells - args.hunt_bells) * args.lead_len
			    / (double) factorial(bells) ) );
      
      row r(bells);  
      for ( method::const_iterator i(m.begin()), e(m.end()); 
	    p.truth() && i != e; ++i )
	p.add_row( r *= *i );

      if ( !p.truth() ) 
	return false;
    }

  if ( args.require_CPS && !is_cps( m ) )
    return false;

  if ( args.true_extent && !might_support_extent(m) )
    return false;

  if ( args.true_positive_extent && !might_support_positive_extent(m) )
    return false;

  if ( args.require_expr_idx != static_cast<size_t>(-1) ) {
    method_properties props(m);
    if ( !expression_cache::b_evaluate( args.require_expr_idx, props ) )
      return false;
  }

  if ( args.prefer_limited_le && !is_limited_le( m.back() ) &&
       ( try_with_limited_le( change( bells, "1"  ) ) ||
	 try_with_limited_le( change( bells, "12" ) ) ) )
    return false;

  return true;
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

  const size_t depth = m.length();

  size_t posn = depth >= hl_len ? 2*hl_len - depth - 2 : depth;
  
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

bool searcher::is_limited_le( const change& ch )
{
  string pn("1");
  pn += bell( args.hunt_bells ).to_char();
  
  if ( ch == change( bells, "1" ) || ch == change( bells, pn ) )
    return true;
  else
    return false;
}

bool searcher::try_halflead_sym_change( const change &ch )
{
  if ( args.true_trivial 
       && ch.count_places() == bells )
    return false;

  if ( (args.skewsym || args.doubsym) && args.require_limited_le 
       && !is_limited_le( ch.reverse() ) )
    return false;

  if ( (args.skewsym || args.doubsym) &&
       args.no_78_pns && ch.findplace(1) )
    return false;

  if ( args.sym && args.max_consec_blows
       && is_too_many_places( m, ch, args.max_consec_blows/2+1 ) )
    return false;

  return true;
}

bool searcher::try_offset_start_change( const change &ch) 
{
  // Offset cyclic methods are started from the last backstroke snap of 
  // the treble's 1-2 up dodge.   This function is called to test that
  // row.

  // Because we are requiring that cyclicity occurs at this point, we 
  // can use this to determine the lead heads, and thus check whether
  // a valid lead head is possible.   This offers a big saving if we
  // want to restrict the lead head.

  const row r( m.lh() * ch ); // the offset change
  
  const row lh( r * row::cyclic( bells, args.hunt_bells ) * r.inverse() );

  return is_acceptable_leadhead( lh );
}

bool searcher::try_leadend_change( const change &ch )
{
  if ( args.max_consec_blows )
    for ( int i=0; i<bells; ++i )
      if ( ch.findplace(i) )
	{
	  int count(2);
	  
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
    // Need m.size() to handle the (admitedly rather silly) -n1 option
    if ( args.true_trivial && m.size() && ch == m.front() )
      return false;

  return true;
}

bool searcher::try_leadend_sym_change( const change &ch )
{
  if ( args.true_trivial 
       && ch.count_places() == bells )
    return false;
 
  if ( args.require_limited_le && !is_limited_le(ch) ) 
    return false;

  if ( args.no_78_pns && ch.findplace(bells-2) )
    return false;

  if ( args.sym && args.max_consec_blows
       && is_too_many_places( m, ch, args.max_consec_blows/2+1 ) )
    return false;

  return true;
}

bool searcher::try_midlead_change( const change &ch )
{
  size_t depth = m.size();

  if ( args.true_trivial && (args.treble_dodges || !args.hunt_bells) 
       && m.size() && m.back() == ch )
    return false;

  size_t posn = args.hunt_bells ? get_posn() : 0;

  // The treble is moving between dodging positions
  if ( args.surprise && posn % 2 && depth % hl_len != hl_len-1
       && !ch.internal() )
    return false;

  if ( args.treble_bob && posn % 2 && depth % hl_len != hl_len-1
       && ch.internal() )
    return false;
  
  if ( args.same_place_parity && args.treble_dodges == 1
       && m.length() % div_len && depth % div_len != div_len - 1 
       && ch.sign() == m.back().sign() )
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

  return true;
}

bool searcher::try_quarterlead_change( const change &ch )
{
  if ( ch != ch.reverse() )
    return false;

  if ( args.max_consec_blows )
    for ( int i=0; i<bells; ++i )
      if ( ch.findplace(i) )
	{
	  int count(2);
	  
	  {
	    // NOTE: Must be signed to allow decrementing below zero
	    for ( signed offset = m.length()-1;
		  offset >= signed( (m.length()/hl_len)*hl_len ) 
		    && count <= args.max_consec_blows + 1;
		  --offset, ++count )
	      if ( !m[offset].findplace(i) )
		break;
	  }
	  
	  {
	    // NOTE: Must be signed to allow decrementing below zero
	    for ( signed offset = m.length()-1;
		  offset >= signed( (m.length()/hl_len)*hl_len ) 
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
  const size_t depth( m.length() );

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
      if ( first.bells() == 0 || !compare_changes(*i, first) )
	{
	  const change& ch = *i;

	  if ( ! try_midlead_change( ch ) )
	    continue;

	  if ( args.hunt_bells && args.skewsym && hl_len % 2 == 0 
	       && depth % hl_len == hl_len / 2 - args.hunt_bells % 2 &&
	       ! try_quarterlead_change( ch ) )
	    continue;

	  if ( depth == hl_len-1 )
	    if ( ! try_halflead_change( ch ) )
	      continue;

	  if ( args.hunt_bells % 2 == 1 && depth == hl_len-1 ||
	       args.hunt_bells && 
	       args.hunt_bells % 2 == 0 && depth == hl_len+args.treble_dodges )
	    if ( ! try_halflead_sym_change( ch ) )
	      continue;
	  
	  if ( depth == size_t(args.lead_len-1) )
	    if ( ! try_leadend_change( ch ) )
	      continue;
	  
	  if ( args.hunt_bells % 2 == 1 && depth == 2*hl_len-1 ||
	       args.hunt_bells && 
	       args.hunt_bells % 2 == 0 && depth == size_t(args.treble_dodges) )
	    if ( ! try_leadend_sym_change( ch ) )
	      continue;
	  
	  if ( args.hunt_bells && args.require_offset_cyclic && div_len > 3
	       && depth == div_len-3 )
	    if ( ! try_offset_start_change( ch ) )
	      continue;

	  call_recurse( ch );
	}
    }
}


void searcher::double_existing()
{
  if ( args.max_consec_blows )
    for ( int i=0; i<bells; ++i )
      if ( m.back().findplace(i) )
	{
	  int count(2);
	  
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
  
  assert( size_t(m.length()) == hl_len );

  for ( size_t depth = m.length(); depth < 2*hl_len-1; ++depth )
    m.push_back( m[ depth-hl_len ].reverse() );

  change ch( m[ hl_len-1 ].reverse() );
  
  // Not sure if this is still necessary ...
  if ( args.max_consec_blows )
    for ( int i=0; i<bells; ++i )
      if ( ch.findplace(i) )
	{
	  int count(2);
	  
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
  while ( size_t(m.length()) > hl_len )
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
	  if ( lh.ispblh(args.hunt_bells) )
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
  const size_t depth = m.length();

  if ( args.search_limit && search_count == (unsigned long)args.search_limit )
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
  if ( depth == size_t(args.lead_len) )
    {
      if ( is_acceptable_method() )
	found_method();
    }


  // Symmetry in principles is not handled until later, because we cannot
  // be sure where the symmetry points will be.
  else if ( ! args.hunt_bells )
    {
      new_midlead_change();
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
      while ( size_t(m.length()) <= hl_len - 1 - args.hunt_bells % 2 )
	{
	  m.push_back( m[ hl_len - args.hunt_bells % 2 * 2 
			  - m.length() ].reverse() );
	}

      general_recurse();

      while ( size_t(m.length()) > depth )
	m.pop_back();
    }


  // Only rotational symmetry
  else if ( args.skewsym && depth == hl_len/2 + 1 - args.hunt_bells % 2 )
    {
      assert( !args.doubsym && !args.sym );

      while ( size_t(m.length()) <= hl_len - args.hunt_bells % 2 * 2 )
	{
	  m.push_back( m[ hl_len - args.hunt_bells % 2 * 2 
			  - m.length() ].reverse() );
	}

      general_recurse();

      while ( size_t(m.length()) > depth )
	m.pop_back();
    }


  // Double symmetry (with or without others)
  else if ( args.doubsym && depth == hl_len )
    {
      double_existing();
    }


  // Only conventional symmetry
  else if ( args.sym && depth == hl_len + 
	    ( args.hunt_bells % 2 ? 0 : args.treble_dodges + 1 ) )
    {
      assert( !args.skewsym && !args.doubsym );

      copy( m.rbegin() + 1, 
	    m.rend() - ( args.hunt_bells % 2 ? 0 : 2*args.treble_dodges + 1 ),
	    back_inserter(m) );

      general_recurse();

      while ( size_t(m.length()) > depth )
	m.pop_back();
    }


  // Conventional symmetry when we have an even number of hunt bells 
  // treble dodging -- first division
  else if ( args.sym && args.hunt_bells % 2 == 0 && args.treble_dodges &&
	    depth == size_t(args.treble_dodges + 1) )
    {
      copy( m.rbegin() + 1, m.rend(), back_inserter(m) );

      general_recurse();

      while ( size_t(m.length()) > depth )
	m.pop_back();
    }

  // Only rotational symmetry
  else if ( args.skewsym && depth == 3*hl_len / 2 + 1 - args.hunt_bells % 2 )
    {
      assert( !args.doubsym && !args.sym );

      while ( size_t(m.length()) < 2*hl_len )
	{
	  m.push_back( m[ 3*hl_len - args.hunt_bells % 2 * 2
			  - m.length() ].reverse() );
	}

      general_recurse();

      while ( size_t(m.length()) > depth )
	m.pop_back();
    }

  // Need a new change
  else
    {
      new_midlead_change();
    }
}
