#include <ringing/common.h>
#include <ringing/pointers.h>
#include "console_stream.h"
#include "parser.h"
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif
#if RINGING_OLD_IOSTREAMS
#include <iostream.h>
#else
#include <iostream>
#endif


RINGING_USING_NAMESPACE


void welcome()
{
  cout << "gsiril " RINGING_VERSION "\n"
          "Ringing Class Library version " RINGING_VERSION << "." 
       << endl;
}

static const char *init_strings[] = {
  "true     = \"# rows ending in @\", \"Touch is true\"",
  "notround = \"# rows ending in @\", \"Is this OK?\"",
  "false    = \"# rows ending in @\", \"Touch is false in $ rows\"",
  "conflict = \"# rows ending in @\", \"Touch not completed due to false row$$\"",
  "rounds   = ",
  "everyrow = ",
  NULL
};

int main( int argc, const char *argv[] )
{
  bool interactive = false;

  for ( int i=1; i<argc; ++i )
    {
      string arg( argv[i] );
      if (arg == "-i" || arg == "--interactive" )
	interactive = true;
    }

  try
    {
      if (interactive) welcome();

      console_istream in( interactive );
      in.set_prompt( "> " );

      parser p;
      p.set_interactive( interactive );

      // Prepopulate symbol table
      for ( const char **str = init_strings; *str; ++str )
	p.init_with( *str );

      p.read_file( in, cout );

      p.run_final_proof( cout );
    }
  catch ( const exception &ex )
    {
      cerr << "Unexpected error: " << ex.what() << endl;
      exit(1);
    }
  catch ( ... )
    {
      cerr << "An unknown error occured" << endl;
      exit(1);
    }

  return 0;
}

