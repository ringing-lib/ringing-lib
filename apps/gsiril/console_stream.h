// -*- C++ -*-

#ifndef GSIRIL_CONSOLE_STREAM_INCLUDED
#define GSIRIL_CONSOLE_STREAM_INCLUDED

#include <ringing/common.h>
#if RINGING_HAVE_OLD_IOSTREAMS
#include <istream.h>
#else
#include <iosfwd>
#include <istream>
#endif
#include <ringing/pointers.h>

RINGING_USING_NAMESPACE

class console_istream : public istream
{
public:
#if !RINGING_HAVE_OLD_IOSTREAMS
  // Standard typedefs
  typedef char                   char_type;
  typedef char_traits<char_type> traits;
  typedef traits::int_type       int_type;
  typedef traits::pos_type       pos_type;
  typedef traits::off_type       off_type;
#endif

  // Constructor / destructor
  explicit console_istream( bool interactive );
 ~console_istream();

  // Public interface
  void set_prompt( const char *prompt );

private:
  // Helpers
  static streambuf *make_streambuf();

  // Data members
  scoped_pointer<streambuf> buf;
};

#endif // GSIRIL_CONSOLE_STREAM_INCLUDED
