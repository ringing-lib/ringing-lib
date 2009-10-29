// -*- C++ -*- exec.cpp - execute sub-processes
// Copyright (C) 2003, 2004, 2008, 2009 Richard Smith <richard@ex-parrot.com>

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

// The parts of this file for creating a Windows command line are taken
// from GNU make [see http://www.gnu.org/software/make for details], 
// and are under the following copyright:

//   Copyright (C) 1988, 1989, 1991-1997, 1999, 2002 Free Software 
//   Foundation, Inc.

// $Id$

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "exec.h"
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#else
#include <stdexcept>
#endif
#include <string>
#if RINGING_OLD_C_INCLUDES
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#else
#include <cerrno>
#include <cstdlib>
#include <cstring>
#endif
#if RINGING_WINDOWS && !defined(__CYGWIN__)
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <ringing/streamutils.h>

#ifdef _MSC_VER
// Microsoft have deprecated getenv in favour of a non-standard
// extension, getenv_s.
#pragma warning (disable: 4996)
#endif

RINGING_USING_NAMESPACE
RINGING_USING_STD

class system_error : public logic_error {
public:
  system_error( int errnum, const char* fn );
};

#if RINGING_WINDOWS && !defined(__CYGWIN__)

string windows_errstr( int errnum )
{
  // Create before call to FormatMessage in case this throws
  string s;

  {
    LPSTR msg_buf; // or 'char *' as we call it in English.

    FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, errnum, 
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    reinterpret_cast<char*>( &msg_buf ), // Brain dead API
                    0, NULL );

    try {
      s = msg_buf;
    }
    catch( ... ) {
      LocalFree( msg_buf );
      throw;
    }

    LocalFree( msg_buf );
  }

  return s;
}

system_error::system_error( int errnum, const char* fn )
  : logic_error( make_string() << "System error: " << fn << ": " 
		   << windows_errstr( errnum ) )
{
}

#define THROW_SYSTEM_ERROR( str ) \
  do { throw system_error( GetLastError(), str ); } while (false)


/*
 * Description:
 *	 Create a command line buffer to pass to CreateProcess
 *
 * Returns:  the buffer or NULL for failure
 *	Shell case:  sh_name a:/full/path/to/script argv[1] argv[2] ...
 *  Otherwise:   argv[0] argv[1] argv[2] ...
 *
 * Notes/Dependencies:
 *   CreateProcess does not take an argv, so this command creates a
 *   command line for the executable.
 */

