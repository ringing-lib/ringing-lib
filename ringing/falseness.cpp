// -*- C++ -*- falseness.cpp - Class for falseness table
// Copyright (C) 2001, 2002, 2003, 2005, 2008 
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

#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/falseness.h>
#include <ringing/streamutils.h>
#include <ringing/pointers.h>
#include <ringing/group.h>
#if RINGING_OLD_INCLUDES
#include <algo.h>
#include <iterator.h>
#include <set.h>
#include <map.h>
#else
#include <algorithm>
#include <iterator>
#include <set>
#include <map>
#endif
#if RINGING_OLD_C_INCLUDES
#include <assert.h>
#else
#include <cassert>
#endif

#include <iostream>
RINGING_START_NAMESPACE

RINGING_USING_STD

static bool are_tenors_together( const row &r, int working_bells )
{
  for ( int i=working_bells; i<r.bells(); ++i )
    if ( r[i] != i )
      return false;

  return true;
}

falseness_table::falseness_table()
  : t(1, row())
{}

falseness_table::falseness_table( const method &m, int flags )
  : flags(flags), lh(m.lh())
{
  // The falseness table is calculated using
  //   F = \{ a b^{-1} : a, b \in M \}
  // where M is the set of rows in the first lead of the method. 

  set<row> fs;

  method::const_iterator const end
    ( flags & half_lead_only ?  m.begin() + m.size() / 2 : m.end() );

  row r1( m.bells() );
  for ( method::const_iterator i1( m.begin() ); i1 != end; r1 *= *i1++ )
    {
      row r2( m.bells() );
      for ( method::const_iterator i2( m.begin() ); i2 != end; r2 *= *i2++)
	{
	  row f = r1 / r2;

	  if ( !( flags & no_fixed_treble ) && f[0] != 0 )
	    continue;

	  if ( ( flags & in_course_only ) && f.sign() == -1 )
	    continue;

	  fs.insert( f );
	}
    }
  
  // Put the falsenesses into the vector 
  t.reserve( fs.size() );
  copy( fs.begin(), fs.end(), back_inserter(t) );
}


false_courses::false_courses()
  : t(1, row())
{}


class false_courses::initialiser
{
public:
  initialiser( false_courses &fc ) : fc( fc ) {}

  void process( const row &f )
  {
    if ( f[0] != 0 ) return;

    // For efficiency.  We know that if the first lead-head is even
    // and the false lead-head is odd there can be no even FCHs.
    if ( ( fc.flags & in_course_only ) 
	 && fc.lh.sign() == +1 && f.sign() == -1 )
      return;

    row lead; 
    do 
      {
	row c( lead * f );
	lead *= fc.lh;
	      
	// Transpose to the course head
	while ( c[ f.bells() - 1 ] != f.bells() - 1 ) 
	  c *= fc.lh;
	      
	if ( ( fc.flags & in_course_only ) && c.sign() == -1 )
	  continue;

	if ( ( fc.flags & tenors_together ) 
	     && !are_tenors_together( c, 6 ) )
	  continue;

	fs.insert( c );
      }
    while ( !lead.isrounds() );
  }

  void extract() 
  {
    // Put the falsenesses into the vector 
    fc.t.reserve( fs.size() );
    copy( fs.begin(), fs.end(), back_inserter( fc.t ) );
  }

private:
  false_courses &fc;
  set<row> fs;
};

false_courses::false_courses( const method &m, int flags )
  : flags(flags), lh(m.lh())
{
  initialiser init( *this );

  method::const_iterator const e( m.end() );
  row r1( m.bells() );
  for ( method::const_iterator i1( m.begin() ); i1 != e; r1 *= *i1++ )
    {
      row r2( m.bells() );
      for ( method::const_iterator i2( m.begin() ); i2 != e; r2 *= *i2++)
	{
	  init.process( r1 / r2 );
	}
    }

  init.extract();
}

RINGING_START_ANON_NAMESPACE

static const struct init_data_t {
  const char *r, *s;
} init_data[] = {
#include "falseness.dat"
  { NULL, NULL }
};


// An optional cache for efficiency
static shared_pointer< map<row, string> > optimised_table;

static shared_pointer< map<row, string> > make_table(int bells)
{
  if ( optimised_table && optimised_table->size() && 
       optimised_table->begin()->first.bells() == bells )
    return optimised_table;

  shared_pointer< map<row, string> > table( new map<row,string> );

  const row rounds(bells);
  for ( const init_data_t* p=init_data; p->r; ++p )
    {
      if ( bells == 8 )
	(*table)[ rounds * p->r ] = string( p->s, 1 );
      else if ( bells == 10 && strcmp( p->s, "D1" ) == 0 )
	(*table)[ rounds * p->r ] = "B1";
      else
	(*table)[ rounds * p->r ] = p->s;
    }

  return table;
}

string lookup_one_symbol( map<row, string> const& table, int b, row r )
{
  if ( are_tenors_together( r, 6 ) ) {
    map<row, string>::const_iterator i = table.find(r);
    if (i == table.end()) return string();
    assert( b != 8 || i->second.size() == 1 );
    return i->second;
  }

  // This is a hack to get X, Y and Z falseness to work on 8 bells
  if (b == 8) {
    row rounds(b);
    if ( r == rounds * "17623548" )
      return string( "X" );
    else if ( r == rounds * "17645328" )
      return string( "Y" );
    else if ( r == rounds * "17632458" )
      return string( "Z" );
  }

  return string();
}

RINGING_END_ANON_NAMESPACE

void false_courses::optimise(int bells)
{
  if ( bells == 0 )
    optimised_table.reset( NULL );
  else
    optimised_table = make_table(bells);
}

string false_courses::symbols() const 
{ 
  const size_t b( lh.bells() );

  if ( !(flags & tenors_together) && b != 8 )
    throw std::logic_error("Split-tenors falseness groups do not have names");
  
  if ( b % 2 )
    throw std::logic_error("Odd-bell falseness groups not handled");

  row conj(b);

  if ( !lh.ispblh() )
    {
      throw std::logic_error
	("Irregular falseness groups are not implemented");
    }
      
  // *NOT* a reference to optimised_table
  shared_pointer< map<row, string> > table = make_table(b);
  
  set<string> syms;

  const row rounds(b);  
  for ( const_iterator i(begin()), e(end()); i<e; ++i ) {
    string sym( lookup_one_symbol( *table, b, *i ) );
    if ( ! sym.empty() )
      syms.insert(sym); 
  } 

  make_string ms;
  copy( syms.begin(), syms.end(), 
	ostream_iterator<string>( ms.out_stream(), "" ) );
  return ms;
}

string false_courses::lookup_symbol( row const& r )
{
  // *NOT* a reference to optimised_table
  shared_pointer< map<row, string> > table = make_table(r.bells());
  const size_t b = r.bells();

  row const pblh( row::pblh(b) );
  row lhc; lhc *= change(b, "12"); // either 12 or 1N are fine

  group g( pblh, lhc );

  for ( group::const_iterator i = g.begin(), e = g.end(); i != e; ++i )
    for ( group::const_iterator j = g.begin(); j != e; ++j )
    {
      row const fch( *i * r * *j );

      string sym( lookup_one_symbol( *table, b, fch ) );
      if ( ! sym.empty() )
        return sym;
      sym = lookup_one_symbol( *table, b, fch.inverse() );
      if ( ! sym.empty() )
        return sym;
    }

  assert( r.bells() != 8 || r[0] != 0 );
  return string();
}

RINGING_END_NAMESPACE
