#include <ringing/common.h>
#include "util.h"
#include <string>
#include <ringing/streamutils.h>
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <cstdlib.h>
#else
#include <cctype>
#include <cstdlib>
#endif
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif

RINGING_USING_NAMESPACE

void trim_leading_whitespace( string &line )
{
  string::iterator i( line.begin() ), e( line.end() );
  while ( i != e && isspace(*i) ) ++i;
  line = string( i, e );
}

void trim_trailing_whitespace( string &line )
{
  string::iterator b( line.begin() ), j( line.end() );
  while ( j-1 != b && isspace(*(j-1)) ) --j;
  line = string( b, j );
}

int string_to_int( const string &num )
{
  char *nptr;
  int n = strtol( num.c_str(), &nptr, 10 );

  for ( string::const_iterator i = num.begin() + (nptr - num.c_str()); 
	i != num.end();  ++i )
    if ( !isspace(*i) )
      throw runtime_error( make_string() << "'" << num << "'"
			   " is not an integer" );

  return n;
}
