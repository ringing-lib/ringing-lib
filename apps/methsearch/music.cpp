// -*- C++ -*- music.cpp - things to analyse music
// Copyright (C) 2002, 2003, 2008, 2009, 2010, 2022
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation "methsearch/music"
#endif

#include "music.h"
#include "row_calc.h"
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
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

  friend class patterns;
    
  void check_n( int n );

  // Data members
  int bells;

  enum length { half_lead, half_lead_2, half_lead_r, half_lead_2r, 
                lead, course };
  map< pair<row,length>, music > musv;
};


musical_analysis::analyser::analyser( int bells )
  : bells(bells)
{
  const vector<string> &p = patterns::instance().p;

  vector< pair< row, length > > blocks;

  blocks.push_back( 
    make_pair( row(bells), // Course or lead head -- plain course by default
               course ) ); // Look at the whole course by default

  // We set this to true even though we haven't had any patterns.
  // This is because we want the first -Mtype=row option to reset
  // the default block (above) rather than augment it.
  bool had_patterns = true;  
  
  for ( vector<string>::const_iterator i( p.begin() ), e( p.end() );
	    i != e; ++i )
    {
      int scoreh = 1, scoreb = 1;
      string pattern( *i );

      string::size_type eq = pattern.find('=');
      if (eq == string::npos) eq = pattern.size();

      string const opt = pattern.substr(0,eq);

      if ( opt == "course" || opt == "lead" || opt == "halflead" ||
           opt == "2halflead" || opt == "2rhalflead" || opt == "rhalflead" )
        {
          length len = blocks.back().second;
          if (opt == "course") len = course;
          else if (opt == "lead") len = lead;
          else if (opt == "halflead") len = half_lead;
          else if (opt == "2halflead") len = half_lead_2;
          else if (opt == "2rhalflead") len = half_lead_2r;
          else if (opt == "rhalflead") len = half_lead_r;

          row lh = blocks.back().first;
          row_calc::flags rcf = static_cast<row_calc::flags>
            ( row_calc::allow_implicit_treble | row_calc::allow_row_promotion );
          scoped_pointer<row_calc> rc;
          if (eq != pattern.size()) {
            try {
              rc.reset( new row_calc( bells, pattern.substr(eq+1), rcf ) );
            } catch ( exception const& e ) {
              cerr << "Unable to parse music course head expression: " 
                   << e.what() << endl;
              exit(1);
            }
          }

          if (had_patterns) {
            blocks.clear();
            had_patterns = false;
          }
          if (rc)
            try {
              for ( row_calc::const_iterator i=rc->begin(), e=rc->end(); 
                      i!=e; ++i )
                blocks.push_back( make_pair( *i * row(bells), len ) );
            } catch ( row::invalid const& e ) {
              cerr << "Music course head produces invalid row: " 
                   << e.what() << endl;
              exit(1);
            }
           else
            blocks.push_back( make_pair( lh, len ) );

          continue;
        }

      had_patterns = true;
      for ( vector< pair<row,length> >::const_iterator 
              i = blocks.begin(), e = blocks.end(); i != e; ++i )
        {
          music& mu = musv[*i];
          mu.set_bells(bells);
    
          try {
            mu.add_scored_music_string( pattern );
          } 
          catch ( exception const& e ) {
            cerr << "Error parsing music pattern: " << e.what() << endl;
            exit(1);
          }           
        }
    }

  // By default we count the CRUs in the plain course.
  if ( musv.empty() || !had_patterns ) 
    {
      for ( vector< pair<row,length> >::const_iterator
              i = blocks.begin(), e = blocks.end(); i != e; ++i )
        {
          music& mu = musv[*i];
          mu.set_bells(bells);
          mu.add_named_music( "CRUs" ); 
        }
    }
}

void musical_analysis::add_pattern( const string &str )
{
  patterns::instance().p.push_back( str );
}

int musical_analysis::analyse( const method &m )
{
  int score = 0;

  typedef map< pair<row,analyser::length>, music > musv_t;
  musv_t& musv = analyser::instance( m.bells() ).musv;
  for ( musv_t::iterator mi=musv.begin(), me=musv.end(); mi!=me; ++mi)
    {
      vector< row > rows;
      row r = mi->first.first; // Set to the course head

      switch ( mi->first.second ) {
        case analyser::course:
          do 
            transform( m.begin(), m.end(), back_inserter(rows), permute(r) );
          while ( r != mi->first.first );
          break;

        case analyser::lead:
          transform( m.begin(), m.end(), 
                     back_inserter(rows), post_permute(r) );
          break;

        case analyser::half_lead:
          transform( m.begin(), m.begin() + m.size()/2, 
                     back_inserter(rows), post_permute(r) );
          break;

        case analyser::half_lead_2:
          transform( m.begin() + m.size()/2, m.end(),
                     back_inserter(rows), post_permute(r) );
          break;

        case analyser::half_lead_r:
	  transform( m.rbegin() + m.size()/2 + 1, m.rend(),
                     back_inserter(rows), post_permute(r) );
          rows.push_back(r);
          break;

        case analyser::half_lead_2r:
          transform( m.rbegin() + 1, m.rbegin() + m.size()/2 + 1,
                     back_inserter(rows), post_permute(r) );
          break;

        default:
          assert(false);
      }

      mi->second.process_rows( rows.begin(), rows.end() );  
      score += mi->second.get_score();
    }

  return score;
}

void musical_analysis::force_init( int bells )
{
  // Going into this function initialises the static there.
  analyser::instance( bells );
}
