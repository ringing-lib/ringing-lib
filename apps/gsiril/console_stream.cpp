#include <ringing/common.h>
#include "console_stream.h"
#if RINGING_USE_READLINE
#include "rlstream.h"
#endif
#if RINGING_HAVE_OLD_IOSTREAMS
#include <iostream.h>
#include <streambuf.h>
#else
#include <iosfwd>
#include <iostream>
#include <streambuf>
#endif

RINGING_USING_NAMESPACE

void console_istream::set_prompt( const char *prompt ) 
{
#if RINGING_USE_READLINE
  if ( rl_streambuf *rlsb = dynamic_cast< rl_streambuf * >( buf.get() ) )
    rlsb->set_prompt(prompt); 
#endif
}

streambuf *console_istream::make_streambuf()
{
#if RINGING_USE_READLINE
  return new rl_streambuf;
#else
  return NULL;
#endif
}

console_istream::console_istream( bool interactive )
  : istream( NULL ),
    buf( interactive ? make_streambuf() : NULL )
{
  istream::init( buf.get() ? buf.get() : cin.rdbuf() );
}

console_istream::~console_istream()
{
}
