// ringmethod.cpp - Create audio output of ringing.
// Copyright (C) 2009, 2010, 2011 Richard Smith <richard@ex-parrot.com>

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
#include <ctime>
#include <stdint.h>
#include <iostream>
#include <cassert>
#include <vector>
#include <iterator>

#include <ringing/bell.h>
#include <ringing/row.h>
#include <ringing/pointers.h>
#include <ringing/streamutils.h>
#include <ringing/mathutils.h>

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
  double phase;

  double cos_omega_dt;
  double decay_multiple;

  double x1_init, x2_init;

  double x[3];
};

harmonic::harmonic( double freq, double decay, int sample_rate )
  : freq(freq), decay(decay), sample_rate(sample_rate),
    phase( pi/2 ),
    cos_omega_dt( cos( 2*pi*freq/sample_rate ) ),
    decay_multiple( exp( - decay/sample_rate ) ),
    x1_init( cos( phase + 2*pi*freq/sample_rate ) ),
    x2_init( cos( phase ) )
{
  x[0] = x[1] = x[2] = 0;
}

void harmonic::strike() 
{
  x[1] = x1_init;
  x[2] = x2_init;
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
  tenor_freq *= ipower( 2, num / 7 );
  bellv.reserve(num);
  int n = num % 7;
  if (n == 0) n = 7;
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

struct offset {
  offset() :  meanpos_h(0), meanpos_b(0), stddev_h(-1), stddev_b(-1) {}

  double meanpos_h, meanpos_b;
  double stddev_h, stddev_b; // Negative values are replaced with default_dev
};

class row_player {
public:
  row_player( tower_sounds& tower, int peal_speed, double hs_lead,
              map<bell, offset> const& striking );

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

  void do_ring( shared_pointer<row_reader_base> const& r, ostream& os );

public:
  void header( ostream& os );

  template <class RowIterator>
  void ring( RowIterator first, RowIterator last, ostream& os ) {
    return do_ring(
      shared_pointer<row_reader_base>( 
        new row_reader<RowIterator>( first, last ) ),
      os );
  }

private:
  tower_sounds tower;
  double bell_sep;
  double hs_lead;
  map< bell, offset > striking;
};

row_player::row_player( tower_sounds& tower, int peal_speed, double hs_lead,
                        map<bell, offset> const& striking )
  : tower(tower), bell_sep(peal_speed * 60 / 2500.0 / (tower.bells()*2+1)),
    hs_lead(hs_lead), striking(striking)
{
  assert( bell_sep * tower.sample_rate() > 2.0 );
}

static void sprintf_uint32le( char* pbuf, uint32_t val )
{
  pbuf[0] = (val & 0x000000FF) >> 0x0;
  pbuf[1] = (val & 0x0000FF00) >> 0x4;
  pbuf[2] = (val & 0x00FF0000) >> 0x8;
  pbuf[3] = (val & 0xFF000000) >> 0xC;
}

void row_player::header( ostream& os )
{
  char buf[] = "RIFF"             // Chunk ID
               "\xFF\xFF\xFF\xFF" // Chunk length (36 is just header length
                                  // -- this is wrong, but if we're streaming
                                  // out in a single pass, we cannot know
                                  // the actual value; -1 seems de facto std.
                                  // unknown length)
               "WAVE"             // Format

               "fmt "             // Subchunk 1 ID
               "\x10\0\0\0"       // Subchunk 1 length (16 bytes)
               "\1\0"             // Audio format (1 is PCM)
               "\1\0"             // Number of channels (1 is mono)
               "\0\0\0\0"         // Sample rate -- overwritten below
               "\0\0\0\0"         // Byte rate   -- overwritten below
               "\2\0"             // Block align
               "\x10\0"           // Bit rate (16 bits)

               "data"             // Subchunk 2 ID
               "\xFF\xFF\xFF\xFF";// Subchunk 2 length (-1 is wrong, but 
                                  // apparently de facto standard when
                                  // streaming output)
     

  assert( sizeof(buf) == 44 + 1 );  // For final null

  // Write sample_rate as a little endian number
  // Don't use sprintf because that doesn't handle endianness
  sprintf_uint32le( &buf[24], tower.sample_rate() );

  // 1 * 16 / 8  ==  Num Channels * Bit rate / Bits per byte
  sprintf_uint32le( &buf[28], tower.sample_rate() * 1 * 16 / 8 );
 
  os.write( buf, 44 );
}

void row_player::do_ring( shared_pointer<row_reader_base> const& rr,
                          ostream& os )
{
  row r = rr->next();  
  if (r.bells() < tower.bells()) r *= row(tower.bells());

  size_t bell_idx = 0, row_idx = 0;

  vector< pair<bell, int> > queued_bells;
  int queued_timestamp = 0;

  //bool have_queued_bell = false;
  //bell queued_bell;
  //int queued_timestep;
  int eof_timestep;

  size_t t = 0;

  const size_t datasz = 256;
  int16_t data_idx = 0, data[datasz];
  while (r.bells() || t < eof_timestep)
  {
    if (queued_bells.empty() && r.bells()) {
      int ts;  // timestamp of last bell in row
      for (int i = 0; i<r.bells(); ++i) {
        offset const& o = striking[ r[i] ];
        double off = random_normal_deviate
          ( row_idx % 2 ? o.meanpos_b : o.meanpos_h,  
            row_idx % 2 ? o.stddev_b  : o.stddev_h );
        double posn = i + row_idx * tower.bells() + row_idx/2 * hs_lead + off;
        ts = (int)( posn * bell_sep * tower.sample_rate() );
        if (ts<0) ts = 0;
        if (ts<queued_timestamp) queued_timestamp = ts;
        queued_bells.push_back( make_pair( r[i], ts ) );
      }

      row_idx++; // get next row
      r = rr->next();  
      if (r.bells() == 0) 
        eof_timestep = ts + 5 * tower.sample_rate();
      else if (r.bells() < tower.bells()) 
        r *= row(tower.bells());
    }
 
    if (queued_bells.size() && queued_timestamp == t) {
      queued_timestamp = INT_MAX;
      for (int i=0; i < queued_bells.size(); ) {
        if (queued_bells[i].second == t) {
          tower.strike( queued_bells[i].first );
          queued_bells.erase( queued_bells.begin() + i );
        } else {
          if (queued_bells[i].second < queued_timestamp) 
            queued_timestamp = queued_bells[i].second;
          ++i;
        }
      }
    }

    data[data_idx] = (int16_t)( tower.sample() * 1000 );
    ++data_idx;  ++t;

    if (data_idx == datasz) {
      os.write( (char const*)data, 2*datasz );
      data_idx = 0;
    }
  }
}

class double_opt : public option
{
public:
  double_opt( char c, const string& l, const string& d, const string& a,
             double& opt )
    : option(c, l, d, a), opt(opt)
  {}

  bool process( const string& arg, const arg_parser& ap ) const;

public:
  double& opt;
};

bool double_opt::process( const string& arg, const arg_parser& ap ) const
{
  try {
    opt = lexical_cast<double>(arg);
  } catch ( bad_lexical_cast const& ) {
    ap.error( make_string() << "Invalid argument: \"" << arg << "\"" );
    return false;
  }
  return true;
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
  arguments() : hs_lead(1.0), default_deviation(0.0) {}

  void bind( arg_parser& p );
  bool validate( arg_parser& p );

  init_val<int, 0>     bells;
  init_val<int, 22050> sample_rate;
  init_val<int, 180>   peal_speed;
  init_val<int, 587>   tenor_nominal;
  double               hs_lead;
  double               default_deviation;
  init_val<int,-1>     seed;
  map<bell, offset>    striking;
};

class offset_opt : public option {
public:
  offset_opt( char c, const string& l, const string& d, const string& a,
              map<bell, offset> &opt )
    : option(c, l, d, a), opt(opt) 
  {}

private:
  virtual bool process( string const& arg, arg_parser const& ap ) const;
  bool parse_meandev( string const& meandev, arg_parser const& ap,
                      double& mean, double& stddev ) const;

  map<bell, offset>& opt;
};

bool offset_opt::parse_meandev( string const& meandev, arg_parser const& ap,
                                double& mean, double& stddev ) const
{
  // MEANDEV := OFFSET? ( ',' STDDEV )?
  size_t comma = meandev.find(',');
  if ( comma != 0 && meandev.size() ) {
    string meanstr = meandev.substr( 0, comma == string::npos 
                                          ? meandev.size() : comma );
    try { 
      mean = lexical_cast<double>( meanstr );
    } catch ( bad_lexical_cast const& ) {
      ap.error( make_string() << "Invalid offset: \"" << meanstr << "\"" );
      return false;
    }
  }

  if ( comma != string::npos ) {
    try { 
      stddev = lexical_cast<double>( meandev.substr(comma) );
    } catch ( bad_lexical_cast const& ) {
      ap.error( make_string() << "Invalid deviation: \"" 
                              << meandev.substr(comma) << "\"" );
      return false;
    }
    if (stddev < 0) {
      ap.error( make_string() << "Negative deviation: \"" 
                              << meandev.substr(comma) << "\"" );
      return false;
    }
  }

  return true;
}

bool offset_opt::process( string const& arg, arg_parser const& ap ) const
{
  // Format := BELL '=' ( MEANDEV | MEANDEV ';' MEANDEV )
  // where MEANDEV := OFFSET? ( ',' STDDEV )?

  int eq = arg.find('=');
  if (eq == string::npos ) {
    ap.error("Offset option should be of format BELL=OFFSET: no '=' found");
    return false;
  }

  bell b;
  if (eq == 1) {
    try {
      b = bell::read_char(arg[0]);
    } catch ( bell::invalid const& ) {
      ap.error( make_string() << "Invalid bell symbol: \"" << arg[0] << "\"" );
      return false;
    }
  }
  else {
    try { 
      b = bell( lexical_cast<int>( arg.substr(0,eq) ) - 1 );
    } catch ( bad_lexical_cast const& ) {
      ap.error( make_string() << "Invalid bell number: \"" 
                              << arg.substr(0,eq) << "\"" );
      return false;
    }
  }

  offset& off = opt[b];

  size_t semi = arg.find(';', eq+1);
  if ( semi == string::npos ) {
    if ( !parse_meandev( arg.substr(eq+1), ap, off.meanpos_h, off.stddev_h ) )
      return false;
    off.meanpos_b = off.meanpos_h;
    off.stddev_b  = off.stddev_h;
  } else {
    if ( !parse_meandev( arg.substr(eq+1,semi-eq-1), ap, 
                         off.meanpos_h, off.stddev_h ) )
      return false;
    if ( !parse_meandev( arg.substr(semi+1), ap, 
                         off.meanpos_b, off.stddev_b ) )
      return false;
  }

  return true;
}

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

  p.add( new double_opt
         ( 'h', "handstroke-lead",
           "The size of the handstroke lead as a multiple of the inter-bell "
           "spacing", "VAL", hs_lead ) );

  p.add( new double_opt
         ( 'd', "deviation",
           "Standard deviation of bells within the row in multiples of "
           "the inter-bell spacing", "VAL", default_deviation ) );

  p.add( new offset_opt 
         ( 'o', "offset",
           "Set the offset and standard deviation of the striking of BELL",
           "BELL=OFFSET,STDDEV", striking ) );

  p.add( new integer_opt
         ( '\0', "seed",
           "Seed the random number generator",  "NUM",
           seed ) );
}

bool arguments::validate( arg_parser& ap )
{
  for ( bell b=0; b<bells; ++b )
    if ( striking.find(b) == striking.end() ) 
      striking[b] = offset();

  for ( map<bell, offset>::iterator i=striking.begin(), e=striking.end();
        i != e; ++i ) {
    if ( i->second.stddev_h < 0 )  
      i->second.stddev_h = default_deviation;
    if ( i->second.stddev_b < 0 )  
      i->second.stddev_b = default_deviation;
  }

  return true;
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

    if ( !args.validate(ap) )
      return 1;
  }

  if ( args.seed == -1 )
    srand( args.seed = time(NULL) );
  else if ( args.seed )
    srand( args.seed );

  tower_sounds tower( args.tenor_nominal, args.bells, args.sample_rate ); 
  row_player player( tower, args.peal_speed, args.hs_lead, args.striking );
  player.header( cout );
  player.ring( istream_iterator<row>(cin), istream_iterator<row>(), cout );
}

