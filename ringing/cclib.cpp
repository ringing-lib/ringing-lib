// cclib.cpp - Read and write the Central Council Method libraries
// Copyright (C) 2001, 2002, 2003, 2004 Mark Banner <mark@standard8.co.uk>
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

#include <ringing/cclib.h>
#if RINGING_OLD_INCLUDES
#include <algo.h>
#include <fstream.h>
#else
#include <algorithm>
#include <fstream>
#endif
#if RINGING_OLD_C_INCLUDES
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#else
#include <cstring>
#include <cstdio>
#include <cassert>
#include <cctype>
#endif
#include <ringing/method.h>
#include <ringing/pointers.h>
#include <ringing/peal.h>
#include <string>
#if defined(SEPERATE_FILES)
// Be warned that dirent.h is not in the either the C99 or C++98 standards.
#include <dirent.h>
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

RINGING_DEFINE_LIBRARY_FACET( cclib::ref );

class cclib::impl : public library_base {
private:
  ifstream f;                   // The file stream we're using
  int b;                        // Number of bells for files in this lib
  int wr;                       // Is it open for writing?
  int _good;                    // If we have a good filename or not.

  // Extracts the number of bells from the filename.
  static int extract_number(const string&);

public:
#if defined(SEPERATE_FILES)
  static int seperatefiles(const string&);
#endif

  // Is this file in the right format?
  static library_base *canread(const string& filename);

private:
  // Construction handled by library class
  impl(const string& filename);
 ~impl() { if (_good == 1) f.close(); }

  // Iterators into the library
  class entry_type;
  friend class entry_type;
  virtual const_iterator begin() const;

  // Is the library in a usable state?
  virtual bool good(void) const { return _good; }

  // Is this library writeable?
  virtual bool writeable(void) const { return wr; }
};

void cclib::registerlib(void) 
{
  library::addtype(&impl::canread);
}


// A class representing an entry in the collection
class cclib::impl::entry_type : public library_entry::impl
{
  // The public interface
  virtual string name() const;
  virtual string base_name() const;
  virtual string pn() const;
  virtual int bells() const { return b; }

  virtual bool has_facet( const library_facet_id& id ) const;

  virtual shared_pointer< library_facet_base >
    get_facet( const library_facet_id& id ) const;

  // Helper functions
  friend class cclib::impl;
  entry_type();
  virtual ~entry_type() { };
  void parse_header();
  void parse_title();
  peal read_peal( string::size_type date, string::size_type place,
		  string::size_type end ) const;
  peal::date read_date( string::size_type start, string::size_type end ) const;
  static bool maybe_strip_class( string& name, const string& clname );

  virtual bool readentry( library_base &lb );
  virtual library_entry::impl *clone() const { return new entry_type(*this); }


  // The current line
  string linebuf;

  // The previous line sometimes contains just the name
  // when it is too long (e.g. "A Fishmonger and Judith's Hat
  // Surprise Minor").
  string wrapped_name;

  // Offsets for the start/end of various columns
  string::size_type meth_name_starts;
  string::size_type meth_name_ends;
  string::size_type meth_hl;
  string::size_type meth_le;
  string::size_type meth_lh;

  string::size_type tower_date;
  string::size_type tower_place;
  string::size_type tower_end;

  string::size_type hand_date;
  string::size_type hand_end;

  string::size_type rw_start;
  string::size_type rw_end;

  // Method class & stage information
  int b;
};


library_base::const_iterator cclib::impl::begin() const
{
  ifstream *ifs = const_cast< ifstream * >( &f );
  ifs->clear();
  ifs->seekg(0, ios::beg);
  return const_iterator(const_cast< cclib::impl * >(this), 
			new cclib::impl::entry_type);
}
 
cclib::impl::entry_type::entry_type()
  : meth_name_starts( string::npos ),
    meth_name_ends  ( string::npos ),
    meth_hl         ( string::npos ),
    meth_le         ( string::npos ),
    meth_lh         ( string::npos ),
    tower_date      ( string::npos ),
    tower_place     ( string::npos ),
    tower_end       ( string::npos ),
    hand_date       ( string::npos ),
    hand_end        ( string::npos ),
    rw_start        ( string::npos ),
    rw_end          ( string::npos ),
    b               ( 0            )
{}