static string
make_command_line( char const* full_exec_path, char const* const* argv)
{
	int		   argc = 0;
	char const* const* argvi = argv;
	int*		   enclose_in_quotes = NULL;
	int*		   enclose_in_quotes_i;
	unsigned int       bytes_required = 0;
	char*	  	   command_line;
	char*		   command_line_i;

	while (*(argvi++)) argc++;

	if (argc) {
		enclose_in_quotes = (int*) calloc(1, argc * sizeof(int));
		if (!enclose_in_quotes) throw bad_alloc();
	}

	/* We have to make one pass through each argv[i] to see if we need
	 * to enclose it in ", so we might as well figure out how much
	 * memory we'll need on the same pass.
	 */

	argvi = argv;
	enclose_in_quotes_i = enclose_in_quotes;
	while(*argvi) {
		char const* p = *argvi;
		unsigned int backslash_count = 0;

		/*
		 * We have to enclose empty arguments in ".
		 */
		if (!(*p)) *enclose_in_quotes_i = 1;

		while(*p) {
			switch (*p) {
			case '\"':
				/*
				 * We have to insert a backslash for each "
				 * and each \ that precedes the ".
				 */
				bytes_required += (backslash_count + 1);
				backslash_count = 0;
				break;

			case '\\':
				backslash_count++;
				break;
	/*
	 * At one time we set *enclose_in_quotes_i for '*' or '?' to suppress
	 * wildcard expansion in programs linked with MSVC's SETARGV.OBJ so
	 * that argv in always equals argv out. This was removed.  Say you have
	 * such a program named glob.exe.  You enter
	 * glob '*'
	 * at the sh command prompt.  Obviously the intent is to make glob do the
	 * wildcarding instead of sh.  If we set *enclose_in_quotes_i for '*' or '?',
	 * then the command line that glob would see would be
	 * glob "*"
	 * and the _setargv in SETARGV.OBJ would _not_ expand the *.
	 */
			case ' ':
			case '\t':
				*enclose_in_quotes_i = 1;
				/* fall through */

			default:
				backslash_count = 0;
				break;
			}

			/*
			 * Add one for each character in argv[i].
			 */
			bytes_required++;

			p++;
		}

		if (*enclose_in_quotes_i) {
			/*
			 * Add one for each enclosing ",
			 * and one for each \ that precedes the
			 * closing ".
			 */
			bytes_required += (backslash_count + 2);
		}

		/*
		 * Add one for the intervening space.
		 */
		if (*(++argvi)) bytes_required++;
		enclose_in_quotes_i++;
	}

	/*
	 * Add one for the terminating NULL.
	 */
	bytes_required++;

	command_line = (char*) malloc(bytes_required);

	if (!command_line) {
		if (enclose_in_quotes) free(enclose_in_quotes);
		throw bad_alloc();
	}

	command_line_i = command_line;
	argvi = argv;
	enclose_in_quotes_i = enclose_in_quotes;

	while(*argvi) {
		char const* p = *argvi;
		unsigned int backslash_count = 0;

		if (*enclose_in_quotes_i) {
			*(command_line_i++) = '\"';
		}

		while(*p) {
			if (*p == '\"') {
				/*
				 * We have to insert a backslash for the "
				 * and each \ that precedes the ".
				 */
				backslash_count++;

				while(backslash_count) {
					*(command_line_i++) = '\\';
					backslash_count--;
				};
			}

			/*
			 * Copy the character.
			 */
			*(command_line_i++) = *(p++);
		}

		if (*enclose_in_quotes_i) {
			*(command_line_i++) = '\"';
		}

		/*
		 * Append an intervening space.
		 */
		if (*(++argvi)) {
			*(command_line_i++) = ' ';
		}

		enclose_in_quotes_i++;
	}

	/*
	 * Append the terminating NULL.
	 */
	*command_line_i = '\0';

	if (enclose_in_quotes) free(enclose_in_quotes);
        try {
          string rv( command_line );
          free( command_line );
          return rv;
        }
        catch (...) {
          free( command_line );
          throw;
        }
}

