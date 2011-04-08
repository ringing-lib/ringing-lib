// -*- C++ -*- spliceplan.cpp - generate a set of mutually true leads
// Copyright (C) 2010, 2011 Richard Smith <richard@ex-parrot.com>

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

// Turn this on for debugging:
#define RINGING_DEBUG_FILE 0

#include <ringing/extent.h>
#include <ringing/falseness.h>
#include <ringing/group.h>
#include <ringing/iteratorutils.h>
#include <ringing/litelib.h>
#include <ringing/pointers.h>
#include <ringing/multtab.h>
#include <ringing/mathutils.h>
#include <ringing/streamutils.h>

#include "args.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <fstream>
#include <set>
#include <utility>
#include <vector>

#include <cassert>

#if RINGING_DEBUG_FILE
#define DEBUG( expr ) (void)((cout << expr) << endl)
#else
#define DEBUG( expr ) (void)(false)
#endif

RINGING_USING_NAMESPACE

class method_list {
private:
  typedef sqmulttab::row_t row_t;

  struct methinfo {
    method meth;
    row_t le;
  };

public:
  method_list( library const& lib, sqmulttab const& mt );

  int bells() const { return b; }

  typedef vector<methinfo>::const_iterator const_iterator;
  const_iterator begin() const { return data.begin(); }
  const_iterator end() const { return data.end(); }

  const_iterator find( method const& m ) const;

private:
  int b;
  vector<methinfo> data;
};

method_list::method_list( library const& lib, sqmulttab const& mt )
  : b( mt.bells() )
{
  for ( library::const_iterator i(lib.begin()), e(lib.end()); i!=e; ++i ) {
    method m( i->meth() ); 
    m.name( i->get_facet<litelib::payload>() ); 
    methinfo mi = { m, mt.find( m.lh() * m.back() ) };
    data.push_back( mi );
  }
}

method_list::const_iterator method_list::find( method const& m ) const
{
  for ( const_iterator i=begin(), e=end(); i != e; ++i )
    if ( i->meth == m ) 
      return i;
  throw runtime_error("No such method");
}

class spliced_plan {
private:
  typedef sqmulttab::row_t row_t;
  typedef method_list::const_iterator method_ptr;

public:
  spliced_plan( sqmulttab const& mt ) : mt(&mt) {}

  typedef map<row_t, method_ptr, row_t::cmp>::const_iterator const_iterator;

  const_iterator begin() const { return data.begin(); }
  const_iterator end() const { return data.end(); }
  size_t size() const { return data.size(); }

  void set_method( row_t const& r, method_ptr const& m );
  void unset_method( row_t const& r );

  spliced_plan make_rotation( row_t const& r ) const;

  friend bool operator==( const spliced_plan& x, const spliced_plan& y )
    { return x.data == y.data; }
  friend bool operator!=( const spliced_plan& x, const spliced_plan& y )
    { return x.data != y.data; }

  friend bool operator<( const spliced_plan& x, const spliced_plan& y );
  friend bool operator>( const spliced_plan& x, const spliced_plan& y )
    { return y < x; }
  friend bool operator<=( const spliced_plan& x, const spliced_plan& y )
    { return !(y < x); }
  friend bool operator>=( const spliced_plan& x, const spliced_plan& y )
    { return !(x < y); }

  void write_plan( ostream& os ) const;
  void read_plan( method_list const& ml, istream& in );
  void set_plan_name( string const& n ) { plan_n = n; }

  string const& name() const { return plan_n; }

private:
  sqmulttab const* mt;
  map<row_t, method_ptr, row_t::cmp> data;
  string plan_n;
};

template <class Container>
inline void assert_erase(Container& c, typename Container::key_type const& k)
{
  typename Container::iterator i = c.find(k); 
  assert( i != c.end() );
  c.erase(i); 
}

void spliced_plan::set_method( row_t const& r, method_ptr const& m )
{
  assert( data.find(r) == data.end() );
  data[r] = m;
}

void spliced_plan::unset_method( row_t const& r )
{
  assert_erase( data, r );
}

spliced_plan spliced_plan::make_rotation( row_t const& r ) const
{
  spliced_plan rotn( *mt );

  for ( const_iterator ci = begin(), ce = end(); ci != ce; ++ci ) 
    rotn.data[ r * ci->first ] = ci->second;

  rotn.plan_n = plan_n; 
  return rotn;
}