bool cclib::impl::entry_type::readentry( library_base &lb )
{
  ifstream &ifs = dynamic_cast<cclib::impl&>(lb).f;

  // Go through a line at a time.
  while ( ifs )
    {
      wrapped_name = "";
      getline( ifs, linebuf );
      
      // The second check for No. is used as an extra insurance check...
      if ( linebuf.find("Name") != string::npos
	   && linebuf.find("No.") != string::npos )
	{
	  parse_header();
	}
      else if (meth_name_starts != meth_name_ends
	       // Even wrapped lines are always longer than meth_name_ends-3
	       && linebuf.length() > meth_name_ends-3
	       // Check first bit equates to a number...
	       && atoi( string(linebuf, 0, meth_name_starts).c_str() ) )
	{
	  bool is_wrapped(true);
	  
	  {

	    if(linebuf.length() > meth_name_ends
	       && linebuf[meth_name_ends - 2] == ' '
	       && linebuf[meth_name_ends - 1] == ' ')
	      is_wrapped = false;
	    
	    if ( is_wrapped )
	      {
		wrapped_name = linebuf;
		getline( ifs, linebuf );
	      }
	    
	    if ( linebuf.length() > meth_lh )
	      break;
	  }
	}
      else if ( linebuf.find( "methods" ) != string::npos ||
		linebuf.find( "principles" ) != string::npos ||
		linebuf.find( "differentials" ) != string::npos )
	{
	  parse_title();
	}
    }
  return ifs;
}

void cclib::impl::entry_type::parse_title()
{
  // True if it's a method, false if it's a principle
  bool meth( linebuf.find( "methods" ) != string::npos );
  bool prin( linebuf.find( "principles" ) != string::npos );

  for ( int i=3; i<=22; ++i )
    if ( linebuf.find( string( method::stagename(i) ) + 
		       string( meth ? " methods" : 
			       prin ? " principles" : " differentials" ) )
	 != string::npos )
      {
	b = i;
	break;
      }
}

peal::date cclib::impl::entry_type::read_date( string::size_type start,
					       string::size_type end ) const
{
  peal::date dt;

  string::size_type i(start);
  dt.day = atoi( &linebuf[i] );

  i = linebuf.find('.', i);
  if ( i >= end ) return peal::date();
  dt.month = atoi( &linebuf[++i] );
    
  i = linebuf.find('.', i);
  if ( i >= end ) return peal::date();
  dt.year = atoi( &linebuf[++i] );
  
  if ( !dt.day || !dt.month || !dt.year )
    dt.day = dt.month = dt.year = 0;
  return dt;
}

peal cclib::impl::entry_type::read_peal( string::size_type date, 
					 string::size_type place,
					 string::size_type end ) const
{
  if ( date == string::npos || date > linebuf.size() ) 
    return peal();

  if ( end > linebuf.size() ) end = linebuf.size();
  if ( place > linebuf.size() ) place = string::npos;

  peal::date dt( read_date( date, place == string::npos ? end : place ) );

  string loc;
  if ( place != string::npos ) {
     // Remove trailing whitespace
    while ( end > place && isspace( linebuf[end-1] ) )
      --end;

    loc.assign( linebuf, place, end - place );
  }

  return peal( dt, loc );
}

void cclib::impl::entry_type::parse_header()
{
  meth_name_starts = linebuf.find("Name");
  meth_name_ends = linebuf.find("Notation");

  meth_hl = linebuf.find("hl");
  
  // If we have a half lead then take note accordingly.
  if (meth_hl != string::npos)
    {
      meth_le = linebuf.find("le");
      meth_lh = linebuf.find("lh");
    }
  else
    {
      meth_le = string::npos;
      meth_lh = linebuf.find("lh");
    }

  tower_date  = linebuf.find("Tower bells");

  if ( tower_date != string::npos ) {
    tower_place = linebuf.find("Place");
    tower_end = linebuf.find_first_not_of( " \t\r\n", 
      tower_place != string::npos ? tower_place + 5 : tower_date + 11 );
  } else {
    tower_place = string::npos;
    tower_end = string::npos;
  }

  hand_date = linebuf.find("Handbells");

  if ( hand_date != string::npos )
    hand_end = linebuf.find_first_not_of( " \t\r\n", hand_date + 9 );
  else 
    hand_end = string::npos;

  rw_start = linebuf.find("RW ref");
  if ( rw_start != string::npos )
    rw_end = linebuf.find_first_not_of( " \t\r\n", rw_start + 6 );
  else
    rw_end = rw_end;
}

string cclib::impl::entry_type::name() const
{
  string n( linebuf, meth_name_starts, 
	    meth_name_ends - meth_name_starts ); 

  if ( !wrapped_name.empty() )
    n = wrapped_name.substr( meth_name_starts, string::npos );

  // Remove whitespace from end of method name
  string::const_iterator i = n.end();
  while ( i > n.begin() && isspace( i[-1] ) )
    --i;

  return n.substr( 0, i - n.begin() );
}

