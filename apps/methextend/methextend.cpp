// -*- C++ -*- extend.cpp - find extensions of methods
// Copyright (C) 2017, 2022 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/mathutils.h>
#include <ringing/lexical_cast.h>
#include "args.h"
#include <iostream>
#include <algorithm>
#include <map>

RINGING_USING_NAMESPACE
RINGING_USING_STD

// Expand all places from PLACE inclusive by BY.  Because the numeric 
// value of the bell class is zero-based, to effect a mode N extension
// (below the treble), simply pass N as PLACE.
change expand_change(size_t bells, vector<bell> places, bell split, int by) {
  if (by <= 0 || by % 2) throw change::out_of_range();
  for (bell& i : places)
    if ( i >= split ) 
      i += by;
  return change(bells + by, places);
}

// The treble is hunting between places P1 and P2.  Return the section.
// 0 = A (treble leading), 1 = B (treble in 1-2), 2 = C (treble in 2-3), etc.
static int calc_sect( bell const& p1, bell const& p2 ) {
  if (p1 < p2) return p2;
  else if (p1 > p2) return p1;
  else if (p1 == 0) return 0;
  else return p1 + 1;
}

// Copy SRC to DEST and clear the contents of SRC.
static void copy_method_frag(method& dest, vector<change> &src) {
  if (src.size()) {
    for (change const& c : src)
      dest.push_back(c);
    src.clear();
  }
}

// Represent an extension above or below the treble, e.g. 2DEFG. 
// * SECT is 0 = A (treble leading), 1 = B (treble in 1-2), 2 = C (2-3), etc.
// * MODE is as described in (G)C.2(b)(i) and (c)(i).  A mode-m extension below 
// treble expands (away from the front) all internal places above m-ths place; 
// a mode-m extension above the treble expands (away from the back) all 
// internal places below m-ths place from the back.
// * BY is the number of stages incremented by an individual extension step,
// so 2DE has by=2, even if for other reasons the extension only works
// every 4th or 6th stage; 2DEFG has by=4.
struct extension_description {
  extension_description() : sect(0), mode(0), by(0) {} 

  extension_description(int sect, int mode, int by) 
    : sect(sect), mode(mode), by(by) {}

  // The standard string representation.
  friend ostream& operator<<(ostream& os, const extension_description& e) {
    os << e.mode;
    for (int i=0; i<e.by; ++i)
      os << char('A' + e.sect + i);
    return os;
  }

  operator string() const { 
    return make_string() << *this;
  }

//  // Allow it to be put into a std::set
//  friend bool operator<( extension_description const& a,
//                         extension_description const& b ) {
//    return a.mode < b.mode || a.mode == b.mode && 
//         ( a.sect < b.sect || a.sect == b.sect &&
//         ( a.by   < b.by   || a.by   == b.by ) );
//  }

  int sect, mode, by;
};


// Both (G)B.7-8 say "wherever ... this characteristic must be retained in
// all extensions."  This is ambiguous.  When a change gets repeated as 
// a result of the extension, foes "retained" mean the characteristic must 
// be retained in both places or just one?  Set multiple_retention = true
// for both.
// Tony Smith's letter to the RW re Chogolisa S Max (RW 2017, p 365)
// demonstrates that he believes that only one instance of the feature is
// required.  This concurs with what Philip Saddleton has said in the past.
static const bool multiple_retention = false;

// Unclear whether mode A is allowed below, and the equivalent above.
static const bool allow_A_below = false;

bool valid_extension_below(method const& meth, extension_description const& e) {
  bell b(0);
  for (change const& c : meth) {
    bell b2( b*c );
    int s = calc_sect( b, b2 );

    // For (G)B.7-8, we only care about the middle bit of the method.
    if (s >= (multiple_retention ? e.sect : e.sect + e.by)) {
    
      // (G)B.8: "Wherever the parent has working bells making adjacent places,
      // this characteristic must be retained in all extensions."
      //
      // We only split places if they're in M, M+1, where M is the extension
      // mode (but note that the argument to change::findplace is zero-based).
      // Finally, we only care if these places are below the treble which is 
      // currently in S, S+1.
      if ( e.mode < s && c.findplace(e.mode) && c.findplace(e.mode-1))
        return false;
  
      // (G)B.7: "Wherever the parent has a place made adjacent to the path of
      // a hunt bell, this characteristic must be retained in all extensions."
      //
      // The treble is currently in S, S+1, so the place below the treble is
      // S-1 (but again note that change::findplace is zero-based); the latter
      // is also true at the half-lead, where S=N.  (The test that s >= 2 is
      // because change::findplace takes a bell, which is internally an
      // unsigned char, so a negative s-2 is bad, m'kay?) Finally, we only care
      // if the place is below M+1 (the lowest unextended place), as otherwise
      // it is not being extended.  
      if ( s-1 < e.mode+1 && s >= 2 && c.findplace(s-2) )
        return false; 
    }

    b = b2;
  }

  // N-1-ths at the half-lead is a bit special as the treble is one
  // of the bells making the place.  Mode n-1, section n-by+1 (e.g FG in minor)
  // extensions are therefore only allowed under (G)B.8 if there is not an 
  // n-1-ths place half-lead.
  if (e.mode == meth.bells()-1 && e.sect == meth.bells()-e.by+1 
       && meth[ meth.size() / 2 - 1 ].findplace(meth.bells()-2))
    return false;

  return true;
}

