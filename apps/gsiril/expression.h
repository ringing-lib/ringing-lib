// -*- C++ -*- expression.h - Nodes and factory function for expressions
// Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

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

#ifndef GSIRIL_EXPRESSION_INCLUDED
#define GSIRIL_EXPRESSION_INCLUDED

#ifdef __GNUG__
#pragma interface
#endif

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <vector.h>
#include <utility.h>
#else
#include <vector>
#include <utility>
#endif
#include <string>
#if RINGING_HAVE_OLD_IOSTREAMS
#include <ostream.h>
#else
#include <iosfwd>
#endif
#include <ringing/pointers.h>

RINGING_USING_NAMESPACE

class execution_context;
class parser;

struct token_type
{ 
  enum enum_t 
  { 
    open_paren,
    close_paren,
    comma,
    times,
    string_lit,
    pn_lit,
    num_lit,
    name,
    transp_lit
  };
};

typedef pair< token_type::enum_t, string > token;

class expression
{
public:
  expression( const parser &p, const vector< token > &tokens );

  class node
  {
  public:
    virtual ~node() {}
    virtual void debug_print( ostream &os ) const = 0;
    virtual void execute( execution_context &ctx ) = 0;
  };

  void debug_print( ostream &os ) const        { impl->debug_print(os); }
  void execute( execution_context &ctx ) const { impl->execute(ctx);    }

  RINGING_FAKE_DEFAULT_CONSTRUCTOR(expression);

private:
  static shared_pointer< node > 
  make_expr( const parser &p,
	     vector< token >::const_iterator first, 
	     vector< token >::const_iterator last );

  shared_pointer< node > impl;
};

// An exception thrown when executing a string literal containing a $$.
class script_exception
{
};

#endif // GSIRIL_EXPRESSION_INCLUDED
