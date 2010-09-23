#include <ringing/extent.h>
#include <ringing/falseness.h>
#include <ringing/litelib.h>
#include <ringing/pointers.h>
#include <ringing/multtab.h>
#include <ringing/mathutils.h>
#include <ringing/streamutils.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <fstream>
#include <set>
#include <utility>
#include <vector>

#include <cassert>

RINGING_USING_NAMESPACE

class searcher {
public:
  searcher();

  void recurse();

private:
  typedef sqmulttab::row_t row_t;

  struct methinfo {
    method meth;
    row_t le;
  };

  typedef vector<methinfo> method_list;
  typedef method_list::const_iterator method_ptr;
  typedef pair<method_ptr, method_ptr> method_pair;
  typedef vector<row_t> falseness_tab;
  typedef map<row_t, method_ptr, row_t::cmp> composition;
  typedef multimap<row_t, method_ptr, row_t::cmp> pos_map;

  void read_methods();
  void init_multtab();
  void init_falseness();
  void init_search();
  void select_possibles( row_t& lh, vector<method_ptr>& meths ) const;
  bool is_possible( row_t const& lh, method_ptr const& m ) const;
  void found();
  void set_method( row_t& lh, method_ptr const& m );
  void unset_method( row_t& lh, method_ptr const& m );
  void recheck_pos_map( row_t const& lh, method_ptr const& m,
                        vector<pair<row_t, method_ptr> >& backtrack );
  bool are_false( row_t const& lh1, method_ptr const& m1, 
                  row_t const& lh2, method_ptr const& m2 ) const;

   
  const int bells;
  const bool in_course_only;


  method_list meths; 
  map<method_pair, falseness_tab> false_data;

  scoped_pointer<sqmulttab> mt;

  set<row_t, row_t::cmp> free_lhs;

  composition comp;
  pos_map possibles;  
};

searcher::searcher()
  : bells(6), in_course_only(true) 
{
  init_multtab();
  read_methods();
  init_falseness();
  init_search();
}

void searcher::init_multtab()
{
  if (in_course_only) {
    mt.reset( new sqmulttab( incourse_extent_iterator(bells-1, 1), 
                             incourse_extent_iterator() ) );
    assert( mt->size() == factorial(bells-1)/2 );
  }
  else {
    mt.reset( new sqmulttab( extent_iterator(bells-1, 1), 
                             extent_iterator() ) );
    assert( mt->size() == factorial(bells-1) );
  }
}

void searcher::read_methods()
{
  litelib lib( bells, cin );
  for ( library::const_iterator i(lib.begin()), e(lib.end()); i!=e; ++i ) {
    method m( i->meth() ); 
    m.name( i->get_facet<litelib::payload>() ); 
    methinfo mi = { m, mt->compute_post_mult( m.lh() * m.back() ) };
    meths.push_back( mi );
  }
}

void searcher::init_falseness()  
{
  int ftflags = 0 | (in_course_only ? falseness_table::in_course_only : 0);
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
  // Is the le you would get by ringing a lead of the method free?
  if ( free_lhs.find(lh * m->le) == free_lhs.end() )
    return false;

  // Is the lead true agaisnt all the methods currently chosen?
  for ( composition::const_iterator ci = comp.begin(), ce = comp.end(); 
        ci != ce; ++ci ) 
  {
    if ( are_false( lh, m, ci->first, ci->second ) )
      return false;
  }

  return true; 
}

void searcher::init_search()
{
  if (in_course_only)
    for ( incourse_extent_iterator i(bells-1, 1), e; i != e; ++i )
      free_lhs.insert( mt->find(*i) );
  else
    for ( extent_iterator i(bells-1, 1), e; i != e; ++i )
      free_lhs.insert( mt->find(*i) );

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
 
  for ( falseness_tab::const_iterator 
          fi=fd->second.begin(), fe=fd->second.end(); fi!=fe; ++fi )
    if ( lh2 * *fi == lh1 )
      return true;

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

template <class Container>
inline void assert_erase(Container& c, typename Container::key_type const& k)
{
  typename Container::iterator i = c.find(k); 
  assert( i != c.end() );
  c.erase(i); 
}

void searcher::found()
{
  static int plan_n = 0;

  ++plan_n;
  ofstream 
    os( string( make_string() << "plans/" << plan_n << ".plan" ).c_str() );

  map< method_ptr, unsigned > counts;

  for ( composition::const_iterator ci = comp.begin(), ce = comp.end();
          ci != ce; ++ci ) {
    counts[ ci->second ]++;

    os << mt->find(ci->first) << "\t" 
       << ci->second->meth.format(method::M_DASH|method::M_SYMMETRY) << "\t"
       << ci->second->meth.name() << "\n";
  }
  os.close();

  bool comma = false;
  for ( map<method_ptr, unsigned >::iterator 
          i = counts.begin(), e = counts.end(); i !=e; ++i ) {
    if (comma) cout << ", ";
    cout << i->first->meth.name() << " (" << i->second << ")";
    comma = true;
  }
  cout << "\n";
}

void searcher::set_method( row_t& lh, method_ptr const& m )
{
  row_t const le = lh * m->le;

  assert( comp.find(lh) == comp.end() );
  assert( comp.find(le) == comp.end() );
  
  assert_erase( free_lhs, lh );
  assert_erase( free_lhs, le );

  comp[lh] = comp[le] = m;
}

void searcher::unset_method( row_t& lh, method_ptr const& m )
{
  row_t const le = lh * m->le;

  free_lhs.insert(lh);
  free_lhs.insert(le);

  assert_erase( comp, lh ); 
  assert_erase( comp, le );
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
    vector<pair<row_t, method_ptr> > undo;
    set_method(lh, *i);
    recheck_pos_map(lh, *i, undo);
    if ( free_lhs.empty() ) found();  
    else recurse();
    unset_method(lh, *i);
    copy( undo.begin(), undo.end(), inserter(possibles, possibles.begin()) );
  }

  for ( vector<method_ptr>::const_iterator 
          i = try_meths.begin(), e = try_meths.end(); i != e; ++i )
    possibles.insert( make_pair(lh, *i) );
}

int main()
{
  searcher().recurse();
}