template <class Cmp1, class Cmp2>
struct pair_cmp {
  typedef pair< typename Cmp1::first_argument_type,
                typename Cmp2::first_argument_type > first_argument_type;
  typedef pair< typename Cmp1::second_argument_type,
                typename Cmp2::second_argument_type > second_argument_type;
  typedef bool result_type;

  pair_cmp() {}
  pair_cmp( Cmp1 const& cmp1, Cmp2 const& cmp2 ) : cmp1(cmp1), cmp2(cmp2) {}

  bool operator()( first_argument_type const& x, 
                   second_argument_type const& y ) const {
    if (cmp1(x.first, y.first)) return true;
    else if (cmp1(y.first, x.first)) return false;
    else return (cmp2(x.second, y.second));
  }

private:
  Cmp1 cmp1; 
  Cmp2 cmp2;
};

// Matches fried decl. in class
bool operator<( const spliced_plan& x, const spliced_plan& y )
{
  return lexicographical_compare( 
    x.begin(), x.end(), y.begin(), y.end(),
    pair_cmp< sqmulttab::row_t::cmp, 
              less<method_list::const_iterator> >() );
}

void spliced_plan::write_plan( ostream& os ) const
{
  for ( const_iterator ci = begin(), ce = end(); ci != ce; ++ci ) 
    os << mt->find(ci->first) << "\t" 
       << ci->second->meth.format(method::M_DASH|method::M_SYMMETRY) << "\t"
       << ci->second->meth.name() << "\n";
}

void spliced_plan::read_plan( method_list const& ml, istream& in )
{
  while (in) {
    string line; getline(in, line);
    if (line.empty() || line[0] == '#') continue;

    size_t i = line.find('\t');
    if (i == string::npos) throw runtime_error("Unable to read plan");
    row lh( line.substr(0,i) );

    size_t j = line.find('\t', ++i);
    if (j == string::npos) throw runtime_error("Unable to read plan");
    method m( line.substr(i, j-i), ml.bells(), line.substr(j+1) );

    set_method( mt->find(lh), ml.find(m) );
  }
}

struct arguments 
{
  init_val<int,0>      bells;

  init_val<bool,false> in_course; 
  init_val<bool,false> prune_rotations;

  init_val<int,0>      verbosity;
  init_val<bool,false> quiet;

  vector<string>       pend_strs;
  group                pends;

  string               write_plan;

  arguments( int argc, char const* argv[] );

private:
  void bind( arg_parser& p );
  bool validate( arg_parser& p );
  bool generate_pends( arg_parser& ap );
};

arguments::arguments( int argc, char const *argv[] )
{
  arg_parser ap( argv[0], "musgrep -- grep rows for music", "OPTIONS" );
  bind( ap );

  if ( !ap.parse(argc, argv) )
    {
      ap.usage();
      exit(1);
    }
  
  if ( !validate(ap) )
    exit(1);
}

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );
           
  p.add( new integer_opt
         ( 'b', "bells",
           "The number of bells.  This option is required", "BELLS",
           bells ) );

  p.add( new boolean_opt
         ( 'i', "in-course",
           "Only display splices for in-course lead heads", 
           in_course ) );

  p.add( new boolean_opt
         ( 'r', "ignore-rotations",
           "Filter out touches only differing by a rotation",
           prune_rotations ) );

  p.add( new strings_opt
         ( 'P', "part-end",
           "Specify a part end",  "ROW",
           pend_strs ) );

  p.add( new boolean_opt
         ( 'q', "quiet",
           "Suppress logging of plans as they're found",
           quiet ) );

  p.add( new repeated_boolean_opt
         ( 'v', "verbose",
           "Be more verbose (use multiple times for increased verbosity)",
           verbosity ) );

  p.add( new string_opt
         ( 'O', "output-plans",
           "Write plans out to directory or file (with % for the plan number)",
           "FILE", write_plan ) );
}

bool arguments::validate( arg_parser& ap )
{               
  if ( bells <= 0 )
    {
      ap.error( "Then number of bells must be positive" );
      return false;
    }

  if ( bells > int(bell::MAX_BELLS) )
    {
      ap.error( make_string() << "The number of bells must be less than "
                << bell::MAX_BELLS+1 );
      return false;
    }

  if ( !generate_pends( ap ) )
    return false;

  if ( prune_rotations && pends.size() > 1 ) 
    {
      ap.error( "Part-ends cannot be specified while ignoring rotations" );
      return false;
    }

  return true;
}

