// -*- C++ -*- tokeniser.h - Tokenise lines of input into statements
// Copyright (C) 2003, 2011 Richard Smith <richard@ex-parrot.com>

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

#ifndef GSIRIL_TOKENISER_INCLUDED
#define GSIRIL_TOKENISER_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_HAVE_OLD_IOSTREAMS
#include <istream.h>
#else
#include <iosfwd>
#endif
#include <string>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <iterator.h>
#else
#include <vector>
#include <iterator>
#endif

RINGING_USING_NAMESPACE

class token : public string
{
public:
  token(const string& s = string(), int t=0) : string(s), t(t) {}
  
  // Use of type:
  //   'A' is an identifier (or id_first_chars[0])
  //   '0' is a number
  //   other 1..255 are single characters
  //   > 255 are defined by supertypes of quoted_token
  int type() const { return t;}
  void type(int tt) { t = tt; }
  
private:
  int t;
};



class tokeniser
{
public:
  virtual ~tokeniser() {}

  class const_iterator;
  const_iterator begin() const;
  const_iterator end() const;
 
  int line() const { return lineno; }

  enum { first_token = 256 };

  class basic_token;
  class string_token;

protected:
  enum new_line_policy { keep_new_lines, discard_new_lines };
  enum case_policy { case_sensitive, case_insensitive };

  tokeniser( istream& in, 
	     new_line_policy nlp = discard_new_lines,
	     case_policy cp = case_sensitive );

  void add_qtype( const basic_token* qtype );
  void set_id_chars( const char *idfchs, const char *idochs );

private:
  virtual void validate( const token& ) const;

  friend class const_iterator;
  bool parse( token& tok );

  new_line_policy nlp;
  case_policy cp;
  vector< const basic_token* > qtypes;  // Not owned
  const char *id_first_chars, *id_other_chars;

  string buffer;
  string::const_iterator i, e;
  istream *in;
  int lineno;
};


class tokeniser::const_iterator
{
public:
  typedef input_iterator_tag iterator_category;
  typedef token              value_type;
  typedef const token&       reference;
  typedef const token*       pointer;
  typedef ptrdiff_t          difference_type;
  
  const value_type& operator* () const { return  val; }
  const value_type* operator->() const { return &val; }
  
  const_iterator() : tok(NULL) {}
  
  const_iterator& operator++() 
    { increment(); return *this; }
  
  const_iterator operator++(int) 
    { const_iterator tmp(*this); ++*this; return tmp; }

  friend bool operator==( const const_iterator& x, const const_iterator& y )
    { return x.tok == y.tok; }

  friend bool operator!=( const const_iterator& x, const const_iterator& y )
    { return x.tok != y.tok; }
    
private:
  void increment();

  friend class tokeniser;
  explicit const_iterator( tokeniser* tok ) : tok(tok) { increment(); }
  
  tokeniser *tok;
  value_type val;
};

inline tokeniser::const_iterator tokeniser::begin() const
{ 
  return const_iterator(const_cast<tokeniser*>(this)); 
}

inline tokeniser::const_iterator tokeniser::end() const
{ 
  return const_iterator();     
}


// Looks for a particular string.  
// Useful for a sequence of characters (e.g. *=) or keywords (e.g. exit).
// Subclass this to provide more advanced behaviour.
class tokeniser::basic_token
{ 
public:
  enum parse_result { done, more, failed };
  virtual parse_result parse( string::const_iterator& i, 
			      string::const_iterator e, 
			      token& tok ) const;
  
  bool matches( string::const_iterator i, string::const_iterator e ) const;
  int type() const { return t; }
  
  basic_token( const char* initial_sequence, int type );

private:
  const char* init_seq;
  int t;
  size_t len;
};

// Parses quoted strings
class tokeniser::string_token : public tokeniser::basic_token
{
public:
  enum multi_line_policy { multi_line, one_line };

  // q is the quote character; it must be one character long only.
  // e.g. q == "'"
  string_token(const char* q, int type,
	       multi_line_policy mlp = multi_line);
  
private:
  virtual parse_result parse( string::const_iterator& i, 
			      string::const_iterator e, 
			      token& tok ) const;
  char q;
  multi_line_policy mlp;
};

#endif // GSIRIL_TOKENISER_INCLUDED
