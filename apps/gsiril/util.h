#ifndef GSIRIL_UTIL_INCLUDED
#define GSIRIL_UTIL_INCLUDED

#include <ringing/common.h>
#include <string>

RINGING_USING_NAMESPACE

int string_to_int( const string &num );

void trim_leading_whitespace( string &line );
void trim_trailing_whitespace( string &line );

#endif // GSIRIL_UTIL_INCLUDED