method extend_below_work(method const& m, extension_description const &e) {
  bell b(0);
  int const n( m.bells() );
  method m2;
  vector<change> x;
  if ( e.sect == 0 )
    x.push_back(expand_change(n, m.back().places(), 1, e.by));
  
  bool first_half = true;
  for (change const& c : m) {
    bell b2( b*c );
    int s = calc_sect( b, b2 );

    vector<bell> places;
    for ( bell p : c.places() )
      if ( p <= s )
        places.push_back(p);

    // This is the place above which we expand the change
    int pl = s+1 < e.mode ? s+1 : e.mode;
    change const c1(expand_change(n, places, n-1, e.by));
    change const c2(expand_change(n, places, pl, e.by));

    if (s < e.sect) {
      copy_method_frag(m2, x);
      m2.push_back(c1);
    }
    else if (s >= e.sect && s < e.sect + e.by) {
      if (first_half) {
        m2.push_back(c1);
        x.push_back(c2);
      }
      else {
        m2.push_back(c2); 
        x.push_back(c1);    
      }
    }
    else {
      copy_method_frag(m2, x);
      m2.push_back(c2);
    }
    if ( b == b2 && first_half ) {
      first_half = false;
      copy_method_frag(m2, x);
      if  (s == e.sect + e.by - 1)
        x.push_back(c1);
    }
    b = b2;
  }
  copy_method_frag(m2, x);
  return m2;
}

method 
extend_below_work(method const& m, extension_description const &b, int stage) {
  method below(m); 
  do {
    below = extend_below_work( below, b );
  } while ( stage && below.bells() < stage );

  if (!stage || below.bells() == stage) 
    return below;
  return method();
}


bool valid_extension_above(method const& meth, extension_description const& e) {
  bell b(0);
  for (change const& c : meth) {
    bell b2( b*c );
    int const n( meth.bells() );
    int s = calc_sect( b, b2 );

    // For (G)B.7-8, we only care about the first bit of the method.
    if ( s < (multiple_retention ? e.sect + e.by : e.sect) ) {
  
      // (G)B.8: "Wherever the parent has working bells making adjacent places,
      // this characteristic must be retained in all extensions."
      //
      // We only split places if they're in N-M and N-M+1, where N is the
      // number of bells and M is the extension mode (but note that argument to
      // change::findplace is zero-based).  Finally, we only care if these
      // places are above the treble which is currently in S, S+1.
      if ( n-e.mode > s && c.findplace(n-e.mode) && c.findplace(n-e.mode-1) )
        return false;
  
      // (G)B.7: "Wherever the parent has a place made adjacent to the path of
      // a hunt bell, this characteristic must be retained in all extensions."
      //
      // The treble is currently in S, S+1, so the place above the treble is
      // S+2 (but again note that change::findplace is zero-based); the latter
      // is also true for section A, where S=0.  Finally, we only care if the
      // place is above N-M (the highest unextended place), as otherwise it is
      // not being extended.
      if ( s+2 > n-e.mode && c.findplace(s+1) )
        return false;
    }

    b = b2;
  }

  // Seconds at the lead-end place is a bit special as the treble is one
  // of the bells making the place.  Mode n-1 AB extensions are therefore
  // only allowed under (G)B.8 if there is not a 12 lead end.
  if (e.mode == meth.bells()-1 && e.sect == 0 && meth.back().findplace(1))
    return false;
   
  return true;
}


method extend_above_work(method const& m, extension_description const &e) {
  bell b(0);
  int const n( m.bells() );
  method m2;
  vector<change> x;
  if ( e.sect == 0 )
    x.push_back(expand_change(n, m.back().places(), 1, e.by));

  bool first_half = true;
  for (change const& c : m) {
    bell b2( b*c );
    int s = calc_sect( b, b2 );

    vector<bell> places;
    if (s && s % 2 == 0)
      places.push_back(0); // A place at lead 
    for ( bell p : c.places() )
      if ( p >= s )
        places.push_back(p);

    // This is the place above which we expand the change
    int pl = s-1 > n-e.mode ? s-1 : n-e.mode;
    change const c1(expand_change(n, places, 1, e.by));
    change const c2(expand_change(n, places, pl, e.by));
    if (s < e.sect) {
      copy_method_frag(m2, x);
      m2.push_back(c2);
    }
    else if (s >= e.sect && s < e.sect + e.by) {
      if (first_half) {
        m2.push_back(c2);
        x.push_back(c1);
      }
      else {
        m2.push_back(c1); 
        x.push_back(c2);    
      }
    }
    else {
      copy_method_frag(m2, x);
      m2.push_back(c1);
    }
    if ( b == b2 && first_half ) {
      first_half = false;
      copy_method_frag(m2, x);
      if  (s == e.sect + e.by - 1)
        x.push_back(c2);
    }
    b = b2;
  }
  copy_method_frag(m2, x);
  return m2;
}

