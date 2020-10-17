// -*- C++ -*- search.cpp - the actual search algorithm
// Copyright (C) 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2013,
// 2020 Richard Smith <richard@ex-parrot.com>

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
#include <string.h>
#include <time.h>
#else
#include <cassert>
#include <cmath>
#include <cstring>
#include <ctime>
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
  void reset();

  inline void do_status( method const& m );
  void filter( library const& );
  void general_recurse();

  inline bool push_change( const change& ch);
  inline void pop_change( row const* r_old = NULL );
  inline void call_recurse( const change &ch );
  inline int get_posn() const;
  inline size_t calc_cur_div_len() const;

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

  bool is_acceptable_method();
  void output_method( method const& meth );

  bool is_acceptable_leadhead( const row &lh );
  bool is_falseness_acceptable( const change& ch );

private:
  const arguments &args; 
  const int bells;

  size_t lead_len;

  const size_t div_len;  // How long the treble stays in a dodging position
                         // 2 for plain methods;  4 for normal TD methods

  size_t sym_offset;     // Position of the symmetry point in trebles' paths

  RINGING_ULLONG search_limit;
  RINGING_ULLONG search_count;
  RINGING_ULLONG node_count;

  vector<change> startmeth;
  method filter_method;
  string filter_payload;
  size_t div_start;  // The index (into m) of the row of the division
  size_t cur_div_len; 
  method m;
  bool maintain_r;   // Whether r is valid
  row r;
  scoped_pointer<prover> prv;
  time_t start;
};


searcher::searcher( const arguments &args )
  : args(args),
    bells( args.bells ),
    lead_len( args.lead_len ),
    div_len( (1 + args.treble_dodges) * 2 ),
    sym_offset( args.hunt_bells && args.hunt_bells % 2 == 0
                ? (1 + args.treble_dodges) : 0 ), // XXX ALLIANCE
    search_limit( args.search_limit ),
    search_count( 0ul ), node_count( 0ul ),
    div_start( 0 ), cur_div_len( calc_cur_div_len() ),
    r( args.pends.rcoset_label( args.start_row ) ),
    maintain_r( args.avoid_rows.size() || args.row_matches.size() )
{
  reset();

  copy( args.startmeth.rbegin(), args.startmeth.rend(), 
        back_inserter(startmeth) );

}

void searcher::reset()
{
  m.clear();
  m.reserve( lead_len );
  div_start = 0; cur_div_len = calc_cur_div_len();

  if (maintain_r) 
    r = args.pends.rcoset_label( args.start_row );

  if ( args.true_lead && ( !args.sym ||  
         args.treble_dodges && !args.same_place_parity )  ||
       args.true_half_lead && ( args.pends.size() > 1 ||
         args.hunt_bells == 0 || args.treble_dodges > 1 ) ) 
  {
    prv.reset( new prover(1) ); // XXX n_extents
    for ( set<row>::const_iterator 
            i=args.avoid_rows.begin(), e=args.avoid_rows.end(); i != e; ++i )
      prv->add_row(*i);
    prv->add_row(r);
    assert( prv->truth() );
    maintain_r = true;
  }

  start = time(NULL);
}

inline void searcher::do_status( method const& m ) {
  if ( node_count % args.status_freq == 0 ) {
    if ( args.status ) output_status(m);
    if ( args.timeout && time(NULL) - start > args.timeout ) 
      throw timeout_exception();
  }
  ++node_count;
}

