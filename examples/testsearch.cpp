// -*- C++ -*- testsearch.cpp - test the search classes
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

#include <iostream>
#include <vector>
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/touch.h>
#include <ringing/table_search.h>

#if RINGING_USE_NAMESPACES
using namespace ringing;
#endif

// NB.  Once ostream &operator<<( ostream &, const touch & ) has
// been defined, there will be no need for this.

// A function object to attempt to sanely print the touch
class print_touch
{
public:
  typedef touch argument_type;
  typedef void result_type;

  print_touch() : bells( 6 ), leadlen( 12 ) {}

  void operator()( const touch &t ) const
  {
    row r;
    int n=0, l=0;
    for ( touch::const_iterator i( t.begin() ); i != t.end(); ++i )
      {
	if ( ++n % leadlen == 0 )
	  cout << ( *i == change( bells, "12" ) ? '.' : 'B' );
      }
    cout << "  (" << n << " changes)" << endl;
  }
private:
  size_t bells, leadlen;
};

template < class Function >
class iter_from_fun_t 
{
public:
  // Standard iterator typedefs
  typedef void value_type;
  typedef void reference;
  typedef void pointer;
  typedef void differnce_type;  
  typedef output_iterator_tag iterator_category;

  // Output Iterator requirements
  iter_from_fun_t &operator*() { return *this; }
  iter_from_fun_t &operator++() { return *this; }  
  iter_from_fun_t &operator++(int) { return *this; }
  iter_from_fun_t &operator=( typename Function::argument_type arg ) 
  { func( arg ); return *this; }

  // Construction
  iter_from_fun_t( const Function &func ) : func( func ) {}

private:
  // Data members
  Function func;
};

// Construction helper
template < class Function >
iter_from_fun_t< Function > iter_from_fun( const Function &func )
{
  return iter_from_fun_t< Function >( func );
}



int main()
{
  method m( "&-16-16-16,12", 6 );

  vector<change> calls;
  calls.push_back( change( m.bells(), "14" ) );

  cout << "An exhaustive list of bobs-only 360s of Bob Minor "
          "(modulo rotation)" << endl;
  touch_search( table_search( m, calls, make_pair( size_t(30), size_t(30) ), 
			      true ),
		iter_from_fun( print_touch() ) );

  return 0;
}
