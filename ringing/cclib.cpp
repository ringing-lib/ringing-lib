// cclib.cpp - Read and write the Central Council Method libraries
// Copyright (C) 2001 Mark Banner <mark@standard8.co.uk>

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
#include <ringing/cclib.h>
#if RINGING_OLD_C_INCLUDES
#include <string.h>
#include <stdio.h>
#else
#include <cstring>
#include <cstdio>
#endif
#include <ringing/method.h>
#include <string>
#if defined(SEPERATE_FILES)
// Be warned that dirent.h is not in the either the C99 or C++98 standards.
// Also indentifiers containing two adjacent underscores or one leading one
// followed by an uppercaes letter are prohibited.
#include <dirent.h>
#endif
RINGING_START_NAMESPACE


class cclib::iterator
{
public:
  // Standard iterator typedefs
  class value_type;
  typedef input_iterator_tag iterator_category;
  typedef ptrdiff_t difference_type;
  typedef value_type &reference;
  typedef value_type *pointer;

  // Construction
  iterator( ifstream &f ) : ifs(&f) { ok = val.readentry( *ifs ); }
  iterator() : ifs(0), ok(false) {}

  // Equality Comparable requirements
  bool operator==( const iterator &i ) const
    { return ok ? (i.ok && ifs == i.ifs) : !i.ok; }
  bool operator!=( const iterator &i ) const
    { return !operator==( i ); }

  // Trivial Iterator requirements
  reference operator*() { return val; }
  pointer operator->() { return &val; }

  // Input Iterator requirements
  iterator &operator++() { ok = val.readentry( *ifs ); return *this; }
  iterator operator++(int) { iterator tmp(*this); ++*this; return tmp; }

  // A class representing an entry in the collection
  class value_type
  {
  public:
    // The name of the method
    string name() const;

    // The place notation of the method
    string pn() const;

  private:
    // Helper functions
    friend class cclib::iterator;
    value_type();
    void parse_header();
    bool readentry( ifstream &ifs );

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
  };

private:
  // Data members
  ifstream *ifs;
  value_type val;
  bool ok;
};

cclib::iterator cclib::begin() 
{
  // return file to the start.
  f.clear();
  f.seekg(0, ios::beg);
  return iterator(f);
}

cclib::iterator cclib::end()
{
  return iterator();
}

cclib::iterator::value_type::value_type()
  : meth_name_starts( string::npos ),
    meth_name_ends  ( string::npos ),
    meth_hl         ( string::npos ),
    meth_le         ( string::npos ),
    meth_lh         ( string::npos )
{}

bool cclib::iterator::value_type::readentry( ifstream &ifs )
{
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
      else if (meth_name_starts != meth_name_ends)
	{
	  // Even wrapped lines are always longer than meth_name_ends-3
	  if ( linebuf.length() > meth_name_ends-3
	       // Check first bit equates to a number...
	       && atoi( string(linebuf, 0, meth_name_starts).c_str() ) )
	    {
	      bool is_wrapped(true);

	      // Unfortunately, I can't see a non-heuristical way
	      // of doing this.  
	      {
		string wrap( linebuf, meth_name_ends - 4, string::npos );
		
		for ( string::const_iterator i(wrap.begin()), e(wrap.end()-1); 
		      i < e; ++i )
		  {
		    if ( // Does it have multiple consecutive spaces?
			 *i == ' ' && i[1] == ' ' 
			 // Or does it contain characters that are not
			 // from [A-Za-z\']?
			 || *i != ' ' && !isalnum(*i) && *i != '\'' )
		      { 
			is_wrapped = false; 
			break; 
		      }
		  }
	      }

	      if ( is_wrapped )
		{
		  wrapped_name = linebuf;
		  getline( ifs, linebuf );
		}

	      if ( linebuf.length() > meth_lh )
		break;
	    }
	}
    }
  return ifs;
}

void cclib::iterator::value_type::parse_header()
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
}

string cclib::iterator::value_type::name() const
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

string cclib::iterator::value_type::pn() const
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
				     meth_lh - meth_name_ends ) );
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


newlib<cclib> cclib::type;

// This function is for creating lower case strings.
void lowercase(char &c)
{
  c = tolower(c);
}

int cclib::extractNumber(const string &filename)
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

string cclib::simple_name(const string &original)
{
  string newname(original);
  string::size_type pos = 0;

  // Erase sections of the string that contain a classname.
  for (int i = 2; i < 11; i++)
    {
      if ((pos = newname.find(method::classname(i))) != string::npos)
	{
	  newname.erase(pos, strlen(method::classname(i)));
	}
    }
  // Do the same for little - but only find the last
  if ((pos = newname.rfind(method::txt_little)) != string::npos)
    {
      newname.erase(pos, strlen(method::txt_little));
    }

  // Now remove space on end of line.
  string::const_iterator j = newname.end();
  while ((j >= newname.begin()) && (isspace(*(j - 1))))
    {
      j--;
    }

  return newname.substr(0, j - newname.begin());
}

cclib::cclib(const string& name) : f(name.c_str()), wr(0), _good(0)
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
int cclib::canread(ifstream& ifs)
{
  // Rewind the stream
  ifs.clear();
  ifs.seekg(0, ios::beg);

  return iterator(ifs) != iterator();
}

// Return a list of items
int cclib::dir(list<string>& result)
{
  if (_good != 1)
    return 0;

  for ( iterator i(begin()); i != end(); ++i )
    result.push_back( i->name() );

  return result.size();
}

// Load a method from a Central Council Method library
method cclib::load(const string& name)
{
  string methname(name);

  for ( iterator i(begin()); i != end(); ++i )
    {
      // Extract the method name section
      string wordbuf( i->name() );
      
      // Copy wordbuf to preserve for later
      string methodname(wordbuf);
      
      // Make all letters lower case
      for_each(wordbuf.begin(), wordbuf.end(), lowercase);
      for_each(methname.begin(), methname.end(), lowercase);
	    
      // Do we have the correct line for the method?
      if ((wordbuf.length() == methname.length()) &&
	  (wordbuf.compare(methname) == 0))
	{
	  // we have found the method.
	  return method( i->pn(), b,
			 simple_name( methodname ) );
	}
    }

  // If we are here we couldn't find the method.

#if RINGING_USE_EXCEPTIONS
  // If we are using exceptions, throw one to notify it couldn't be found.
  throw invalid_name();
#endif 
  // Visual Studio 5 requires a return statement even after a throw.

  // Otherwise we have to return something to avoid warning and errors, so
  // make up something strange. Give it a name so it can always be checked
  // against.
  method m;
  return m;
}

#if defined(SEPERATE_FILES)
// This function is designed to seperate the cc method collection files into
// seperate ones - they have a nasty habit of bundling them together which
// makes them impossible to search through easily. Especially if you are only
// specifying a name not a number of bells.
// Returns 0 = successful, 1 = no modifications required, -1 = unsuccessful
int cclib::seperatefiles(const string &dirname)
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
		  int bells = extractNumber(direntryname);
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
