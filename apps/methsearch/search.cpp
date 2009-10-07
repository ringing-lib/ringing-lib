// -*- C++ -*- search.cpp - the actual search algorithm
// Copyright (C) 2002, 2003, 2004, 2005, 2007, 2008, 2009
// Richard Smith <richard@ex-parrot.com>

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
#include "libraries.h" // for filter_lib code
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
#include <ringing/mathutils.h>
#include <ringing/litelib.h>
#include <ringing/falseness.h>


RINGING_USING_NAMESPACE
RINGING_USING_STD

class searcher
{
private:
  friend void run_search( const arguments &args );

  searcher( const arguments &args );

  void filter( library const& );
  void general_recurse();

  inline bool push_change( const change& ch);
  inline void call_recurse( const change &ch );
  inline int get_posn();

  inline bool is_cyclic_hl( const row& hl );
  inline bool is_rev_cyclic_hl( const row& hl );
  inline bool is_regular_hl( const row& hl );
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

  row canonical_coset_member( row const& r );

  bool is_acceptable_method();
  void found_method();

  bool is_acceptable_leadhead( const row &lh );
  bool is_falseness_acceptable( const change& ch );

private:
  const arguments &args; 
  const int bells;

  const size_t div_len;  // How long the treble stays in a dodging position
                         // 2 for plain methods;  4 for normal TD methods

  const size_t hl_len;   // Length of half a lead

  RINGING_ULLONG search_count;
  RINGING_ULLONG node_count;

  vector<change> startmeth;
  method filter_method;
  method m;
  row r;
  bool maintain_r;   // Whether r is valid
};

searcher::searcher( const arguments &args )
  : args(args),
    bells( args.bells ),
    div_len( (1 + args.treble_dodges) * 2 ),
    hl_len( args.lead_len / 2 ),
    search_count( 0ul ), node_count( 0ul ),
    r( bells ), maintain_r( args.avoid_rows.size() )
{
  m.reserve( 2 * hl_len );

  copy( args.startmeth.rbegin(), args.startmeth.rend(), 
        back_inserter(startmeth) );
}

void searcher::filter( library const& in )
{
  for ( library::const_iterator i=in.begin(), e=in.end(); i!=e; ++i ) 
    {
      try {
        filter_method = i->meth();
      } 
      catch ( std::exception const& ex ) {
        std::cerr << "Error reading method from input stream: " 
                  << ex.what() << "\n";
        string pn;  try { pn = i->pn(); } catch (...) {}
        if ( pn.size() ) std::cerr << "Place notation: '" << pn << "'\n";
        std::cerr << std::flush;

        continue;
      }

      // Status message (when in filter mode)
      if ( args.status && node_count % 10000 == 0 )
        output_status( filter_method );
      ++node_count;

      general_recurse();
      assert( m.length() == 0 );
    } 
}