bool arguments::generate_pends( arg_parser& ap )
{
  vector<row> gens;
  gens.reserve( pend_strs.size() );

  for ( vector<string>::const_iterator
          i( pend_strs.begin() ), e( pend_strs.end() ); i != e; ++i )
    {
      row const g( row(bells) * row(*i) );
      gens.push_back(g);
    }

  if ( gens.size() )
    pends = group( gens );
  else
    pends = group( row(bells) );

  // TODO: Warning if partends generate the extent? 
  // Or anything equally silly?

  return true;
}


class searcher {
public:
  searcher( arguments const& args );

  void recurse();

private:
  typedef sqmulttab::row_t row_t;

  typedef method_list::const_iterator method_ptr;
  typedef pair<method_ptr, method_ptr> method_pair;
  typedef vector<row_t> falseness_tab;
  //typedef map<row_t, method_ptr, row_t::cmp> composition;
  typedef multimap<row_t, method_ptr, row_t::cmp> pos_map;

  sqmulttab* make_multtab();
  void init_pends();
  void init_falseness();
  void init_search();
  void select_possibles( row_t& lh, vector<method_ptr>& meths ) const;
  bool is_possible( row_t const& lh, method_ptr const& m ) const;
  void found( unsigned rotn_count );
  void set_method( row_t const& lh, method_ptr const& m );
  void unset_method( row_t const& lh, method_ptr const& m );
  void recheck_pos_map( row_t const& lh, method_ptr const& m,
                        vector<pair<row_t, method_ptr> >& backtrack );
  bool are_false( row_t const& lh1, method_ptr const& m1, 
                  row_t const& lh2, method_ptr const& m2 ) const;
  void done_with_method( method_ptr const& m );
  bool is_rotational_standard_form( unsigned& rotn_count ) const;

  arguments const& args;

  RINGING_ULLONG node_count;

  scoped_pointer<sqmulttab> mt;

  method_list meths; 
  map<method_pair, falseness_tab> false_data;
  vector<row_t> pends;

  set<row_t, row_t::cmp> free_lhs;

  spliced_plan comp;
  pos_map possibles;
};

searcher::searcher( arguments const& args )
  : args(args),
    node_count(0), mt(make_multtab()), 
    meths(litelib(args.bells, cin), *mt), comp(*mt)
{
  init_pends();
  init_falseness();
  init_search();
}

sqmulttab* searcher::make_multtab()
{
  if (args.in_course)
    return new sqmulttab( incourse_extent_iterator(args.bells-1, 1), 
                          incourse_extent_iterator() );
  else
    return new sqmulttab( extent_iterator(args.bells-1, 1), 
                          extent_iterator() );
}

void searcher::init_pends()
{
  pends.reserve( args.pends.size() );
  for ( group::const_iterator i=args.pends.begin(), e=args.pends.end(); 
        i != e; ++i )
    pends.push_back( mt->find(*i) );
}

void searcher::init_falseness()  
{
  int ftflags = 0 | (args.in_course ? falseness_table::in_course_only : 0);
  for ( method_ptr i=meths.begin(), e=meths.end(); i != e; ++i )
  for ( method_ptr j=meths.begin()               ; j != e; ++j ) {
    falseness_table ft(i->meth, j->meth, ftflags);
    vector<row_t> ft2;
    for ( falseness_table::const_iterator fi=ft.begin(), fe=ft.end();
          fi != fe; ++fi )
      ft2.push_back( mt->find(*fi) );
    false_data[ make_pair(i,j) ] = ft2;
  }
}

bool searcher::is_possible( row_t const& lh, 
                            searcher::method_ptr const& m ) const
{
  row_t le = lh * m->le;

  // Is the le you would get by ringing a lead of the method free?
  if ( le != lh && free_lhs.find(le) == free_lhs.end() )
    return false;

  // Is the lead true agaisnt all the methods currently chosen?
  for ( spliced_plan::const_iterator ci = comp.begin(), ce = comp.end(); 
        ci != ce; ++ci ) 
  {
    if ( are_false( lh, m, ci->first, ci->second ) )
      return false;
  }

  return true; 
}

