// -*- C++ -*- expression.h - Expression and statement interfaces
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface "gsiril/expression.h"
#endif

#if RINGING_HAVE_OLD_IOSTREAMS
#include <ostream.h>
#else
#include <iosfwd>
#endif
#include <ringing/pointers.h>

RINGING_USING_NAMESPACE

class proof_context;
class execution_context;
class parser;

// Represents a whole statement (e.g. a definition, proof, ...)
class statement 
{
public:
  class impl
  {
  public:
    virtual ~impl() {}
    virtual void execute( execution_context& ) const = 0;
  };

  statement( impl* pimpl = 0 ) : pimpl(pimpl) {}

  void execute( execution_context& e ) const { pimpl->execute(e); }
  bool eof() const { return !pimpl; } 

private:
  shared_pointer< impl > pimpl;
};

// A node in of an expression
class expression
{
public:
  class node
  {
  public:
    virtual ~node() {}
    virtual void debug_print( ostream &os ) const = 0;
    virtual void execute( proof_context &ctx ) = 0;
    virtual bool isnop() const { return false; }
  };

  // Create an expression handle
  expression( node* impl ) : impl(impl) {}

  bool isnull() const { return !impl; }
  bool isnop() const { return !impl || impl->isnop(); }

  void debug_print( ostream &os ) const    { impl->debug_print(os); }
  void execute( proof_context &ctx ) const { impl->execute(ctx);    }

  RINGING_FAKE_DEFAULT_CONSTRUCTOR(expression);

private:
  shared_pointer< node > impl;
};

// An exception thrown when executing a string literal containing a $$.
class script_exception
{
};

#endif // GSIRIL_EXPRESSION_INCLUDED
