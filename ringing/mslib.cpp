// mslib.cpp - Read and write MicroSIRIL libraries
// Copyright (C) 2001-2 Martin Bright <M.Bright@dpmms.cam.ac.uk>
// and Richard Smith <richard@ex-parrot.com>

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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#if RINGING_OLD_C_INCLUDES
#include <string.h>
#include <ctype.h>
#else
#include <cstring>
#include <cctype>
#endif
#include <ringing/mslib.h>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

class mslib::entry_type : public library_entry
{
  // The public interface
  virtual string name() const;
  virtual string base_name() const;
  virtual string pn() const;
  virtual int bells() const { return b; }

  // Helper functions
  friend class mslib;
  entry_type();
  virtual bool readentry( ifstream &ifs );
  virtual library_entry *clone() const { return new entry_type(*this); }

  // The current line
  string linebuf;

  // The number of bells
  int b;
};


mslib::const_iterator mslib::begin() const
{
  ifstream *ifs = const_cast< ifstream * >( &f );
  ifs->clear();
  ifs->seekg(0, ios::beg);
  return const_iterator(ifs, new mslib::entry_type);
}

mslib::const_iterator mslib::end() const
{
  return const_iterator();
}

mslib::entry_type::entry_type()
  : b(0)
{}

bool mslib::entry_type::readentry( ifstream &ifs )
{
  while ( ifs )
    {
      getline( ifs, linebuf );

      if ( linebuf.length() > 0 && linebuf[0] == '*' )
	{
	  // The header 
	  string::size_type p = linebuf.find(' ');
	  if ( p != string::npos )
	    {
	      ++p;

	      // Skip the class code
	      while ( p < linebuf.size() && isalpha( linebuf[p] ) ) ++p;

	      // Extract number of bells
	      if ( p < linebuf.size() )
		b = atoi( linebuf.c_str() + p );
	    }
	}
      // An added check.  If this returns true we're looking at a 
      // CC library, not a MicroSIRIL one.
      else if ( linebuf.find("Name") != string::npos && 
		linebuf.find("No.") != string::npos )
	{
	  return false;
	}
      else if ( linebuf.length() > 0 )
	{
	  // Entries have exactly two spaces.
	  if ( count(linebuf.begin(), linebuf.end(), ' ') != 2 )
	    return false;

	  break;
	}
    }

  return ifs;
}

string mslib::entry_type::name() const
{
  return string( linebuf, 0, linebuf.find(' ') );
}

string mslib::entry_type::base_name() const
{
  // TODO  Strip trailing 'place' or 'bob' off plain methods
  return name();
}

string mslib::entry_type::pn() const
{
  string::size_type s1( linebuf.find(' ') );
  string::size_type s2( linebuf.find(' ', s1+1) );

  // Get the lead head code
  string lh( linebuf, s1+1, s2-s1-1 );

  string placenotation( linebuf, s2+1, string::npos );

  // Now remove space on end of line.
  {
    string::iterator j = placenotation.end();
    while ( j > placenotation.begin() && isspace(*(j - 1)) ) --j;
    placenotation.erase(j, placenotation.end());
  }


  // if we have a + on the front it is not a reflection method,
  // hence don't add the last change.
  if (placenotation[0] != '+')
    {
      if ( lh.size() && lh[ lh.size()-1 ] == 'z' )
	{
	  placenotation.append(",");
	  placenotation.append(lh, 0, lh.size()-1);
	}
      else if ( lh[0] >= 'a' && lh[0] <= 'f' ||
		lh[0] == 'p' || lh[0] == 'q' )
	{
	  placenotation.append(",2");
	}
      else
	{
	  placenotation.append(",1");
	}
    }

  return placenotation;
}

// ---------------------------------------------------------------------


static int extractNumber(const string &filename)
{
  string::const_iterator s;

  // Get the number off the end of the file name
  // Is there a '.'? e.g. '.txt', if so account for it
  // We want the last one so that a filename like
  // ../libraries/surprise8.txt works.
  string subname(filename, 0, filename.find_last_of('.'));

  // now start to reverse from last.
  for(s = subname.end(); s > subname.begin() && isdigit(s[-1]); s--);
  return atoi(&*s);
}


mslib::mslib(const string& name) 
  : f(name.c_str()), wr(0), _good(0)
{
  // Open file. Not going to bother to see if it's writeable as the
  // save function is not currently planned to be implemented.
  if(f.good())
    {
      _good = 1;
      b = extractNumber(name);
    }
}

// Is this file in the right format?
library_base *mslib::canread(ifstream& ifs, const string& name)
{
  if ( const_iterator(&ifs, new entry_type) != const_iterator() )
    return new mslib( name );
  else
    return NULL;
}

#if 0
// Write a method to a MicroSIRIL library
//
// This isn't ideal, because MicroSIRIL libraries are far
// from ideal.  If the name is more than one word, we run them
// all together and make sure the first letter is upper case, and
// the rest lower case.  If the method isn't already in the library,
// put it in so that everything stays in alphabetical order.
int mslib::save(method& m)
{
  char *buffer = new char[160];
  char *buffer2 = new char[160];
  char name[32];

  // First sort the name out.
  char *s = m.name(), *t;
  if(*s == '\0') return -1;
  *t++ = toupper(*s++);
  while(*s != '\0') {
    if(isspace(*s))      s++;
    else
      *t++ = tolower(*s++);
  }
  *t = '\0';

  streampos rpos, wpos;

  // Now let's see whether the method is already in the library
  f.seekg(0, ios::beg);
  while(f) {
    wpos = f.tellg();
    s = name;
    // See whether the name matches
    while(*s && tolower(f.get()) == tolower(*s++));
    if(*s == '\0' && isspace(f.get()) == ' ') // Found it
      break;
    while(!f.eof() && f.get() != '\n');	// Skip to the next line
  }

  if(!f.eof()) {		// Found it
    // Skip to the next line
    while(!f.eof() && f.get() != '\n');
    rpos = f.tellg();
  } else  {			// Didn't find it
    f.seekg(0, ios::beg);
    while(f) {
      char c;
      wpos = rpos = f.tellg();
      s = name;
      while(*s && tolower((c = f.get())) == tolower(*s++));
      // See whether we should be before this in alphabetical order
      if(*s == '\0' || tolower(c) > tolower(s[-1]))
	break;
    }
    if(f.eof()) {		// Need to add at the end of the file
      f.put('\n');
      wpos = rpos = f.tellp();
    }
  }

  // Now read in the first buffer-ful after the insertion point, so
  // we don't lose it.
  f.seekg(rpos);
  f.get(buffer, sizeof buffer);
  rpos = f.tellg();

  // Write a line containing the method to be saved

  // Oh sod it, this is too complicated and nasty.
  // Who wants to be able to write to MicroSIRIL libraries
  // anyway?  They're designed to be written by humans
  // and read by computers.
}

#endif

RINGING_END_NAMESPACE