void searcher::init_search()
{
  if (args.in_course)
    for ( incourse_extent_iterator i(args.bells-1, 1), e; i != e; ++i )
      free_lhs.insert( mt->find(*i) );
  else
    for ( extent_iterator i(args.bells-1, 1), e; i != e; ++i )
      free_lhs.insert( mt->find(*i) );

  DEBUG( "Have " << free_lhs.size() << " lead heads and ends" );

  for ( set<row_t>::const_iterator 
          li = free_lhs.begin(), le = free_lhs.end(); li != le; ++li ) 
    for ( method_ptr mi=meths.begin(), me=meths.end(); mi != me; ++mi ) 
      if ( is_possible( *li, mi ) )
        possibles.insert( make_pair( *li, mi ) );
}

bool searcher::are_false( row_t const& lh1, method_ptr const& m1, 
                          row_t const& lh2, method_ptr const& m2 ) const
{
  map<method_pair, falseness_tab>::const_iterator
    fd = false_data.find( make_pair( m2, m1 ) );
  assert( fd != false_data.end() );

  // We want to check each part against each other part, i.e.
  //   ( p2 * lh2 * f == p1 * lh1 )
  // for each p1, p2 in the part end group.  However, we can pre-multiply
  // both sides by p2.inverse() as the part ends form a group (which is 
  // therefore closed under multiplication and inverse), we can just 
  // iterate once over the group.
 
  for ( falseness_tab::const_iterator 
          fi=fd->second.begin(), fe=fd->second.end(); fi!=fe; ++fi ) {
    row_t f = *fi;
    for ( vector<row_t>::const_iterator
            pi=pends.begin(), pe=pends.end(); pi!=pe; ++pi )
      if ( lh2 * f == *pi * lh1 )
        return true;
  }

  return false;
}

void searcher::recheck_pos_map( row_t const& lh, method_ptr const& m,
                                vector<pair<row_t,method_ptr> >& backtrack )
{
  row_t const le = lh * m->le;

  for ( pos_map::iterator j=possibles.begin(), e=possibles.end(); j != e; ) 
  {
    pos_map::iterator i = j++;  // This allows us to erase i

    if ( i->first * i->second->le == le ) {
      backtrack.push_back(*i);
      possibles.erase(i);
      continue;
    }
  
    if ( are_false( lh, m, i->first, i->second ) ) { 
      backtrack.push_back(*i);
      possibles.erase(i);
    }
  }
}

bool searcher::is_rotational_standard_form( unsigned& rotn_count ) const
{
  unsigned rotn_eq_count = 0u;

  for ( spliced_plan::const_iterator ci = comp.begin(), ce = comp.end(); 
        ci != ce; ++ci ) {
    // XXX:  This assumes that the set of lhs and les form a group
    // really we want inverse( ci->first ), but that's tedious to evaluate. 
    // If we start looking at touches with singles, this will need changing.
    spliced_plan rotn( comp.make_rotation( ci->first ) );
    if ( rotn <  comp ) return false;
    if ( rotn == comp ) ++rotn_eq_count;
  }

  assert( comp.size() % rotn_eq_count == 0 );
  rotn_count = comp.size() / rotn_eq_count;

  return true;
}

void searcher::done_with_method( method_ptr const& m ) 
{
  pos_map::iterator i = possibles.begin(), e = possibles.end();
  while ( i != e ) {
    pos_map::iterator j = i;  ++i;  // We can now call erase(j) safely
    if ( j->second == m ) possibles.erase(j);
  }
}

void searcher::select_possibles( searcher::row_t& lh,
                                 vector<method_ptr>& try_meths ) const
{
  // First, lets choose which lead head to look at.  Our strategy is to
  // choose the l.h. with the fewest possible methods available.
  pos_map::const_iterator i = possibles.begin(), e = possibles.end();

  pair< pos_map::const_iterator, pos_map::const_iterator > best(i, i);
  unsigned best_sz = unsigned(-1);  

  while ( i != e ) {
    unsigned sz = 0;
    pos_map::const_iterator j = i;
    for ( ; j != e; ++j, ++sz )
      if ( j->first != i->first )
        break;
    if ( sz < best_sz ) {
      best_sz = sz;
      best = make_pair( i, j );
    }
    i = j;
  }

  // BEST is the range in POSSIBLES where the l.h. has the fewest possible
  // methods, and BEST_SZ is the number of methods.

  if ( best.first == best.second ) {
    try_meths.clear();
  } 
  else {
    lh = best.first->first;
    for ( ; best.first != best.second; ++best.first )
      try_meths.push_back( best.first->second );
  }
  const int depth = comp.size() / 2;
}

