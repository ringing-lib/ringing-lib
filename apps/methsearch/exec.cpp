// -*- C++ -*- exec.cpp - execute sub-processes
// Copyright (C) 2003 Richard Smith <richard@ex-parrot.com>

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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "exec.h"
#include "expression.h"
#include "format.h"
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif
#include <string>
#if RINGING_OLD_C_INCLUDES
#include <errno.h>
#include <stdlib.h>
#else
#include <cerrno>
#include <cstdlib>
#endif
#ifdef _MSC_VER
#include <io.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <ringing/streamutils.h>


RINGING_USING_NAMESPACE
RINGING_USING_STD

class system_error : public logic_error {
public:
  system_error( int errnum, const char* fn )
    : logic_error( make_string() << "System error: " << fn << ": " 
		   << strerror(errnum) )
  {
  }
};

string exec_command( const string& str )
{
  // Construct this before any resources are claimed
  string result; 

  const char *argv[4]; // No ownership of memory

  // Respect the SHELL environment variable,
  // and drop back to using /bin/sh
  argv[0] = getenv("SHELL");
  if ( !argv[0] || !*argv[0] ) 
#   if RINGING_WINDOWS
      argv[0] = "cmd.exe";
#   else
      argv[0] = "/bin/sh";
#   endif

  argv[1] = "-c";
  argv[2] = str.c_str();
  argv[3] = NULL;

  // -------
  // From now on we have to be careful about exceptions
  // as we have pipes and child processes hanging around
  //
  int pipedes[2];
  if ( pipe(pipedes) == -1 )
    throw system_error( errno, "pipe" );
 
  int pid = vfork();
  if ( pid == 0 ) {
    close( pipedes[0] ); // reading end

    // Make the writing end of the pipe the new stdout (fd 1) 
    // for the  child process.
    dup2( pipedes[1], 1 );
    close( pipedes[1] );


    // Semantically this is a char const* const*, but POSIX
    // seems to disagree...
    execvp( argv[0], const_cast<char* const*>( argv ) );
    _exit( 127 ); // Can't happen
  }
  else if ( pid == -1 ) {
    close( pipedes[0] );
    close( pipedes[1] );
    throw system_error( errno, "vfork" ); 
  }

  close( pipedes[1] ); // writing end 

  try {
    while (true) {
      const size_t buflen(4096);
      char buffer[buflen];
    
      int i = read( pipedes[0], buffer, buflen-1 );
      if (i <= 0) break; // TODO: Do something more sensible
      result.append( buffer, i );
    }
  } 
  catch( ... ) {
    close( pipedes[0] ); // reading end

    int status, w;
    while ( ( w = wait( &status ) ) == -1 && errno == EINTR );
    if ( w == -1 )
      ;// Ignore an error here, as we're already in a bad way ...

    throw;
  }

  close( pipedes[0] );

  // Reap child processes
  int status, w;
  while ( ( w = wait( &status ) ) == -1 && errno == EINTR );
  if ( w == -1 )
    throw system_error( errno, "wait" ); 
    
  return result;
}


class exec_expr_node : public expression::s_node {
public:
  exec_expr_node( const string& expr ) 
    : fs(expr, format_string::preparsed_type) 
  {}

private:
  virtual string s_evaluate( const method_properties& m ) const {
    make_string ms;
    fs.print_method( m, ms.out_stream() );
    return exec_command( ms );
  }

  format_string fs;
};

size_t store_exec_expression( const string& expr ) 
{
  return expression_cache::store( expression( new exec_expr_node( expr ) ) );
}