method 
extend_above_work(method const& m, extension_description const &a, int stage) {
  method above(m); 
  do {
    above = extend_above_work( above, a );
  } while ( stage && above.bells() < stage );

  if (!stage || above.bells() == stage)
    return above;
  return method();
}

change merge_change( change const& below, change const& above, bell pos ) {
  vector<bell> places;
  for ( bell b : below.places() )
    if ( b < pos ) places.push_back(b);
  for ( bell b : above.places() ) 
    if ( b >= pos ) places.push_back(b);
  return change(above.bells(), places);
}

method merge_above_below( method const& above, method const& below ) {
  bell b(0);
  method m;
  if (above.length() != below.length() || above.bells() != below.bells()) 
    throw out_of_range("Merging incompatible methods");

  for (int i=0; i<above.size(); ++i) {
    bell b1( b*above[i] ), b2( b*below[i] );
    if ( b1 != b2 ) 
      throw out_of_range("Merging methods with different treble paths");
    int s = calc_sect( b, b2 );
    m.push_back( merge_change(below[i], above[i], s) );
    b = b2;
  }
  return m;
}

bool is_valid_extension( method const& base, method const& m ) {
  // (G)B.2 "The extension of a method must have the same symmetry as 
  // the parent."  Note our code only works on palindromic methods.
  if ( m.isdouble() != base.isdouble() ) return false;

  // (G)B.3 "The extension of a method must have the same number of hunt
  // bells as the parent."
  if ( m.huntbells() != base.huntbells() ) return false;
  
  // (G)B.5 "The extension of a method with Plain Bob-type lead-heads must
  // also have Plain Bob-type lead-heads. The extension of a method with
  // non-Plain Bob-type lead-heads must also have non-Plain Bob-type
  // lead-heads."
  if ( m.isregular() != base.isregular() ) return false;

  // (G)B.4 "The extension of a method must have the same number of cycles
  // of working bells as the parent."
  string mcyc( m.lh().cycles() ), basecyc( base.lh().cycles() );
  if ( std::count(mcyc.begin(), mcyc.end(), ',') !=
       std::count(mcyc.begin(), mcyc.end(), ',') ) return false;

  return true;
}

method extend_method( method const& base, extension_description const &a,
                      extension_description const &b, int stage ) {
  method above( extend_above_work( base, a, stage ) );
  method below( extend_below_work( base, b, stage ) );
  return merge_above_below( above, below );
}

class arguments : public arguments_base {
public:
  init_val<int,0>  bells;

  string basestr;
  method base;

  init_val<int,0>         required_bells;
  init_val<bool,false>    include_invalid;
  init_val<bool,false>    show_above;
  init_val<bool,false>    show_below;
  init_val<bool,false>    show_methods;
  init_val<bool,false>    show_stages;
  init_val<int,60>        max_stage;

  arguments( int argc, char* argv[] );

  void bind( arg_parser &p );
  bool validate( arg_parser &p );
};

arguments::arguments( int argc, char* argv[] ) {
  arg_parser(argv[0], "methext -- find extensions of methods.", 
             "OPTIONS METHOD" )
    .process( *this, argc, argv );
}

void arguments::bind( arg_parser &p ) {
  p.add( new help_opt );
  p.add( new version_opt );

  p.set_default( new method_opt( '\0', "", "", "", basestr, bells, base ) );

  p.add( new bells_opt( bells ) );

  p.add( new integer_opt
	 ( 'B', "required-bells",  
	   "The number of bells in the extension", 
           "BELLS", required_bells ) );

  p.add( new boolean_opt
	 ( '\0', "include-invalid",  
	   "Include extensions that fail to preserve a required feature",
	   include_invalid ) );

  p.add( new boolean_opt
	 ( '\0', "show-above",  
	   "Show possible extensions of the above-work",
	   show_above ) );

  p.add( new boolean_opt
	 ( '\0', "show-below",  
	   "Show possible extensions of the below-work",
	   show_below ) );

  p.add( new boolean_opt
	 ( '\0', "show-methods",  
	   "Show possible extensions.  This is the default unless "
           "--show-above or --show-below are used",
	   show_methods ) );

  p.add( new boolean_opt
	 ( '\0', "show-stages",  
	   "Show the stages that the extensions work on, up to --max-stages",
	   show_stages ) );

  p.add( new integer_opt
	 ( '\0', "max-stage",  
	   "The highest stage to check.  The default is 60", "NUMBER",
	   max_stage ) );
}