void searcher::found( unsigned rotn_count )
{
  static int plan_n = 0;
  ++plan_n;

  if ( args.write_plan.size() )
  {
    string filename;
    { size_t i = args.write_plan.find('%');
      if ( i == string::npos ) 
        filename = ( make_string() << args.write_plan 
                                   << "/" << plan_n << ".plan" );
      else
        filename = ( make_string() << args.write_plan.substr(0,i)
                                   << plan_n << args.write_plan.substr(i+1) );
    }

    ofstream os( filename.c_str() );
    comp.write_plan(os);
    os.close();
  }

  map< method_ptr, unsigned > counts;

  for ( spliced_plan::const_iterator ci = comp.begin(), ce = comp.end();
          ci != ce; ++ci ) {
    counts[ ci->second ]++;
  }

  if (!args.quiet) {
    bool comma = false;
    for ( map<method_ptr, unsigned >::iterator 
            i = counts.begin(), e = counts.end(); i !=e; ++i ) {
      if (comma) cout << ", ";
      cout << i->first->meth.name() << " (" << i->second << ")";
      comma = true;
    }
    if ( args.prune_rotations )
      cout << " [" << rotn_count << " rotations]";
    cout << endl;
  }
}

void searcher::set_method( row_t const& lh1, method_ptr const& m )
{
  bool palindrome = false;
  row_t const le1 = lh1 * m->le;

  for ( vector<row_t>::const_iterator
          pi=pends.begin(), pe=pends.end(); pi!=pe; ++pi ) {
    if ( !pi->isrounds() && *pi * lh1 == le1 )
      palindrome = true;
  }

  DEBUG( "Set " << palindrome );
  for ( vector<row_t>::const_iterator
          pi=pends.begin(), pe=pends.end(); pi!=pe; ++pi )
  {
    row_t const lh = *pi * lh1;
    DEBUG( "Setting " << m->meth.name() << " at " << mt->find(lh) );
    comp.set_method( lh, m );
    assert_erase( free_lhs, lh );
  
    if ( !palindrome ) {
      row_t const le = *pi * le1;
      DEBUG( " ... and " << m->meth.name() << " at " << mt->find(le) );
      comp.set_method( le, m );
      assert_erase( free_lhs, le );
    }
  }
}

void searcher::unset_method( row_t const& lh1, method_ptr const& m )
{
  bool palindrome = false;
  row_t const le1 = lh1 * m->le;

  for ( vector<row_t>::const_iterator
          pi=pends.begin(), pe=pends.end(); pi!=pe; ++pi ) {
    if ( !pi->isrounds() && *pi * lh1 == le1 )
      palindrome = true;
  }

  DEBUG( "Unset " << palindrome );
  for ( vector<row_t>::const_iterator
          pi=pends.begin(), pe=pends.end(); pi!=pe; ++pi )
  {
    row_t const lh = *pi * lh1;
    DEBUG( "Unsetting " << m->meth.name() << " at " << mt->find(lh) );
    free_lhs.insert(lh);
    comp.unset_method( lh );
  
    if ( !palindrome ) {
      row_t const le = *pi * le1;
      DEBUG( " ... and " << m->meth.name() << " at " << mt->find(le) );
      free_lhs.insert(le);
      comp.unset_method( le );
    }
  }
}

void searcher::recurse()
{
  row_t lh;
  vector<method_ptr> try_meths;
  select_possibles( lh, try_meths );

  possibles.erase(lh);

  const int depth = comp.size() / 2;

  for ( vector<method_ptr>::const_iterator 
          i = try_meths.begin(), e = try_meths.end(); i != e; ++i )
  {
    if (args.verbosity && depth == 0)
      cerr << "Trying start method: " << (*i)->meth.name() << endl;

    if (depth && depth < args.verbosity)
      cerr << string(depth, ' ') << string(depth, ' ') << mt->find(lh) 
           << " -> " << (*i)->meth.name() << endl;

    vector<pair<row_t, method_ptr> > undo;
    set_method(lh, *i);
    recheck_pos_map(lh, *i, undo);
    if ( free_lhs.empty() ) {
      unsigned rotn_count = 0;
      bool canonical_rotn = is_rotational_standard_form(rotn_count);
      if ( !args.prune_rotations || canonical_rotn ) 
        found( rotn_count );  
    }
    else recurse();
    unset_method(lh, *i);
    copy( undo.begin(), undo.end(), inserter(possibles, possibles.begin()) );

    // Poor man's rotational pruning
    if (depth == 0 && args.prune_rotations) 
      done_with_method(*i);
  }

  for ( vector<method_ptr>::const_iterator 
          i = try_meths.begin(), e = try_meths.end(); i != e; ++i )
    possibles.insert( make_pair(lh, *i) );

  ++node_count;
  if (depth == 0 && args.verbosity) 
    cerr << "Searched " << node_count << " nodes\n";
}

