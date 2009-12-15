// ringmethod.cpp - Create audio output of ringing.
// Copyright (C) 2009 Richard Smith <richard@ex-parrot.com>

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

#include <cmath>
#include <stdint.h>
#include <iostream>
#include <cassert>
#include <vector>
#include <iterator>

#include <ringing/bell.h>
#include <ringing/row.h>
#include <ringing/pointers.h>
#include <ringing/streamutils.h>

#include "args.h"
#include "init_val.h"


RINGING_USING_NAMESPACE
RINGING_USING_STD

const double pi = 3.14159265358979323844;

class harmonic {
public:
  harmonic( double freq, double decay, int sample_rate );

  void strike();
  double sample();

private:
  double freq;
  double decay;
  int sample_rate;

  double cos_omega_dt;
  double decay_multiple;

  double x[3];
};

harmonic::harmonic( double freq, double decay, int sample_rate )
  : freq(freq), decay(decay), sample_rate(sample_rate),
    cos_omega_dt( cos( 2*pi*freq/sample_rate ) ),
    decay_multiple( exp( - decay/sample_rate ) )
{
  x[0] = x[1] = x[2] = 0;
}

void harmonic::strike() 
{
  x[1] = cos_omega_dt;
  x[2] = 1;
}

double harmonic::sample()
{
  // cos(a + n.wdt) = 2 cos( a + (n-1)wdt ) cos(wdt) - cos( a + (n-2)wdt )
  x[0] = 2*x[1]*cos_omega_dt - x[2];
  x[2] = x[1] * decay_multiple;
  return x[1] = x[0] * decay_multiple;
}

class bell_sound {
public:
  bell_sound( double freq, int sample_rate );

  void strike();
  double sample();

private:
  vector<harmonic> x;
};

bell_sound::bell_sound( double freq, int sample_rate )
{
  x.reserve(5);

  x.push_back( harmonic( freq/4,            0.5, sample_rate ) ); // Hum
  x.push_back( harmonic( freq/2,            2.5, sample_rate ) ); // Prime
  x.push_back( harmonic( freq/pow(2,9/12.), 2.5, sample_rate ) ); // Tierce
  x.push_back( harmonic( freq/pow(2,5/12.), 2.5, sample_rate ) ); // Quint
  x.push_back( harmonic( freq,              2.5, sample_rate ) ); // Nominal
}


void bell_sound::strike()
{
  for ( vector<harmonic>::iterator i=x.begin(), e=x.end(); i!=e; ++i )
    i->strike();
}

double bell_sound::sample()
{
  double sum = 0;
  for ( vector<harmonic>::iterator i=x.begin(), e=x.end(); i!=e; ++i )
    sum += i->sample();
  return sum;
}

class tower_sounds {
public:
  tower_sounds( double tenor_freq, int num, int sample_rate );

  void   strike( bell b );
  double sample();

  int bells() const { return bellv.size(); }
  int sample_rate() const { return srate; }

private:
  vector<bell_sound> bellv;
  int srate;
};

tower_sounds::tower_sounds( double tenor_freq, int num, int sample_rate )
  : srate(sample_rate)
{
  tenor_freq *= pow( 2, num / 7 );
  bellv.reserve(num);
  int n = num % 7;
  while (n <= num) {
    switch (n) {
    default:
      bellv.push_back( bell_sound( tenor_freq * pow(2,11/12.), srate ) );
    case 6:
      bellv.push_back( bell_sound( tenor_freq * pow(2, 9/12.), srate ) );
    case 5:
      bellv.push_back( bell_sound( tenor_freq * pow(2, 7/12.), srate ) );
    case 4:
      bellv.push_back( bell_sound( tenor_freq * pow(2, 5/12.), srate ) );
    case 3:
      bellv.push_back( bell_sound( tenor_freq * pow(2, 4/12.), srate ) );
    case 2:
      bellv.push_back( bell_sound( tenor_freq * pow(2, 2/12.), srate ) );
    case 1:
      bellv.push_back( bell_sound( tenor_freq * pow(2, 0/12.), srate ) );
    }
    n += 7;
    tenor_freq /= 2;
  }
}

void tower_sounds::strike( bell b )
{
  assert( b < bellv.size() && b >= 0 );
  bellv[b].strike();
}

double tower_sounds::sample() 
{
  double sum = 0;
  for ( vector<bell_sound>::iterator i=bellv.begin(), e=bellv.end(); 
          i!=e; ++i )
    sum += i->sample();
  return sum;
} 

class row_player {
public:
  row_player( tower_sounds& tower, int peal_speed );

private:
  class row_reader_base {
  public:
    virtual ~row_reader_base() {}
    virtual row next() = 0;
  };