bool arguments::validate( arg_parser& ap ) {
  if ( bells < 4 || bells > int(bell::MAX_BELLS) ) {
    ap.error( make_string() << "The base number of bells must be between 4 " 
	      "and " << bell::MAX_BELLS << " (inclusive)" );
    return false;
  }

  if ( max_stage <= bells || max_stage > 255) {
    ap.error( make_string() << "The maximum stage must be between " 
                            << (bells+1) << " and 256 (inclusive)" );
    return false;
  }

  return true;
}

vector<extension_description> enumerate_below_works( arguments const& args ) {
  map<method, extension_description> below;
  map<method, string> below_names;
  for (int by = 2; by <= args.bells; by += 2)
    for (int sect=1; sect+by <= args.bells+1; ++sect)
      for (int mode=(allow_A_below ? 0 : 1); mode < args.bells; ++mode) {
        extension_description desc(sect, mode, by);
        if (args.include_invalid || valid_extension_below(args.base, desc)) {
          method work
            ( extend_below_work(args.base, desc, args.required_bells) );
          if (work.size()) {
            if (!below.count(work)) below[work] = desc;
            below_names[work] += '=';
            below_names[work] += desc;
          }
        }
      }

  if (args.show_below)
    for (auto b : below_names)
      cout << b.first.format( method::M_DASH | method::M_SYMMETRY ) << "\t" 
           << b.second.substr(1) << " below\n";

  vector<extension_description> v; v.reserve(below.size());
  for ( auto b : below ) v.push_back( b.second );
  return v;
}

vector<extension_description> enumerate_above_works( arguments const& args ) {
  map<method, extension_description> above;
  map<method, string> above_names;
  for (int by = 2; by <= args.bells; by += 2)
    for (int sect=0; sect+by <= args.bells + (allow_A_below ? 1 : 0); ++sect)
      for (int mode=1; mode < args.bells; ++mode) {
        extension_description desc(sect, mode, by);
        if (args.include_invalid || valid_extension_above(args.base, desc)) {
          method work
            ( extend_above_work(args.base, desc, args.required_bells) );
          if (work.size()) {
            if (!above.count(work)) above[work] = desc;
            above_names[work] += '=';
            above_names[work] += desc;
          }
        }
      }

  if (args.show_above)
    for (auto a : above_names)
      cout << a.first.format( method::M_DASH | method::M_SYMMETRY ) << "\t" 
           << a.second.substr(1) << " above\n";

  vector<extension_description> v; v.reserve(above.size());
  for ( auto a : above ) v.push_back( a.second );
  return v;
}

int main( int argc, char* argv[] )
{
  try {
    bell::set_symbols_from_env(); 
    arguments args( argc, argv );
  
    vector<extension_description> above( enumerate_above_works(args) ),
                                  below( enumerate_below_works(args) );
  
    if ( !args.show_above && !args.show_below || args.show_methods )
      for ( auto a : above )
        for ( auto b : below ) {
          /* int flags( method::M_DASH | method::M_SYMMETRY );
          cout << "Trying " << a.second << "=" << a.first.format(flags) << " / "
               << b.second << "=" << b.first.format(flags) << endl; */
 
          int by = lcm( a.by, b.by );
          method m0(args.base), m1;

          int max_stage;
          if (args.show_stages) max_stage = args.max_stage;
          else if (args.required_bells) max_stage = args.required_bells;
          else max_stage = m0.bells() + by;

          vector<int> stages;
          while ( m0.bells() + by <= max_stage ) {
            int stage = m0.bells() + by;
            method m( extend_method( m0, a, b, stage ) );
            if (is_valid_extension(args.base, m)) {
              if (!args.required_bells && m1.empty() 
                  || args.required_bells == stage) m1 = m;
              stages.push_back(stage);
            }
            else if (args.include_invalid) {
              if (!args.required_bells && m1.empty() 
                  || args.required_bells == stage) m1 = m;
            }
    
            m0 = m;
          }
    
          if (m1.size()) {
            if (!args.required_bells)
               cout << m1.bells() << ':';
            cout << m1.format( method::M_DASH | method::M_SYMMETRY ) << "\t"
                 << a << "/" << b;
            if (args.show_stages) {
              cout << "  (";
              for ( auto i : stages ) cout << ' ' << i; 
              cout << " )";
            }
            cout << "\n";
          }
        }
  
    return 0;
  }
  catch (exception const& ex) {
    cerr << ex.what() << endl;
    exit(1);
  }
}
