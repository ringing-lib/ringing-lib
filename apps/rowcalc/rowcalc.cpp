#include <ringing/pointers.h>
#include <ringing/streamutils.h>
#include <ringing/row.h>

#include "init_val.h"
#include "row_calc.h"
#include "args.h"

#include <iostream>
#include <exception>
#include <string>
#include <algorithm>
#include <iterator>

RINGING_USING_NAMESPACE
RINGING_USING_STD

struct arguments
{
  init_val<int,0> bells;
  string          expr;

  void bind( arg_parser& p );
  bool validate( arg_parser& p );
};

void arguments::bind( arg_parser& p )
{
  p.add( new help_opt );
  p.add( new version_opt );

  p.add( new integer_opt
         ( 'b', "bells",
           "The number of bells.  This option is required", "BELLS",
           bells ) );

  p.set_default( new string_opt( '\0', "", "", "", expr ) );
}

bool arguments::validate( arg_parser& ap )
{
  if ( bells < 0 || bells >= int(bell::MAX_BELLS) )
    {
      ap.error( make_string() << "The number of bell must be greater than "
                "0 and less than " << bell::MAX_BELLS );
      return false;
    }
  return true;
}

int main( int argc, char *argv[] )
{
  arguments args;

  {
    arg_parser ap( argv[0], "rowcalc -- a calculator for rows", "OPTIONS" );
    args.bind( ap );

    if ( !ap.parse(argc, argv) )
      {
        ap.usage();
        return 1;
      }

    if ( !args.validate(ap) )
      return 1;
  }

  scoped_pointer<row_calc> rc;
  try {
    // If args.bells == 0 this is equivalent to omitting that argument.
    rc.reset( new row_calc( args.bells, args.expr ) );
  } 
  catch ( exception const& e ) { 
    cerr << "Error parsing expression: " << e.what() << "\n";
    return 1;
  }

  try {  
    copy( rc->begin(), rc->end(), ostream_iterator<row>(cout, "\n") );
  }
  catch ( row::invalid const& ex ) {
    cerr << "Invalid row produced: " << ex.what() << "\n";
    return 1;
  }
}
