// mslib.cpp - Read and write MicroSIRIL libraries
// Copyright (C) 2001, 2002, 2017 Martin Bright <martin@boojum.org.uk>
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/mslib.h>
#if RINGING_OLD_C_INCLUDES
#include <string.h>
#include <ctype.h>
#else
#include <cstring>
#include <cctype>
#endif
#if RINGING_OLD_INCLUDES
#include <algo.h>
#include <fstream.h>
#else
#include <algorithm>
#include <fstream>
#endif
#include <string>
#include <ringing/pointers.h>

RINGING_START_NAMESPACE

RINGING_USING_STD

// mslib : Implement MicroSIRIL libraries
class mslib::impl : public library_base {
private:
  ifstream f;                   // The file stream we're using
  int b;                        // Number of bells for files in this lib
  int wr;                       // Is it open for writing?
  int _good;

private:
  // Construction handled by library class
  impl(const string& filename);
 ~impl() { if (_good == 1) f.close(); }
  friend class mslib;

public:
  // Is this file in the right format?
  static library_base *canread(const string& filename);

  // Needs to be public because of a compiler bug in MSVC 6.0
  class entry_type;
  friend class entry_type;

private:
  // Iterators into the library
  virtual const_iterator begin() const;

  // Is the library in a usable state?
  virtual bool good(void) const { return _good; }

  // Is this library writeable?
  virtual bool writeable(void) const { return wr; }
};

void mslib::registerlib()
{
  library::addtype(&impl::canread);
}

mslib::mslib(const string& filename)
  : library( new impl(filename) ) {}

class mslib::impl::entry_type : public library_entry::impl
{
  // The public interface
  virtual string name() const;
  virtual string base_name() const;
  virtual string pn() const;
  virtual int bells() const { return b; }

  // Helper functions
  friend class mslib::impl;
  entry_type();
  virtual ~entry_type() { };
  virtual bool readentry( library_base &lb );
  virtual library_entry::impl *clone() const { return new entry_type(*this); }

  // The current line
  string linebuf;

  // The number of bells
  int b;
};


library::const_iterator mslib::impl::begin() const
{
  ifstream *ifs = const_cast< ifstream * >( &f );
  ifs->clear();
  ifs->seekg(0, ios::beg);
  return const_iterator( const_cast< mslib::impl * >(this), 
			 new mslib::impl::entry_type );
}

mslib::impl::entry_type::entry_type()
  : b(0)
{}

bool mslib::impl::entry_type::readentry( library_base &lb )
{
  ifstream &ifs = dynamic_cast<mslib::impl&>(lb).f;

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

  return bool(ifs);
}

string mslib::impl::entry_type::name() const
{
  return string( linebuf, 0, linebuf.find(' ') );
}

string mslib::impl::entry_type::base_name() const
{
  // TODO  Strip trailing 'place' or 'bob' off plain methods
  return name();
}

string mslib::impl::entry_type::pn() const
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


mslib::impl::impl(const string& filename) 
  : f(filename.c_str()), wr(0), _good(0)
{
  // Open file. Not going to bother to see if it's writeable as the
  // save function is not currently planned to be implemented.
  if(f.good())
    {
      _good = 1;
      b = extractNumber(filename);
    }
}

// Is this file in the right format?
library_base *mslib::impl::canread(const string& filename)
{
  scoped_pointer<library_base> ptr( new impl(filename) );
  if ( ptr->begin() != ptr->end() )
    return ptr.release();
  else
    return NULL;
}

RINGING_END_NAMESPACE

