// -*- C++ -*- music.cpp - things to analyse music
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
#include "music.h"
#if RINGING_OLD_INCLUDES
#include <vector.h>
#else
#include <vector>
#endif
#include <ringing/row.h>
#include <ringing/method.h>
#include <ringing/music.h>
#include <ringing/streamutils.h>


RINGING_USING_NAMESPACE
RINGING_USING_STD


class musical_analysis::patterns
{
private:
  patterns() {}
  
public:
 ~patterns() {}
  
  static patterns &instance() 
  { static patterns tmp; return tmp; }

  friend class analyser;
  
  // Data members
  vector<string> p;
};

class musical_analysis::analyser
{
private:
  analyser( int bells );

public:
  // This is public to get it to compile in MSVC.
 ~analyser() {}
    
  static analyser &instance( int bells ) 
  { static analyser tmp( bells ); return tmp; }

  void init_crus();
  void init_n_runs( int n );

  friend class patterns;
    
  // Data members
  int bells;
  music mu;
};

void musical_analysis::analyser::init_n_runs( int n )
{
  if ( n > bells )
    {
      cerr << "Cannot have runs longer than the total number of bells\n";
      exit(1);
    }
  else if ( n < 3 )
    {
      cerr << "Can only search for runs of three or more bells\n";
      exit(1);
    }

  {
    for ( int i=0; i<=bells-n; ++i )
      {
	make_string os;
	os << '*';
	for ( int j=0; j<n; ++j ) os << bell( i+j );
	os << '*';
	mu.push_back(music_details(os));
      }
  }
  {
    for ( int i=0; i<=bells-n; ++i )
      {
	make_string os;
	os << '*';
	for ( int j=n-1; j>=0; --j ) os << bell( i+j );
	os << '*';
	mu.push_back(music_details(os));
      }
  }
}

void musical_analysis::analyser::init_crus()
{
  for ( int i = 3; i < 6; ++i ) 
    for ( int j = 3; j < 6; ++j )
      {
	if ( i == j ) 
	  continue;
	
	make_string os;
	os << '*' << bell(i) << bell(j) ;
	for ( int k = 6; k < bells; ++k )
	  os << bell(k);
	
	mu.push_back(music_details(os));
      }
}

musical_analysis::analyser::analyser( int bells )
  : bells(bells), mu(bells)
{
  const vector<string> &p = patterns::instance().p;

  if ( p.empty() )
    init_crus();   // By default we count the CRUs in the plain course.
  else
    {
      for ( vector<string>::const_iterator i( p.begin() ), e( p.end() );
	    i != e; ++i )
	{
	  if ( i->size() > 2 && (*i)[0] == '<' && (*i)[ i->size()-1 ] == '>' )
	    {
	      string n( i->substr( 1, i->size()-2 ) );

	      if ( n == "queens" )
		mu.push_back
		  ( music_details( row::queens( bells ).print() ) );

	      else if ( n == "kings" )
		mu.push_back
		  ( music_details( row::kings( bells ).print() ) );

	      else if ( n == "tittums" )
		mu.push_back
		  ( music_details( row::tittums( bells ).print() ) );


	      else if ( n == "reverse-rounds" )
		mu.push_back
		  ( music_details( row::reverse_rounds( bells ).print() ) );

	      else if ( n == "CRUs" )
		init_crus();
	      
	      else if ( n.length() > 5 && n.find("-runs") != string::npos )
		{
		  char *endp;
		  int l = strtol( n.c_str(), &endp, 10 );
		  if ( strcmp(endp, "-runs") )
		    {
		      cerr << "Unknown type of run: " << n << endl;
		      exit(1);
		    }

		  init_n_runs( l );
		}

	      else 
		{
		  cerr << "Unknown named music type: " << n << endl;
		  exit(1);
		}
	    }
	  else
	    {
	      mu.push_back( music_details( *i ) );
	    }
	}
    }
}

void musical_analysis::add_pattern( const string &str )
{
  patterns::instance().p.push_back( str );
}

int musical_analysis::analyse( const method &m )
{
 vector< row > course;
  row r( m.bells() );

  do for ( method::const_iterator i( m.begin() ), e( m.end() ); 
	   i != e; ++i )
    {
      r *= *i;
      course.push_back( r );
    }
  while ( !r.isrounds() );

  music &mu = analyser::instance( m.bells() ).mu;

  mu.process_rows( course.begin(), course.end() );  
  return mu.get_score();
}

void musical_analysis::force_init( int bells )
{
  // Going into this function initialises the static there.
  analyser::instance( bells );
}