void run_search( const arguments &args )
{
  searcher s( args );

  try 
  {
    if ( args.filter_mode ) {
      litelib in( args.bells, std::cin );
      s.filter(in);
    } else if ( args.filter_lib_mode ) { 
      s.filter( method_libraries::instance() );
    } else {
      s.general_recurse();
      assert( s.m.length() == 0 );
    }
  } 
  catch ( const exit_exception& ) {}

  if ( args.status && args.outfile.empty() ) clear_status();

  // Causes the stats to be emittted
  if ( args.H_fmt_str.size() ) {
    if ( !args.quiet && s.search_count ) cout << "\n";
    args.outputs.flush();
  }

  if ( args.count || args.raw_count || args.node_count )
    {
      if ( s.search_count && ( !args.quiet || args.H_fmt_str.size() ) ) 
        cout << "\n";

      if ( args.raw_count ) output_raw_count( cout, s.search_count );
      else if ( args.count ) output_count( cout, s.search_count );
      if ( args.node_count ) output_node_count( cout, s.node_count );
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
  // Don't bother maintaining r in this function.  
  // We don't want want the -F options to modify the behaviour of -E, 
  // so nothing here should test falseness, and so nothing here will test r.

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

  // This used to call is_acceptable_method.  This was bad because
  // we don't want things like --start-at, --require, and the falseness
  // options to effect -E.  At least, I don't think we do.
  bool ok = is_acceptable_leadhead( m.lh() );
  m.back() = orig;
  return ok;
}

row searcher::canonical_coset_member( row const& r )
{
  // Streamline the normal case
  if ( args.pends.size() == 1 ) return r;

  row best;
  for ( group::const_iterator i=args.pends.begin(), e=args.pends.end(); 
        i != e; ++i ) 
  {
    row const ir = *i * r;
    if (best.bells() == 0 || ir < best) 
      best = ir;
  }

  return best;
}

bool searcher::is_acceptable_method()
{
  if ( lexicographical_compare( m.begin(), m.end(), 
           args.startmeth.begin(), args.startmeth.end(),
           compare_changes ) )
    return false;

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

  // --- Falseness requirements ---
  if ( args.true_course || args.true_lead )
    {
      // This is a bit of a hack to allow -Fl to be used with TD Minimus
      int const n_extents = 1; 
//        = (int) ceil( (double) (bells - args.hunt_bells) * args.lead_len
//                    / (double) factorial(bells) );

      prover p( n_extents );
      row r(bells);  

      do for ( method::const_iterator i(m.begin()), e(m.end()); 
               p.truth() && i != e; ++i ) {
        p.add_row( canonical_coset_member( r ) );
        r *= *i;
      }
      while ( args.true_course && !r.isrounds() && p.truth() );

      if ( !p.truth() ) 
        return false;
    }
  // TODO: Merge with above code
  if ( args.pends.size() > 1 && args.true_half_lead )
    {
      prover p( 1 );
      row r(bells);

      for ( method::const_iterator i(m.begin()), e(m.begin()+m.size()/2);
               p.truth() && i != e; ++i ) {
        p.add_row( canonical_coset_member( r ) );
        r *= *i;
      }

      if ( !p.truth() )
        return false;
    }

  if ( args.require_CPS && !is_cps( m ) )
    return false;

  if ( args.true_extent && !might_support_extent(m) )
    return false;

  if ( args.true_positive_extent && !might_support_positive_extent(m) )
    return false;

  // --- Other expensive requirements ---

  // Calls back to this function, so pretty expensive
  if ( args.prefer_limited_le && !is_limited_le( m.back() ) &&
       ( try_with_limited_le( change( bells, "1"  ) ) ||
         try_with_limited_le( change( bells, "12" ) ) ) )
    return false;

  // Leave this one last as --requires does a fork and so is very expensive
  for ( vector<size_t>::const_iterator 
          i = args.require_expr_idxs.begin(), e = args.require_expr_idxs.end(); 
        i != e; ++i ) {
    method_properties props(m);
    if ( !expression_cache::b_evaluate( *i, props ) )
      return false;
  }

  return true;
}

inline bool searcher::push_change( const change& ch )
{
  m.push_back( ch );
  if ( maintain_r ) {
    r *= ch;
    // We don't care about the lead head row.
    if ( m.size() != 2 * hl_len && 
         args.avoid_rows.find(r) != args.avoid_rows.end() )
      return false;
  }
  return true;
}

inline void searcher::call_recurse( const change &ch )
{
  // Store old value of r -- this is considerably cheaper than calling
  // m.lh() and still a little better than restoring with r *= ch.
  row old;
  if ( maintain_r ) old = r;

  if ( push_change( ch ) )
    general_recurse();

  m.pop_back();
  if ( maintain_r ) r = old;
}

// 0 = treble in 1-2
// 1 = treble in 2-3
// 2 = treble in 3-4, ...
inline int searcher::get_posn()
{
  assert( args.hunt_bells );

  const size_t depth = m.length();

  // Reflect about the half-lead:
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
  // Don't test falseness (except -Fn) here because it is called with from
  // try_with_limited_le which handles -E.

  if ( args.max_consec_blows )
    for ( int i=0; i<bells; ++i )
      if ( ch.findplace(i) )
        {
          int count(2);
          
          for ( int offset = m.length() - 1; 
                offset >= 0 && count <= args.max_consec_blows; 
                --offset, ++count )
            if ( !m[offset].findplace(i) )
              break;

          for ( int offset = 0; 
                offset < m.length() && count <= args.max_consec_blows; 
                ++offset, ++count )
            if ( !m[offset].findplace(i) )
              break;

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
  // Don't test falseness (except -Fn) here because it is called with from
  // try_with_limited_le which handles -E.

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
  // Don't test falseness (except -Fn) here because it is called with from
  // try_with_limited_le which handles -E.

  size_t depth = m.size();

  if ( args.true_trivial && (args.treble_dodges || !args.hunt_bells) 
       && m.size() && m.back() == ch )
    return false;

  size_t posn = args.hunt_bells ? get_posn() : 0;

  // Is the treble moving between dodging positions (i.e. is posn odd)?
  if (posn % 2 && depth % hl_len != hl_len-1) 
    {
      if ( args.surprise && !ch.internal() )
        return false;
  
      if ( args.treble_bob && ch.internal() )
        return false;
    }
 
  if ( args.sym_sects && posn % 2 == 0 && depth % div_len >= div_len/2 )
    {
      int i = depth/div_len*div_len + div_len - 2 - (depth % div_len);
      assert( i < (int)m.size() && i >= 0 );
      if ( ch != m[i] )
        return false;
    }

 
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
 
  // We've just completed a section.  If there is more than one dodge
  // per position in the treble's path, then there is the possibility of 
  // an individual section being internally false.  (With a single 
  // dodge, this can only happen with a repeated change which is eliminated
  // by the default -Fn.)
  // This test doesn't effect the -E handling noted above as if the base
  // method passes this, so will the variant with a 12 or 1N lh.
  if ( args.true_half_lead && args.treble_dodges > 1 && posn % 2 == 0
       && is_division_false( m, ch, div_len ) )
    return false;
  
  if ( args.same_place_parity && args.treble_dodges > 1 
       && m.length() % div_len == div_len - 2 
       && division_bad_parity_hack( m, ch, div_len ) )
    return false;

  if ( args.allowed_falseness.size() && depth % 4 >= 1 && depth % 4 != 3
       && ! is_falseness_acceptable( ch ) )
    return false;

  return true;
}

bool searcher::is_falseness_acceptable( const change& ch )
{
  row r( args.bells ); for_each( m.begin(), m.end() - 1, permute(r) );
  row c; c *= m.back(); c *= ch;

  // 1 contains 1.r and 1.r.c
  // x contains 1.r.c and 1.r.c.c
  // so x.r = 1.r.c  =>  x = r.c.r^{-1}

  row x = r * c * r.inverse();

  assert( x[0] == 0 );
  string sym( false_courses::lookup_symbol(x) );
  assert( args.bells != 8 || sym.size() );
  if ( sym == "A" || sym.empty() ) return true;
  if ( args.allowed_falseness.find(sym) != string::npos ) return true;
  return false;
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
          
          // NOTE: Must be signed to allow decrementing below zero
          for ( signed offset = m.length()-1;
                offset >= signed( (m.length()/hl_len)*hl_len ) 
                  && count <= args.max_consec_blows + 1;
                --offset, ++count )
            if ( !m[offset].findplace(i) )
              break;
        
          // NOTE: Must be signed to allow decrementing below zero
          for ( signed offset = m.length()-1;
                offset >= signed( (m.length()/hl_len)*hl_len ) 
                  && count <= args.max_consec_blows + 1;
                --offset, ++count )
            if ( !m[offset].reverse().findplace(i) )
              break;
          
          if ( count > args.max_consec_blows )
            return false;
        }

  return true;
}

void searcher::new_midlead_change()
{
  const size_t depth( m.length() );

  const vector< change >& changes_to_try = args.allowed_changes[depth];
  assert( changes_to_try.size() );

  // If we're starting at a particular point (with --start), find out
  // what that change is.
  change first;
  if ( startmeth.size() ) {
    first = startmeth.back();
    startmeth.pop_back();
  }

  for ( vector<change>::const_iterator 
          i( changes_to_try.begin() ), e( changes_to_try.end() ); 
        i != e; ++i )
    {
      const change& ch = *i;

      // Ignore posibilities that are earlier than --start
      if ( first.bells() != 0 && compare_changes(*i, first) )
        continue;

      // If we're parsing a prefix, require the change to be that one
      // TODO: --prefix should be folded into -m.
      if ( args.prefix.size() > m.size() && ch != args.prefix[m.size()] )
        continue;

      // Likewise if filtering, require it to match the current filter method
      if ( filter_method.size() > m.size() && ch != filter_method[m.size()] )
        continue;

      // Generic tests that apply anywhere:
      if ( ! try_midlead_change( ch ) )
        continue;

      // Additional requirements for the rotational symmetry point:
      if ( args.hunt_bells && args.skewsym && hl_len % 2 == 0 
           && depth % hl_len == hl_len / 2 - args.hunt_bells % 2 &&
           ! try_quarterlead_change( ch ) )
        continue;

      // Additional requirements for the half-lead:
      if ( depth == hl_len-1 )
        if ( ! try_halflead_change( ch ) )
          continue;

      // Additional requirements for the palindromic symmetry point of the 
      // treble's path near the middle of the lead.  For single hunt methods,  
      // this is the half-lead; for twin-hunt methods, it is shifted.
      if ( args.hunt_bells % 2 == 1 && depth == hl_len-1 ||
           args.hunt_bells && 
           args.hunt_bells % 2 == 0 && depth == hl_len+args.treble_dodges )
        if ( ! try_halflead_sym_change( ch ) )
          continue;
     
      // Additional requirements for the lead-end: 
      if ( depth == size_t(args.lead_len-1) )
        if ( ! try_leadend_change( ch ) )
          continue;
      
      // Additional requirements for the palindromic symmetry point of the
      // treble's path near the lead end.  For single hunt methods, this is
      // the lead-end; for twin-hun methods, it is shifted to the start of
      // the lead (e.g. in Grandsire, it is the 3 at the start of the lead).
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


void searcher::double_existing()
{
  if ( args.max_consec_blows )
    for ( int i=0; i<bells; ++i )
      if ( m.back().findplace(i) )
        {
          int count(2);
          
          for ( int offset = m.length()-2; 
                offset >= 0 && count <= args.max_consec_blows + 1; 
                --offset, ++count )
            if ( !m[offset].findplace(i) )
              break;
        
          for ( int offset = 0; 
                offset < m.length() && count <= args.max_consec_blows + 1; 
                ++offset, ++count )
            if ( !m[offset].reverse().findplace(i) )
              break;
        
          if ( count > args.max_consec_blows )
            return;
        }
  
  assert( size_t(m.length()) == hl_len );

  // Note: this loop does not add the lead-end change
  for ( size_t depth = m.length(); depth < 2*hl_len-1; ++depth )
    push_change( m[ depth-hl_len ].reverse() ); // XXX: Update r

  change ch( m[ hl_len-1 ].reverse() );
  
  // Not sure if this is still necessary ...
  if ( args.max_consec_blows )
    for ( int i=0; i<bells; ++i )
      if ( ch.findplace(i) )
        {
          int count(2);
          
          for ( int offset = m.length()-1; offset >= 0; --offset, ++count )
            if ( !m[offset].findplace(i) )
              break;

          for ( int offset = 0; offset < m.length(); ++offset, ++count )
            if ( !m[offset].findplace(i) )
              break;
          
          if ( count > args.max_consec_blows )
            goto end_of_function;
        }
 
  // This is the lead-head change 
  if ( push_change( ch ) )
    general_recurse();

 end_of_function:
  while ( size_t(m.length()) > hl_len )
    m.pop_back();
  if (maintain_r)
    r = m.lh();
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

  // Status message (when in search mode)
  if ( !args.filter_mode ) {
    if ( args.status && node_count % 10000 == 0 )
      output_status( m );
    ++node_count;
  }

  // Found something
  if ( depth == size_t(args.lead_len) )
    {
      if ( filter_method.size() && filter_method != m )
        ;
      else if ( is_acceptable_method() )
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
      // This loop will add up to the half-lead
      while ( size_t(m.length()) <= hl_len - 1 - args.hunt_bells % 2 )
        push_change( m[ hl_len - args.hunt_bells % 2 * 2
                          - m.length() ].reverse() ); // XXX: Update r

      general_recurse();

      while ( size_t(m.length()) > depth )
        m.pop_back();
      if (maintain_r)
        r = m.lh();
    }


  // Only rotational symmetry
  else if ( args.skewsym && depth == hl_len/2 + 1 - args.hunt_bells % 2 )
    {
      assert( !args.doubsym && !args.sym );

      // This loop will add up to the half-lead
      while ( size_t(m.length()) <= hl_len - args.hunt_bells % 2 * 2 )
        push_change( m[ hl_len - args.hunt_bells % 2 * 2
                        - m.length() ].reverse() ); // XXX: Update r

      general_recurse();

      while ( size_t(m.length()) > depth )
        m.pop_back();
      if (maintain_r)
        r = m.lh();
    }


  // Double (glide) symmetry (with or without others)
  else if ( args.doubsym && depth == hl_len )
    {
      double_existing();
    }


  // Only conventional (palindromic) symmetry
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
      if (maintain_r)
        r = m.lh();
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
      if (maintain_r)
        r = m.lh();
    }

  // Only rotational symmetry
  else if ( args.skewsym && depth == 3*hl_len / 2 + 1 - args.hunt_bells % 2 )
    {
      assert( !args.doubsym && !args.sym );

      // This loop *will* add the lead end change.
      while ( size_t(m.length()) < 2*hl_len )
        push_change( m[ 3*hl_len - args.hunt_bells % 2 * 2
                          - m.length() ].reverse() ); // XXX: Update r

      general_recurse();

      while ( size_t(m.length()) > depth )
        m.pop_back();
      if (maintain_r) 
        r = m.lh();
    }

  // Need a new change
  else
    {
      new_midlead_change();
    }
}