bool cclib::impl::entry_type::has_facet( const library_facet_id& id ) const
{
  if ( id == cclib::ref::id )
    return true;

  else if ( id == first_tower_peal::id )
    return tower_date != string::npos && bool( get_facet( id ) );
  
  else if ( id == first_hand_peal::id ) 
    return tower_end != string::npos && bool( get_facet( id ) );

  else if ( id == rw_ref::id ) 
    return rw_start != string::npos && bool( get_facet( id ) );

  else
    return false;
}

shared_pointer< library_facet_base >
cclib::impl::entry_type::get_facet( const library_facet_id& id ) const
{
  shared_pointer< library_facet_base > result;

  if ( id == cclib::ref::id ) 
    result.reset( new cclib::ref( atoi( linebuf.c_str() ) ) );

  else if ( id == first_tower_peal::id ) {
    peal p( read_peal( tower_date, tower_place, tower_end ) );
    if ( p.when().day || p.where().size() )
      result.reset( new first_tower_peal( p ) );
  }

  else if ( id == first_hand_peal::id ) {
    peal p( read_peal( hand_date, string::npos, hand_end ) );
    if ( p.when().day || p.where().size() )
      result.reset( new first_hand_peal( p ) );
  }

  else if ( id == rw_ref::id ) {
    if ( rw_start != string::npos && rw_start < linebuf.size() ) {
      string::size_type end( rw_end );
      if ( end > linebuf.size() ) end = linebuf.size();
      while ( end > rw_start && isspace( linebuf[end-1] ) )
	--end;
      if ( end != rw_start )
	result.reset( new rw_ref( linebuf.substr( rw_start, end-rw_start ) ) );
    }
  }

  return result;
}

bool 
cclib::impl::entry_type::maybe_strip_class( string& name, 
					    const string& clname )
{
  string::size_type pos = name.rfind(clname);
  if ( pos != string::npos && pos + clname.size() == name.size() ) {
    while (pos>0 && isspace(name[pos-1])) --pos;
    name.erase( pos, string::npos );
    return true;
  }
  return false;
}

string cclib::impl::entry_type::base_name() const
{
  string newname(name());

  // Entries in CC Libs do not contain the stage and only contain 
  // "selected" classes.  "The selected classes are Bob, Little, Place 
  // and Slow Course."  It also appears that for Differential Hunters 
  // (but not Differentials) have "Differential" in their name.

  maybe_strip_class( newname, method::classname( method::M_BOB          ) ) ||
  maybe_strip_class( newname, method::classname( method::M_PLACE        ) ) ||
  maybe_strip_class( newname, method::classname( method::M_SLOW_COURSE  ) );

  maybe_strip_class( newname, method::classname( method::M_DIFFERENTIAL ) );
  maybe_strip_class( newname, method::classname( method::M_LITTLE       ) );

  return newname;
}

string cclib::impl::entry_type::pn() const
{
  string pn;

  // Get place notation
  if ( meth_hl != string::npos && meth_le != string::npos )
    {
      // We have a reflection
      pn.append("&");
      // Add place notation
      pn.append(linebuf.substr(meth_name_ends, meth_hl - meth_name_ends));
      // Add half lead notation
      pn.append(linebuf.substr(meth_hl, meth_le - meth_hl));
      // And the lead head change
      pn.append(",");
      pn.append(linebuf.substr(meth_le, meth_lh - meth_le));
    }

  else if (meth_hl != string::npos)
    {
      // This is for methods like Grandsire which the CC
      // have entered in an awkward way.

      // As an example, for Grandsire Doubles we return "3,&1.5.1.5.1".
      // This is not a very standard form, but it works fine in our place 
      // notation handling code.
      switch ( linebuf[meth_name_ends] )
	{
	case 'X': case 'x': case '-':
	  pn.append( "-,&" );
	  pn.append( linebuf.substr( meth_name_ends+1, 
				     meth_lh - meth_name_ends - 1 ) );
	  break;

	default:
	  {
	    string::const_iterator i( linebuf.begin() + meth_name_ends );
	    string::const_iterator j(i), e( linebuf.begin() + meth_lh );

	    while ( j < e && isalnum(*j) && *j != 'X' && *j != 'x') 
	      ++j;

	    pn.append( i, j );
	    pn.append( ",&" );
	    pn.append( j, e );
	  }
	}
    }

  else
    {
      // This is for the non-reflecting irregular methods.
      pn.append( linebuf.substr(meth_name_ends, meth_lh - meth_name_ends) );
    }

  return pn;
}

// ---------------------------------------------------------------------


