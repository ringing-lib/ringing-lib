// -*- C++ -*-

#ifndef GSIRIL_RLSTREAM_INCLUDED
#define GSIRIL_RLSTREAM_INCLUDED

#include <ringing/common.h>
#if !RINGING_USE_READLINE
#error "This file requires GNU readline" 
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <streambuf.h>
#else
#include <iosfwd>
#include <streambuf>
#endif
#include <string>

RINGING_USING_NAMESPACE

class rl_streambuf : public streambuf
{
  // Typedefs
#if RINGING_HAVE_OLD_IOSTREAMS
private:
  typedef int int_type;
#else
public:
  typedef char                   char_type;
  typedef char_traits<char_type> traits;
  typedef traits::int_type       int_type;
  typedef traits::pos_type       pos_type;
  typedef traits::off_type       off_type;
#endif

public:
  // Constructors / Destructors
  rl_streambuf() {}
  virtual ~rl_streambuf();

  void set_prompt( const string &p ) { prompt = p; }

protected:
  // Overridden virtual functions
  virtual int_type    underflow() { return underflow_common(0); }
  virtual int_type    uflow()     { return underflow_common(1); }

private:
  // Helpers
  int_type underflow_common( int bump );

  // Unimplemented copy constructor and copy assignment operator
  rl_streambuf &operator=( const rl_streambuf & );
  rl_streambuf( const rl_streambuf & );

  // Data members
  string prompt;
};

#endif // GSIRIL_RLSTREAM_INCLUDED