class analyser
{
private:
  typedef sqmulttab::row_t row_t;

public:
  analyser( arguments const& args );

  void add_plan( string const& filename );
 
private:
  sqmulttab* make_multtab();
  bool compare( spliced_plan const& x, spliced_plan const& y ) const;
  void compare_all_rotations( spliced_plan const& x, 
                              spliced_plan const& y ) const;

  arguments const& args;

  scoped_pointer<sqmulttab> mt;

  method_list meths; 

  list<spliced_plan> plans;
};

analyser::analyser( arguments const& args )
  : args(args),
    mt(make_multtab()), meths(litelib(args.bells, cin), *mt)
{
}

void analyser::add_plan( string const& filename )
{
  {
    ifstream in( filename.c_str() );
    if (!in) throw runtime_error("Unable to load plan");
    spliced_plan plan(*mt);  
    plan.read_plan(meths, in);
    in.close();
    
    string name( filename); 
    if (name.find(".plan") == name.size()-5) 
      name = name.substr(0,name.size()-5);
    plan.set_plan_name(name);

    cout << name << ";" << endl;

    plans.push_back(plan);
  }

  if (args.verbosity)
    cerr << "Adding plan #" << plans.size() << endl;


  for ( list<spliced_plan>::const_iterator 
          i = plans.begin(), e = prior( plans.end() ); i != e; ++i )
    if (args.prune_rotations) 
      compare_all_rotations( *i, *e );
    else
      compare( *i, *e );
}

bool analyser::compare( spliced_plan const& x, spliced_plan const& y ) const
{
  typedef method_list::const_iterator method_ptr;

  bool set_meths = false;
  method_ptr xm, ym;
  set<row_t, row_t::cmp> rows;

  for ( spliced_plan::const_iterator 
          xi = x.begin(), xe = x.end(), yi = y.begin(), ye = y.end();
          xi != xe && yi != ye;  ++xi, ++yi )
  {
    // XXX: This will need fixing if we do singles
    if ( xi->first != yi->first ) 
      throw runtime_error( "Plans have incompatbile lead heads" );

    if ( xi->second != yi->second ) {
      if (set_meths && ( xi->second != xm || yi->second != ym ) )
        return false; // The difference is not a simple splice
      if (!set_meths) { xm = xi->second; ym = yi->second; set_meths = true; }
      rows.insert(xi->first);
    }
  }

  // We don't want to link single method plans together.
  if (rows.size() == x.size()) return true;

  cout << x.name() << " -- " << y.name() 
       << " [ label=\"" << (rows.size()/2) << " leads "
       << xm->meth.name() << "-" << ym->meth.name() << "\" ];" 
       << endl;
  return true;
}

void analyser::compare_all_rotations( spliced_plan const& x, 
                                      spliced_plan const& y ) const
{
  for ( spliced_plan::const_iterator ci = x.begin(), ce = x.end(); 
        ci != ce; ++ci ) {
    // XXX:  This assumes that the set of lhs and les form a group
    // really we want inverse( ci->first ), but that's tedious to evaluate. 
    // If we start looking at touches with singles, this will need changing.
    if ( compare( x.make_rotation(ci->first), y ) )
      return;
  }
}

sqmulttab* analyser::make_multtab()
{
  if (args.in_course)
    return new sqmulttab( incourse_extent_iterator(args.bells-1, 1), 
                          incourse_extent_iterator() );
  else
    return new sqmulttab( extent_iterator(args.bells-1, 1), 
                          extent_iterator() );
}

int main(int argc, char const* argv[] )
{
  bell::set_symbols_from_env();


  arguments args( argc, argv );

  const bool search = true;

  if (search)
    searcher(args).recurse();
  else {
    analyser a(args);
    for ( int i=1; i<argc; ++i )
      a.add_plan( argv[i] );
  } 
}