  template <class RowIterator>
  class row_reader : public row_reader_base {
  public:
    row_reader( RowIterator first, RowIterator last )
      : first(first), last(last) {}

    virtual row next() {
      return first == last ? row() : *first++; 
    }

  private:
    RowIterator first, last;
  };

  void do_ring( shared_pointer<row_reader_base> const& r );

public:
  template <class RowIterator>
  void ring( RowIterator first, RowIterator last ) {
    return do_ring(
      shared_pointer<row_reader_base>( 
        new row_reader<RowIterator>( first, last ) ) );
  }

private:
  tower_sounds tower;
  double bell_sep;
};

row_player::row_player( tower_sounds& tower, int peal_speed )
  : tower(tower), bell_sep(peal_speed * 60 / 2500.0 / (tower.bells()*2+1))
{
  assert( bell_sep * tower.sample_rate() > 2.0 );
}

void row_player::do_ring( shared_pointer<row_reader_base> const& rr )
{
  row r = rr->next();  

  size_t bell_idx = 0, row_idx = 0;
  bool have_queued_bell = false;
  bell queued_bell;
  int queued_timestep;
  int eof_timestep;

  size_t t = 0;

  const size_t datasz = 256;
  int16_t data_idx = 0, data[datasz];
  while (r.bells() || t < eof_timestep)
  {
    if (!have_queued_bell && r.bells()) {
      queued_timestep = (bell_idx + row_idx * tower.bells() + row_idx/2)
                       * bell_sep * tower.sample_rate();
      queued_bell = r[bell_idx++];
      if (bell_idx == tower.bells()) {
        bell_idx = 0, row_idx++; // get next row
        r = rr->next();  
        if (r.bells() == 0) 
          eof_timestep = t + 5 * tower.sample_rate();
      }
      have_queued_bell = true;
    }

    if (have_queued_bell && queued_timestep == t) {
      tower.strike( queued_bell );
      have_queued_bell = false;
    }

    data[data_idx] = (int16_t)( tower.sample() * 1000 );
    ++data_idx;  ++t;

    if (data_idx == datasz) {
      cout.write( (char const*)data, 2*datasz );
      data_idx = 0;
    }
  }
}

class time_opt : public option
{
public:
  time_opt( char c, const string& l, const string& d, const string& a,
            init_val_base<int>& opt )
    : option(c, l, d, a), opt(opt.get())
  {}

  bool process( const string& arg, const arg_parser& ap ) const;

public:
  int& opt;
};

bool time_opt::process( const string& arg, const arg_parser& ap ) const
{
  try {
    string::size_type i = arg.find_first_of("hm:");
    opt = lexical_cast<int>(arg.substr(0,i));

    if ( i != string::npos && arg[i] != 'm' ) {
      opt *= 60;
      if ( i+1 != arg.size() )
        opt += lexical_cast<int>(arg.substr(i+1, arg.find('m',i+1)-i-1));
    }
  } catch ( bad_lexical_cast const& ) {
    ap.error( make_string() << "Invalid time argument: \"" << arg << "\"" );
    return false;
  }
  return true;
}

class arguments {
public:
  void bind( arg_parser& p );

  init_val<int, 0>     bells;
  init_val<int, 22050> sample_rate;
  init_val<int, 180>   peal_speed;
  init_val<int, 587>   tenor_nominal;
};

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );
    
  p.add( new integer_opt
         ( 'b', "bells",
           "The number of bells.  This option is required", "BELLS",
           bells ) );

  p.add( new integer_opt
         ( 'r', "sample-rate",
           "The sample rate in Hertz (default: 22050)",  "NUM",
           sample_rate ) );

  p.add( new time_opt
         ( 's', "peal-speed", 
           "The peal speed in minutes, or hours and minutes (default: 3h)", 
           "TIME", peal_speed ) );

  p.add( new time_opt
         ( 'f', "frequency", 
           "The frequency of the tenor nominal in Hertz (default: 587 = D)",
           "FREQ", tenor_nominal ) );
}

int main(int argc, char* argv[])
{
  arguments args;

  {
    arg_parser ap(argv[0], "ringmethod -- convert a sequence of rows into a "
                  "raw audio feed.", "OPTIONS" );
    
    args.bind( ap );
    
    if ( !ap.parse(argc, argv) ) 
      {
	ap.usage();
	return 1;
      }
  }

  tower_sounds tower( args.tenor_nominal, args.bells, args.sample_rate ); 
  row_player player( tower, args.peal_speed );
  player.ring( istream_iterator<row>(cin), istream_iterator<row>() );
}

