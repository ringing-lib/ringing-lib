#include <ringing/common.h>
#if !RINGING_USE_READLINE
#error "This file requires GNU readline" 
#endif
#include "rlstream.h"
#include <readline/readline.h>
#include <readline/history.h>
#if RINGING_OLD_C_INCLUDES
#include <string.h>
#include <ctype.h>
#else
#include <cstring>
#include <cctype>
#endif

RINGING_USING_NAMESPACE

rl_streambuf::~rl_streambuf() 
{ 
  if (eback()) free( eback() );
  streambuf::setg( NULL, NULL, NULL );
}

rl_streambuf::int_type rl_streambuf::underflow_common( int bump )
{
  if (eback()) free( eback() );
  streambuf::setg( NULL, NULL, NULL );

  char *b = const_cast<char *>
    ( readline( const_cast<char*>( prompt.c_str() ) ) );
#if RINGING_HAVE_OLD_IOSTREAMS
  if (!b) return -1;
#else
  if (!b) return traits::eof();
#endif

  streambuf::setg( b, b+bump, b + strlen(b) + 1 );
  for ( const char *p=eback(); p < egptr()-1; ++p )
    if ( !isspace(*p) )
      {
	add_history(b);
	break;
      }
  egptr()[-1] = '\n';
  return *b;
}