void searcher::filter( library const& in )
{
  for ( library::const_iterator i=in.begin(), e=in.end(); i!=e; ++i ) 
    {
      try {
        filter_method = i->meth();
        if ( !args.lead_len )
          lead_len = filter_method.length();
        if ( i->has_facet<litelib::payload>() )
          filter_payload = i->get_facet<litelib::payload>();
        else
          filter_payload.clear();
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
      do_status( filter_method );

      RINGING_ULLONG old_search_count = search_count;
      general_recurse();
      assert( m.length() == 0 );

      if ( args.invert_filter ) {
        assert( search_count == old_search_count ||
                search_count == old_search_count + 1 );
        if ( old_search_count == search_count ) {
          ++search_count;
          output_method( filter_method );
        } 
        else --search_count;
      }
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
      } else if ( args.random_count ) {
        for ( int i=0; args.random_count==-1 || i<args.random_count; ++i ) {
          try {
            s.search_limit = s.search_count + 1;
            s.general_recurse();
            assert( s.m.length() == 0 );
          } catch (timeout_exception) {}
          s.reset();
          if (s.search_count >= args.search_limit) break;
        }
      } else {
        s.general_recurse();
        assert( s.m.length() == 0 );
      }
    } 
  catch ( const exit_exception& ) {}
  catch ( const timeout_exception& ) {}
 
  if ( args.status ) clear_status();

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

void searcher::output_method( method const& meth )
{
  if ( !args.outputs.empty() ) {
    method_properties props( meth, filter_payload );

    if ( !args.quiet && args.status && args.outfile.empty() )
      clear_status();

    args.outputs.append( props );
  }
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

  assert( m.length() == size_t(lead_len-1) );

  if ( ! try_midlead_change(ch) || ! try_leadend_change(ch) ||
       args.hunt_bells % 2 == 1 && ! try_leadend_sym_change(ch) )
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

class prover2 {
private:
  void init() {
    for ( set<row>::const_iterator 
            i=args.avoid_rows.begin(), e=args.avoid_rows.end(); i != e; ++i )
      p.add_row(*i);
    assert( p.truth() );
  }

public:
  prover2( arguments const& args ) 
   : args(args), n_extents(1), p(n_extents), r(args.start_row) { init(); }

  prover2( arguments const& args, row const& r ) 
   : args(args), n_extents(1), p(n_extents), r(r) { init(); }

  struct raw {};
  prover2( arguments const& args, raw )
    : args(args), n_extents(1), p(n_extents), r(args.bells)
  {}

  bool prove( method::const_iterator i, method::const_iterator e ) {
    for ( ; p.truth() && i != e; ++i ) {
      p.add_row( args.pends.rcoset_label(r) );
      r *= *i;
    }
    return p.truth();
  }

  bool prove_lh( row const& lh ) {
    row const r2 = args.pends.rcoset_label(lh);
    if ( r2 != args.start_row )
      p.add_row(r2);
    return p.truth();
  }

  bool prove_lh() { return prove_lh(r); }

  bool prove_hl( change const& hlc ) {
    row const r2 = args.pends.rcoset_label(r);
    row const r3 = args.pends.rcoset_label(r*hlc);
    if ( r2 != r3 ) 
      p.add_row(r2);
    return p.truth();
  }
    

  bool truth() const { return p.truth(); }
  bool is_course_head() const { return r == args.start_row; }
  row const& current_row() const { return r; }

private:
  arguments const& args;
  int const n_extents;
  prover p;
  row r;
};

bool searcher::is_acceptable_method()
{
  if ( lexicographical_compare( m.begin(), m.end(), 
           args.startmeth.begin(), args.startmeth.end(),
           compare_changes ) )
    return false;

  if ( ! is_acceptable_leadhead( m.lh() ) )
    return false;

  if ( args.floating_sym )
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
  // Plain methods cannot be false in the half-lead, and if symmetric
  // cannot be false in the plain course.  Therefore we only need this 
  // test if (i) we have part ends; or (ii) we're not a symmetric plain 
  // method.
  // As a further optimisation, we could (but don't) move symmetric 
  // into the following block by folding the actual lhs and les into the
  // part end group.
  if ( args.true_lead 
         && ( args.pends.size() > 1 
              || !( args.sym && args.hunt_bells && !args.treble_dodges ) ) )
    {
      prover2 p(args);
      while ( p.prove(m.begin(), m.end()) &&
              args.true_course && !p.is_course_head() )
        ;

      if ( !args.true_course )  // i.e. if -Fl
        assert( p.truth() );

      // There doesn't seem any ideal solution as to what to do with the 
      // lead head row.  Arguably we shouldn't prove it as it's not 
      // part of the lead.  But that leads to odd things in -AU0 searches
      // where we're just looking for a block of rows.  So lets require that 
      // either it is true or it is the first row again.
      if ( !p.prove_lh() ) return false;
    }

  // Although treble-dodging methods with more than one dodge can run
  // false within a half lead, this is handled by the is_division_false
  // function.  We only care if (i) we have part ends; or (ii) we're 
  // looking for hunt-dominated method.  And as this test is a subset
  // of the previous one, we don't want to do them both.
  // If we adopt the above further optimisation, we need to also test
  // -Fl/-Fc for symmetric methods.
  else if ( args.true_half_lead 
            && ( args.pends.size() > 1 || args.hunt_bells == 0 )  )
    {
      prover2 p(args);
      if ( !p.prove(m.begin(), m.begin()+m.size()/2) )
        return false;
 
      // Similarly to above, we need to do something with the half-lead change.
      // We require that either the half-lead head is the same as the half-lead
      // end (modulo the part-end group), or that the half-lead head does not
      // cause the initial half-lead to run false.   As above, it's a bit
      // heuristical, but it seems to work.
      if ( args.sym && !p.prove_hl( m[m.size()/2-1] ) ) return false;

      if ( !args.sym && !args.doubsym ) {
        prover2 p2(args, p.current_row());
        if ( !p2.prove(m.begin()+m.size()/2, m.end()) )
          return false;
        if ( !p2.prove_lh() ) return false;
      }  
      else {
        if ( !p.prove_lh(m.lh()) ) return false;
      }
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
    method_properties props(m, filter_payload);
    if ( !expression_cache::b_evaluate( *i, props ) )
      return false;
  }

  return true;
}

inline bool searcher::push_change( const change& ch )
{
  m.push_back( ch );
  if ( maintain_r ) {
    r = args.pends.rcoset_label( r * ch );
    // We don't care about the lead head row.
    if (!( m.size() == lead_len ||
           // Or the half-lead with just -Fh (and not -Fl)
           !args.true_lead && args.true_half_lead && m.size() >= lead_len/2 )) {
      if ( prv && !prv->add_row(r) )
        return false;
      // Is this case still necessary?
      else if ( !prv && args.avoid_rows.find(r) != args.avoid_rows.end() )
        return false;
    }

    if ( m.size() < args.row_matches.size() 
         && args.row_matches[m.size()].bells() ) {
      music& mus = const_cast<music&>( args.row_matches[m.size()] );

      if (args.pends.size() > 1) {
        bool matched = false;
        for (group::const_iterator i(args.pends.begin()), e(args.pends.end());
               i != e; ++i)
          if (mus.process_row(*i * r))
            matched = true;
        if (!matched)
          return false;
      }
      else if (!mus.process_row(r))
        return false;
    }
  }
  if (m.length() == div_start + cur_div_len) {
    div_start += cur_div_len;
    cur_div_len = calc_cur_div_len();
  }
  return true;
}

inline void searcher::pop_change( row const* r_old )
{
  m.pop_back();
  if (div_start > m.length()) {
     div_start -= cur_div_len;
     cur_div_len = calc_cur_div_len();
  }
  if ( maintain_r ) {

    if (!( m.size() == lead_len-1 ||
           // Or the half-lead with just -Fh (and not -Fl)
           !args.true_lead && args.true_half_lead && m.size() >= lead_len/2-1 )
        && prv )
      prv->remove_row(r);
    if ( r_old ) r = *r_old;  
    else r = args.pends.rcoset_label( m.lh() );
  }
}

inline void searcher::call_recurse( const change &ch )
{
  // Store old value of r -- this is considerably cheaper than calling
  // m.lh() and still a little better than restoring with r *= ch.
  row old;
  if ( maintain_r ) old = r;

  if ( push_change( ch ) )
    general_recurse();

  pop_change( &old );
}

// The length of the current division
inline size_t searcher::calc_cur_div_len() const 
{
  return (1 + args.treble_dodges) * 2;
}


// 0 = treble in 1-2
// 1 = treble in 2-3
// 2 = treble in 3-4, ...
inline int searcher::get_posn() const
{
  // XXX ALLIANCE -- Whole function needs reworking

  assert( args.hunt_bells );
  assert( lead_len % 2 == 0 );

  const size_t depth = m.length();

  // Reflect about the half-lead:
  size_t posn = depth >= lead_len/2 ? lead_len - depth - 2 : depth;
  
  if ( posn % div_len == div_len - 1 )
    return args.treble_front-1 + posn / div_len * 2 + 1;
  else
    return args.treble_front-1 + posn / div_len * 2;
}

inline bool searcher::is_cyclic_hl( const row& hl )
{
  assert( hl[ bells-1 ] == 0 );
  
  for (int i=1; i<bells-1; ++i)
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

      if ( ! is_cyclic_hl(hl) )
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
  if ( (args.skewsym || args.doubsym) && args.require_limited_le 
       && !is_limited_le( ch.reverse() ) )
    return false;

  if ( (args.skewsym || args.doubsym) &&
       args.no_78_pns && ch.findplace(1) )
    return false;

  size_t stopoff = args.long_le_place 
    ? (lead_len+sym_offset-1) % lead_len : (size_t)-1;
  if ( args.sym && args.max_consec_blows
       && is_too_many_places( m, ch, args.max_consec_blows/2+1, stopoff ) )
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
          if (args.long_le_place && !sym_offset) count=1;
         
          for ( int offset = m.length() - 1; 
                offset >= 0 && count <= args.max_consec_blows; 
                --offset, ++count )
            if ( !m[offset].findplace(i) )
              break;

          if ( count > args.max_consec_blows )
            return false;


          if (args.long_le_place && !sym_offset) count=1; 

          int end = m.length();
          if (args.long_le_place && sym_offset) end = sym_offset;

          for ( int offset = 0; 
                offset < end && count <= args.max_consec_blows; 
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

  if ( args.require_limited_le && !is_limited_le(ch) ) 
    return false;

  if ( args.no_78_pns && ch.findplace(bells-2) )
    return false;

  if ( args.sym && !args.long_le_place && args.max_consec_blows
       && is_too_many_places( m, ch, args.max_consec_blows/2+1 ) )
    return false;

  return true;
}

bool searcher::try_midlead_change( const change &ch )
{
  // Don't test falseness (except -Fn) here because it is called with from
  // try_with_limited_le which handles -E.

  size_t depth = m.size();

  if ( args.true_trivial && m.size() && m.back() == ch )
    return false;

  size_t const posn = args.hunt_bells ? get_posn() : 0;

  // XXX ALLIANCE -- Needs fixing
  // Is the treble moving between dodging positions (i.e. is posn odd)?
  bool const intersection = ( args.hunt_bells && 
                              posn % 2 == args.treble_front % 2 );

  if ( intersection && depth != lead_len-1 && depth != lead_len/2 -1 )
    {
      assert( lead_len % 2 == 0 );
      size_t hl_len = lead_len / 2;

      if ( args.surprise && !ch.internal() )
        return false;
  
      if ( args.treble_bob && ch.internal() )
        return false;

      if ( args.delight3 && ( posn == 3 && !ch.internal() ||
                              posn == 1 &&  ch.internal() ) )
        return false;

      if ( args.delight4 && ( posn == 1 && !ch.internal() ||
                              posn == 3 &&  ch.internal() ) )
        return false;
         
      // Are we at the last cross-section that is chosen independently?
      // (Cross-sections that are determined purely by copying or reflecting 
      // a previous change do not pass through this code path.)  If so, 
      // determine whether the method is a valid delight method (or exercise
      // or whatever).   Surprise and treble bob are handled above and are
      // much easier as they're the all or nothing cases.  The cases here 
      // require us to counting how many cross-sections have internal places.
      //
      // TODO: Pas-alla-tessera (and the others less so) can be sped up
      // by testing this at each division end.
      //
      if ( args.delight || args.exercise ||
           args.strict_delight || args.strict_exercise ||
           args.pas_alla_tria || args.pas_alla_tessera )
      {
        const int f = args.treble_front, b = args.treble_back, p = posn;

        if ( // If we're palindromic or glide symmetric (but not both) 
             // the last independent cross-section is when the treble is 
             // e.g. in 10-11 for maximus.
             (args.sym ^ args.doubsym) && p == b - 3 ||
             // If we're maximally symmetric, it's when the treble's in 
             // 6-7 for fourteen or maximus, or 4-5 for royal or major.
             args.sym && args.doubsym && p == (f + (b-f+1)/2 - 1)/2*2-1 ||
             // If we're rotationally symmetric, it's when the treble's in 
             // 6-7 down for max or royal, 4-5 for major or minor
             args.skewsym && p == (f + (b-f+1)/2)/2*2 - 1 && depth > hl_len ||
             // If we're asymmetric, the treble is in 2-3 on the way down.
             !args.sym && !args.doubsym && !args.skewsym 
                       && p == f && depth > hl_len ) 
        {
          // Count how many external (i.e. 1N) cross-sections there are:
          int external_cross_sections = 0, cross_sections = 0;
          for ( int i = div_len-1; i < depth; i += div_len ) 
          {
            if (i == hl_len-1) 
              continue;  // Don't include the h.l.
  
            // If we have double symmetry, count the cross-section twice:
            // TODO: This logic is incorrect for methods that are just 
            // rotational symmetric, but we don't care because --exercise et 
            // al. are not enabled for non-palindromic methods.
            int value = 1;
            if ( args.skewsym && i%hl_len != hl_len/2 - 1 ) value = 2;
     
            if (!m[i].internal()) external_cross_sections += value;
            cross_sections += value;
          }
          {
            int value = 1;
            if ( args.skewsym && (b-f)%4 == 1 ) value = 2;
            if (!ch.internal()) external_cross_sections += value;
            cross_sections += value;
          }
  
          if (external_cross_sections == 0 ||            // Surprise
              external_cross_sections == cross_sections) // Treble Bob
            return false;
  
          // Old classes:
          if (args.strict_delight && external_cross_sections != 1)
            return false;
          if (args.exercise && external_cross_sections < 2)
            return false;
          if (args.strict_exercise && external_cross_sections != 2)
            return false;
          if (args.pas_alla_tria && external_cross_sections != 3)
            return false;
          if (args.pas_alla_tessera && external_cross_sections != 4)
            return false;
        }
      }
    }
  
  // Are we more than half way through the current division? 
  if ( args.sym_sects && !intersection && depth - div_start >= cur_div_len/2 )
    {
      int i = div_start + cur_div_len - 2 - (depth - div_start);
      assert( i < (int)m.size() && i >= 0 );
      if ( ch != m[i] )
        return false;
    }

  // The 'parity hack' 
  if ( args.same_place_parity && cur_div_len == 4
       && depth - div_start != 0 && depth - div_start != cur_div_len - 1 
       && ch.sign() == m.back().sign() )
    return false;
 
  if ( args.max_consec_blows )
    {
      size_t stopoff = args.long_le_place 
        ? (lead_len+sym_offset-1) % lead_len : (size_t)-1;

      if ( args.long_le_place && (depth+1) % lead_len == sym_offset )
        ;

#if 0
      // Handle places around the lead end.
      else if ( args.sym && args.sym_offset 
           && depth == args.max_consec_blows/2 
           && is_too_many_places( m, ch, args.max_consec_blows/2+1 ) ) {
cerr << "SOUP: " << ch << " at depth " << m.length() << endl;
        assert(false);  // This case shouldn't be happening
        return false;
      }
#endif

      else if ( is_too_many_places( m, ch, args.max_consec_blows, stopoff ) )
        return false;
    }
 
  // We've just completed a section.  If there is more than one dodge
  // per position in the treble's path, then there is the possibility of 
  // an individual section being internally false.  (With a single 
  // dodge, this can only happen with a repeated change which is eliminated
  // by the default -Fn.)
  // This test doesn't effect the -E handling noted above as if the base
  // method passes this, so will the variant with a 12 or 1N lh.
  if ( args.true_half_lead && cur_div_len > 4 && !intersection
       && is_division_false( m, ch, div_start, cur_div_len ) )
    return false;
  
  if ( args.same_place_parity && cur_div_len > 4
       && depth - div_start == cur_div_len - 2 
       && division_bad_parity_hack( m, ch, div_start, cur_div_len ) )
    return false;

  if ( ( args.allowed_falseness.size() || args.require_CPS ) 
       && depth - div_start >= 1 && depth - div_start != cur_div_len - 1
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
  if ( args.allowed_falseness.size() ) {
    if ( sym == "A" || sym.empty() ) return true;
    if ( args.allowed_falseness.find(sym) == string::npos ) return false;
  } 
  if ( args.require_CPS ) {
    if ( sym.empty() || strchr("AabcdefXYZ", sym[0]) ||
         sym[1] == '2' && strchr("BCDEFHKNOT", sym[0]) ||
         sym[1] == '3' && strchr("KN", sym[0])  )
      return true;
    else return false;
  }
  return true;
}


bool searcher::try_quarterlead_change( const change &ch )
{
  assert( lead_len % 2 == 0 );
  size_t hl_len = lead_len / 2;

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

  // When we're automatically determining the lead length, we only have 
  // two change lists: one for h'stroke and one for b'stroke.
  vector< change > changes_to_try 
    = args.allowed_changes[args.lead_len ? depth : depth % 2];
  assert( changes_to_try.size() );

  // We explicitly use random_int_generator as we know how to seed that.
  // There is no guarantee that the STL's two argument random_shuffle
  // will use rand() and hence respect srand().
  if ( args.random_order ) {
    unsigned (*random_number_generator)(unsigned) = &random_int;
    random_shuffle( changes_to_try.begin(), changes_to_try.end(), 
                    random_number_generator );
  }

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

      // XXX ALLIANCE -- locate rotational symmetry point
      // Additional requirements for the rotational symmetry point:
      if ( args.hunt_bells && args.skewsym && lead_len % 4 == 0 
           && depth % (lead_len/2) == lead_len / 4 - args.hunt_bells % 2 &&
           ! try_quarterlead_change( ch ) )
        continue;

      // Additional requirements for the half-lead:
      if ( depth == lead_len/2-1 )
        if ( ! try_halflead_change( ch ) )
          continue;

      // Additional requirements for the palindromic symmetry point of the 
      // treble's path near the middle of the lead.  For single hunt methods,  
      // this is the half-lead; for twin-hunt methods, it is shifted.
      if ( args.hunt_bells % 2 == 1 && depth == lead_len/2 - 1 ||
           args.hunt_bells && 
           args.hunt_bells % 2 == 0 && depth == lead_len/2 + cur_div_len/2 - 1 )
        if ( ! try_halflead_sym_change( ch ) )
          continue;
     
      // Additional requirements for the lead-end: 
      if ( depth == size_t(lead_len-1) )
        if ( ! try_leadend_change( ch ) )
          continue;
      
      // Additional requirements for the palindromic symmetry point of the
      // treble's path near the lead end.  For single hunt methods, this is
      // the lead-end; for twin-hun methods, it is shifted to the start of
      // the lead (e.g. in Grandsire, it is the 3 at the start of the lead).
      if ( !sym_offset && depth == lead_len-1 ||
           sym_offset && depth == sym_offset-1 )
        if ( ! try_leadend_sym_change( ch ) )
          continue;
      
      if ( args.hunt_bells && args.require_offset_cyclic 
           // XXX ALLIANCE -- First division
           && div_start == 0 && cur_div_len > 3 && depth == cur_div_len-3 )
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

  assert( lead_len % 2 == 0 );
  size_t hl_len = lead_len / 2;
  assert( size_t(m.length()) == hl_len );

 
  // Note: this loop does not add the lead-end change
  bool ok = true;
  for ( size_t depth = m.length(); ok && depth < lead_len-1; ++depth )
    ok = push_change( m[ depth-hl_len ].reverse() );

  change ch( m[ hl_len-1 ].reverse() );
  
  // Not sure if this is still necessary ...
  if ( args.max_consec_blows )
    for ( int i=0; ok && i<bells; ++i )
      if ( ch.findplace(i) )
        {
          int count(2);
          
          for ( int offset = m.length()-1; offset >= 0; --offset, ++count )
            if ( !m[offset].findplace(i) )
              break;

          for ( int offset = 0; offset < m.length(); ++offset, ++count )
            if ( !m[offset].findplace(i) )
              break;
          
          if ( count > args.max_consec_blows ) ok = false;
        }
 
  // This is the lead-head change 
  if ( ok && push_change( ch ) )
    general_recurse();

  while ( size_t(m.length()) > hl_len )
    pop_change();
}

// -----------------------------------------


bool searcher::is_acceptable_leadhead( const row &lh )
{
  if ( ! args.show_all_meths )
    {
      string cycles = lh.cycles();
      string::size_type i = 0;
      while (true) {
        string::size_type j = cycles.find( ',', i );
        if ( j == string::npos ) break;
        else if ( j == i+1 ) {
          bell h = bell::read_char(cycles[i]);
          if ( h < args.treble_front-1 
              || h >= args.treble_front-1+args.hunt_bells )
            return false;
        }
        else if ( j != i+args.bells-args.hunt_bells )
          return false;
        i = j+1;
      }
    }

  if ( args.require_pbles ) {
    if ( lh.ispblh(args.hunt_bells) )
      return true;
  }
  else if ( args.require_cyclic_les ) {
    if ( is_cyclic_le(lh, args.hunt_bells) )
      return true;
  }
  else if ( args.any_regular_le ) {
    unsigned h = 0;
    for (unsigned i=0; i < bells; ++i)
      if (lh[i] == i) ++h;
      else break;
    if ( lh.ispblh(h) )
      return true;
  }
  else return true;

  return false;
}

void searcher::general_recurse()
{
  const size_t depth = m.length();

  // XXX ALLIANCE -- These assertions will fail
  assert( depth - div_start == depth % div_len );
  assert( cur_div_len == div_len );

  if ( search_limit && search_limit != -1 && 
       search_count == search_limit )
    return;

  // Status message (when in search mode)
  if ( !args.filter_mode ) do_status(m);

  // XXX ALLIANCE Is the lead_len % 4 test valid? 
  const bool has_qlead_change = lead_len % 4 == 0;

  const size_t  qlead_sym_pt =   lead_len/4 - 1 + sym_offset;
  const size_t  hlead_sym_pt =   lead_len/2 - 1 + sym_offset;
  const size_t tqlead_sym_pt = 3*lead_len/4 - 1 + sym_offset;

  // Found something
  if ( depth == size_t(lead_len) )
    {
      if ( filter_method.size() && filter_method != m )
        ;
      else if ( is_acceptable_method() ) {
        if ( !args.invert_filter ) 
          output_method(m);

        // This really should be outside the invert_filter test --
        // counts are inverted in the filter() function when inverting.
        ++search_count;
      }
    }

  // Symmetry in principles is not handled until later, because we cannot
  // be sure where the symmetry points will be.
  else if ( args.floating_sym )
    {
      new_midlead_change();
    }

  // Maximum symmetry is handled here
  else if ( args.skewsym && depth == qlead_sym_pt + 1 && lead_len > 4 )
    {
      // If we have a qlead change at Q, then we want m[Q+i] = m[Q-i]
      // m.length() is Q+i, so Q-i = 2*Q - m.length().
      // If there is no qlead change and Q is the last before it, then
      // we want m[Q+i] = m[Q-i+1], so Q-i-1 = 2*Q - m.length() + 1
    
 
      bool ok = true;

      // This loop will add up to the half-lead symmetry point, unless we 
      // have glide symmetry, in which case stop at the half-way point, 
      // when the glide symmetry loop allows us to complete.
      size_t end = hlead_sym_pt;
      if ( args.doubsym && sym_offset ) end = lead_len/2;
      else if ( sym_offset ) end += sym_offset;

      while ( ok && size_t(m.length()) < end )
        ok = ok && push_change( m[ 2*qlead_sym_pt - m.length() 
                                   + (has_qlead_change ? 0 : 1) ].reverse() );

      if (ok) general_recurse();

      while ( size_t(m.length()) > depth )
        pop_change();
    }

  // Double (glide) symmetry (with or without others)
  else if ( args.doubsym && depth == lead_len/2 )
    {
      double_existing();
    }

  // Only conventional (palindromic) symmetry
  else if ( args.sym && depth == hlead_sym_pt + 1 && lead_len > 2 )
    {
      assert( !args.skewsym && !args.doubsym );

      bool ok = true;
      for (method::reverse_iterator i = m.rbegin() + 1,
             e = m.rend() - (sym_offset ? 2*sym_offset-1 : 0); 
             ok && i != e; ++i)
        ok = ok && push_change(*i);
          
      if (ok) general_recurse();

      while ( size_t(m.length()) > depth )
        pop_change();
    }

  // Conventional symmetry when we have an even number of hunt bells 
  // treble dodging -- fold the first division (when two hunts are doging 1-2)
  else if ( args.sym && sym_offset > 1 && depth == sym_offset )
    {
      bool ok = true;
      for (method::reverse_iterator i = m.rbegin()+1, e = m.rend(); 
             ok && i!=e; ++i)
        ok = ok && push_change(*i);

      if (ok) general_recurse();

      while ( size_t(m.length()) > depth )
        pop_change();
    }

  // Only rotational symmetry
  else if ( args.skewsym && depth == tqlead_sym_pt + 1 && lead_len > 4 )
    {
      assert( !args.doubsym && !args.sym );

      bool ok = true;
      // This loop *will* add the lead end change.
      while ( ok && size_t(m.length()) < lead_len ) 
        ok = ok && push_change( m[ 2*tqlead_sym_pt - m.length()
                                   + (has_qlead_change ? 0 : 1) ].reverse() );

      if (ok) general_recurse();

      while ( size_t(m.length()) > depth )
        pop_change();
    }

  // Need a new change
  else
    {
      new_midlead_change();
    }
}
