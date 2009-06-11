// -*- C++ -*- music.cpp - things to analyse music
// Copyright (C) 2002, 2003, 2008, 2009 Richard Smith <richard@ex-parrot.com>

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
#pragma implementation "methsearch/music"
#endif

#include "music.h"
#if RINGING_OLD_C_INCLUDES
#include <string.h>
#else
#include <cstring>
#endif
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <map.h>
#else
#include <vector>
#include <map>
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

  void init_crus( music& m, int score );
  void init_n_runs( music& m, int n, int score );
  void init_front_n_runs( music& m, int n, int score );
  void init_back_n_runs( music& m, int n, int score );

  friend class patterns;
    
  void check_n( int n );

  // Data members
  int bells;
  map< row, music > mu;
};

void musical_analysis::analyser::check_n( int n )
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
}


void musical_analysis::analyser
  ::init_front_n_runs( music& m, int n, int score )
{
  check_n(n);

  {
    for ( int i=0; i<=bells-n; ++i )
      {
	make_string os;
	for ( int j=0; j<n; ++j ) os << bell( i+j );
	os << '*';
	m.push_back(music_details(os, score));
      }
  }
  {
    for ( int i=0; i<=bells-n; ++i )
      {
	make_string os;
	for ( int j=n-1; j>=0; --j ) os << bell( i+j );
	os << '*';
	m.push_back(music_details(os, score));
      }
  }
}

void musical_analysis::analyser
  ::init_back_n_runs( music& m, int n, int score )
{
  check_n(n);

  {
    for ( int i=0; i<=bells-n; ++i )
      {
	make_string os;
	os << '*';
	for ( int j=0; j<n; ++j ) os << bell( i+j );
	m.push_back(music_details(os, score));
      }
  }
  {
    for ( int i=0; i<=bells-n; ++i )
      {
	make_string os;
	os << '*';
	for ( int j=n-1; j>=0; --j ) os << bell( i+j );
	m.push_back(music_details(os, score));
      }
  }
}

void musical_analysis::analyser
  ::init_n_runs( music& m, int n, int score )
{
  check_n(n);

  {
    for ( int i=0; i<=bells-n; ++i )
      {
	make_string os;
	os << '*';
	for ( int j=0; j<n; ++j ) os << bell( i+j );
	os << '*';
	m.push_back(music_details(os, score));
      }
  }
  {
    for ( int i=0; i<=bells-n; ++i )
      {
	make_string os;
	os << '*';
	for ( int j=n-1; j>=0; --j ) os << bell( i+j );
	os << '*';
	m.push_back(music_details(os, score));
      }
  }
}

void musical_analysis::analyser::init_crus( music& m, int score )
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
	
	m.push_back(music_details(os, score));
      }
}

musical_analysis::analyser::analyser( int bells )
  : bells(bells)
{
  const vector<string> &p = patterns::instance().p;
  row ch(bells); // Course head -- plain course by default

  for ( vector<string>::const_iterator i( p.begin() ), e( p.end() );
	    i != e; ++i )
    {
      int score( 1 );
      string pattern( *i );

      string::size_type eq = pattern.find('=');
      if ( eq == 6 && pattern.substr(0,6) == "course" ) 
        {
          string chstr = pattern.substr(7);
          // Allow treble to be omitted
          if ( chstr.find( bell(0).to_char() ) == string::npos ) 
            chstr = bell(0).to_char() + chstr;

          try {          
            ch = row(chstr);
          } catch ( exception const& e ) {
            cerr << "Unable to parse music course head: " << e.what() << endl;
            exit(1);
          }

          ch *= row(bells);

          continue;
        }

      if ( pattern.size() > 2 && isdigit( pattern[0] ) )
        {
          for ( unsigned int j = 0; j < pattern.size(); ++j )
            if ( pattern[j] == ':' )
              {
                score = atoi( pattern.c_str() );
                pattern = pattern.substr(j+1);
                break;
              }
            else if ( !isdigit( pattern[j] ) )
              break;
        }

      mu[ch].set_bells(bells);

      if ( pattern.size() > 2 
           && pattern[0] == '<' && pattern[ pattern.size()-1 ] == '>' )
        {
          string n( pattern.substr( 1, pattern.size()-2 ) );

          if ( n == "queens" )
            mu[ch].push_back
              ( music_details( row::queens( bells ).print(), score ) );

          else if ( n == "kings" )
            mu[ch].push_back
              ( music_details( row::kings( bells ).print(), score ) );

          else if ( n == "tittums" )
            mu[ch].push_back
              ( music_details( row::tittums( bells ).print(), score ) );


          else if ( n == "reverse-rounds" )
            mu[ch].push_back
              ( music_details( row::reverse_rounds( bells ).print(), 
                           score ) );

          else if ( n == "CRUs" )
            init_crus( mu[ch], score );
          
          else if ( n.length() > 11 && n.substr(0,6) == "front-" 
                    && n.find("-runs") != string::npos )
            {
              char *endp;
              int l = strtol( n.c_str() + 6, &endp, 10 );
              if ( strcmp(endp, "-runs") )
                {
                  cerr << "Unknown type of front-run: " << n << endl;
                  exit(1);
                }

              init_front_n_runs( mu[ch], l, score );
            }

          else if ( n.length() > 10 && n.substr(0,5) == "back-" 
                    && n.find("-runs") != string::npos )
            {
              char *endp;
              int l = strtol( n.c_str() + 5, &endp, 10 );
              if ( strcmp(endp, "-runs") )
                {
                  cerr << "Unknown type of back-run: " << n << endl;
                  exit(1);
                }

              init_back_n_runs( mu[ch], l, score );
            }

          else if ( n.length() > 5 && n.find("-runs") != string::npos )
            {
              char *endp;
              int l = strtol( n.c_str(), &endp, 10 );
              if ( strcmp(endp, "-runs") )
                {
                  cerr << "Unknown type of run: " << n << endl;
                  exit(1);
                }

              init_n_runs( mu[ch], l, score );
            }

          else 
            {
              cerr << "Unknown named music type: " << n << endl;
              exit(1);
            }
        }
      else
        {
          mu[ch].push_back( music_details( pattern, score ) );
        }
    }

  // By default we count the CRUs in the plain course.
  if ( mu.empty() ) 
    {
      mu[ch].set_bells(bells);
      init_crus( mu[ch], 1 ); 
    }
}

void musical_analysis::add_pattern( const string &str )
{
  patterns::instance().p.push_back( str );
}

int musical_analysis::analyse( const method &m )
{
  int score = 0;

  map<row, music>& mu = analyser::instance( m.bells() ).mu;
  for ( map<row,music>::iterator mi=mu.begin(), me=mu.end(); mi!=me; ++mi) 
    {
      vector< row > course;
      row r = mi->first; // Set to the course head

      do for ( method::const_iterator i( m.begin() ), e( m.end() ); 
               i != e; ++i )
        {
          r *= *i;
          course.push_back( r );
        }
      while ( r != mi->first );

      mi->second.process_rows( course.begin(), course.end() );  
      score += mi->second.get_score();
    }

  return score;
}

void musical_analysis::force_init( int bells )
{
  // Going into this function initialises the static there.
  analyser::instance( bells );
}