int cclib::impl::extract_number(const string &filename)
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

cclib::impl::impl(const string& filename)
  : f(filename.c_str()), wr(0), _good(0)
{
  // Open file. Not going to bother to see if it's writeable as the
  // save function is not currently planned to be implemented.
  if(f.good())
    {
      _good = 1;
      b = extract_number(filename);
    }
}

// Is this file in the right format?
library_base *cclib::impl::canread(const string& filename)
{
  scoped_pointer<library_base> ptr( new cclib::impl(filename) );
  if ( ptr->begin() != ptr->end() )
    return ptr.release();
  else
    return NULL;
}

#if defined(SEPERATE_FILES)
// This function is designed to seperate the cc method collection files into
// seperate ones - they have a nasty habit of bundling them together which
// makes them impossible to search through easily. Especially if you are only
// specifying a name not a number of bells.
// Returns 0 = successful, 1 = no modifications required, -1 = unsuccessful
int cclib::impl::seperatefiles(const string &dirname)
{
  // We shall assume an extension of *.txt for ccfiles
  // First find all *.txt files in the directory
  DIR *LibDir = opendir(dirname.c_str());
  struct dirent *direntry;
  if (LibDir != NULL)
    {
      int result = -1;

      while ((direntry = readdir (LibDir)) != NULL)
	{
	  string direntryname = direntry->d_name;
	  // Is it a valid cclib file?
	  ifstream ifs((dirname + direntryname).c_str(), ios::in);
	  if (ifs.good())
	    {
	      if (canread(ifs))
		{
		  if (result != 0)
		    result = 1;
		  // This is a valid cclib file - Now check to see if it needs
		  // seperating.
		  int bells = extract_number(direntryname);
		  if (bells != 0)
		    {
		      // Look through file for modifications to be made.
		      ifs.clear();
		      ifs.seekg(0, ios::beg);
		      bool changerequired = false;
		      while ((ifs.good()) && (!changerequired))
			{
			  string l;
			  getline(ifs, l);
			  int i;
			  for (i = 3; i < 23; i++)
			    {
			      if (l.find(method::stagename(i)) != string::npos)
				{
				  if (i != bells)
				    {
				      changerequired = true;
				    }
				}
			    }
			} // end while
		      
		      if (changerequired)
			{
			  result = 0;
			  // Need to extract the file data
			  // Reset the file pointers
			  ifs.clear();
			  ifs.seekg(0, ios::beg);

			  ofstream *f_PTR;

			  // Now open a tmp file for the original data
			  ofstream ofstemp((dirname + direntryname + ".tmp").c_str(), ios::out);
			  ofstream ofsnew;

			  f_PTR = &ofstemp;
			  bool firstl = true;
			  string firstline;
			  while (ifs.good())
			    {
			      string l;
			      getline(ifs, l);
			      if (firstl)
				{
				  // Store the first line for new files.
				  firstline = l;
				  firstl = false;
				}

			      int i;
			      bool isstagedetails = false;
			      // Don't need to do 4 - assume this is the min
			      // file the CC will put together
			      for (i = 5; i < 23; i++)
				{
				  if ((l.size() < firstline.size()) && (l.find(method::stagename(i)) != string::npos))
				    {
				      isstagedetails = true;
				      if (i != bells)
					{
					  // change file
					  (*f_PTR).close();

					  //					  cout << bells << endl;
					  //					  cout << i << endl;
					  char bstr[3];
					  char istr[3];
					  sprintf(&bstr[0], "%d", bells);
					  sprintf(&istr[0], "%d", i);
					  // cout << bstr << "a" << endl;
					  // cout << istr << "A" << endl;
					  // new file name
					  string fnewname = direntryname;
					  int nopos = fnewname.find((string) bstr, 0);
					  fnewname.replace(nopos, ((string) bstr).size(), istr);
					  ofsnew.open((dirname + fnewname).c_str(), ios::out);
					  //cout << direntryname << endl;
					  //cout << fnewname << endl;
					  f_PTR = &ofsnew;
					  *f_PTR << firstline;
					}
				      *f_PTR << l << endl;
				    }
				}
			      if (!isstagedetails)
				*f_PTR << l << endl;
			    } // end while

			  // Now move the temp file over the old one.
			  rename((char*) (dirname + direntryname + ".tmp").c_str(), (char*) (dirname + direntryname).c_str());			  
			  (*f_PTR).close();
			}
		    }
		} // Matches if canread()
	      else
		{
		  ifs.close();
		}
	    }
	}
      closedir(LibDir);
      return result;
    }
  else
    {
      return -1;
    }
}
#endif

RINGING_END_NAMESPACE