string exec_command( const string& str )
{
  // Construct this before any resources are claimed
  string result; 

  const char *argv[4]; // No ownership of memory

  // Respect the SHELL environment variable,
  // and drop back to using CMD.EXE
  argv[0] = getenv("SHELL");
  if ( !argv[0] || !*argv[0] ) {
    // TODO: On Windows NT 4, 2K and XP this is correct;
    // on Windows 95, 98 and ME, this should be COMMAND.COM,
    // though that'll probably be unusably slow.
    argv[0] = "CMD.EXE";
    argv[1] = "/C";
  } else {
    // Assume a POSIX-like shell that takes the -c option.
    // E.g. bash under Cygwin
    argv[1] = "-c";
  }
  argv[2] = str.c_str();
  argv[3] = NULL;

  // -------
  // From now on we have to be careful about exceptions
  // as we have pipes and child processes hanging around
  //
  HANDLE hErr;
  if (DuplicateHandle(GetCurrentProcess(),
                      GetStdHandle(STD_ERROR_HANDLE),
                      GetCurrentProcess(),
                      &hErr,
                      0,
                      TRUE,
                      DUPLICATE_SAME_ACCESS) == FALSE)
    THROW_SYSTEM_ERROR( "DuplicateHandle" );
  

  // Two ends of the pipe:
  HANDLE hChildOutRd, hChildOutWr;
  {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof (SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hChildOutRd, &hChildOutWr, &saAttr, 0)) {
      CloseHandle(hErr);
      THROW_SYSTEM_ERROR( "CreatePipe" );
    }
  }

  PROCESS_INFORMATION procInfo;

  {
    string command_line( make_command_line( argv[0], argv ) );

    STARTUPINFO startInfo;
    GetStartupInfo(&startInfo);
    startInfo.dwFlags = STARTF_USESTDHANDLES;
    startInfo.lpReserved = 0;
    startInfo.cbReserved2 = 0;
    startInfo.lpReserved2 = 0;
    startInfo.lpTitle = 0;
    startInfo.hStdInput = NULL;
    startInfo.hStdOutput = hChildOutWr;
    startInfo.hStdError = hErr;

    if (CreateProcess(0,
                      // Const correctness -- we've heard of it
                      const_cast< char* >( command_line.c_str() ),
                      NULL,
                      0, // default security attributes for thread
                      TRUE, // inherit handles (e.g. helper pipes)
                      0,
                      NULL,
                      0, // default starting directory
                      &startInfo,
                      &procInfo) == FALSE) {
      CloseHandle(hErr);
      CloseHandle(hChildOutRd);
      CloseHandle(hChildOutWr);
      THROW_SYSTEM_ERROR( "CreateProcess" );
    }

    // Close the thread handle -- we'll just watch the process
    CloseHandle(procInfo.hThread);
  }

  CloseHandle( hErr );
  CloseHandle( hChildOutWr );

  try {
    while (true) {
      const size_t buflen(4096);
      char buffer[buflen];

      // TODO:  Unicode conversions???

      DWORD i;
      ReadFile( hChildOutRd, buffer, buflen, &i, NULL);
      if (i <= 0) break; // TODO: Do something more sensible
      result.append( buffer, i ); // Might throw
    }
  } 
  catch( ... ) {
    CloseHandle( hChildOutRd );

    DWORD rv = WaitForMultipleObjects( 1, &procInfo.hProcess, 
				       FALSE, INFINITE );
    if ( rv == WAIT_FAILED ) abort();

    throw;
  }

  CloseHandle( hChildOutRd );

  DWORD rv = WaitForMultipleObjects( 1, &procInfo.hProcess, FALSE, INFINITE );
  if ( rv == WAIT_FAILED )
    THROW_SYSTEM_ERROR( "WaitForMultipleObjects" );  
  return result;
}

#else // !RINGING_WINDOWS -- assume POSIX

system_error::system_error( int errnum, const char* fn )
  : logic_error( make_string() << "System error: " << fn << ": " 
		   << strerror(errnum) )
{
}

#define THROW_SYSTEM_ERROR( str ) \
  do { throw system_error( errno, str ); } while (false)


string exec_command( const string& str )
{
  // Construct this before any resources are claimed
  string result; 

  const char *argv[4]; // No ownership of memory

  // Respect the SHELL environment variable,
  // and drop back to using /bin/sh
  argv[0] = getenv("SHELL");
  if ( !argv[0] || !*argv[0] ) 
    argv[0] = "/bin/sh";

  argv[1] = "-c";
  argv[2] = str.c_str();
  argv[3] = NULL;

  // -------
  // From now on we have to be careful about exceptions
  // as we have pipes and child processes hanging around
  //
  int pipedes[2];

  if (pipe(pipedes) == -1 )
    THROW_SYSTEM_ERROR( "pipe" );
 
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
    THROW_SYSTEM_ERROR( "vfork" );
  }

  close( pipedes[1] ); // writing end 
  int fd = pipedes[0]; // reading end

  try {
    while (true) {
      const size_t buflen(4096);
      char buffer[buflen];
    
      int i = read( fd, buffer, buflen-1 );
      if (i <= 0) break; // TODO: Do something more sensible
      result.append( buffer, i );
    }
  } 
  catch( ... ) {
    close( fd );

    // Reap child processes
    {
      int status, w;
      while ( ( w = wait( &status ) ) == -1 && errno == EINTR );
      if ( w == -1 ) abort();
    }

    throw;
  }

  close( fd );

  // Reap child processes
  {
    int status, w;
    while ( ( w = wait( &status ) ) == -1 && errno == EINTR );
    if ( w == -1 )
      THROW_SYSTEM_ERROR( "wait" );
  }

  if ( result.size() && result[result.size()-1] == '\n' )
    result.erase( result.size()-1 );

  return result;
}

#endif // RINGING_WINDOWS

