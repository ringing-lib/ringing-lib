// -*- C++ -*- expr_base.h - Expression and statement interfaces
// Copyright (C) 2002, 2003, 2004, 2005, 2011
// Richard Smith <richard@ex-parrot.com>

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

#ifndef GSIRIL_EXPR_BASE_INCLUDED
#define GSIRIL_EXPR_BASE_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
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
    virtual bool is_definition() const { return false; }
  };

  statement( impl* pimpl = 0 ) : pimpl(pimpl) {}

  void execute( execution_context& e ) const { pimpl->execute(e); }
  bool eof() const { return !pimpl; } 
  bool is_definition() const { return pimpl && pimpl->is_definition(); }

private:
  shared_pointer< impl > pimpl;
};

// A node in of an expression
class expression
{
public:
  enum type_t {
    boolean,
    no_type
  };

  class node
  {
  public:
    virtual ~node() {}
    virtual void debug_print( ostream &os ) const = 0;
    virtual void execute( proof_context &ctx ) = 0;
    virtual bool evaluate( proof_context &ctx ); // throws
    virtual bool isnop() const { return false; }
    virtual type_t type() const { return no_type; }
  };

  class bnode : public node
  {
  public:
    virtual void execute( proof_context &ctx ) { evaluate(ctx); }
    virtual bool evaluate( proof_context &ctx ) = 0;
    virtual type_t type() const { return boolean; }
  };

  // Create an expression handle
  expression( node* impl ) : impl(impl) {}

  bool isnull() const { return !impl; }
  bool isnop() const { return !impl || impl->isnop(); }
  type_t type() const { return impl ? impl->type() : no_type; }


  void debug_print( ostream &os ) const    { impl->debug_print(os); }

  // execute an expression, possibly adding to the current proof
  void execute( proof_context &ctx ) const { impl->execute(ctx); }

  // Evaluate a const expression in boolean context.
  // If evaluation requires execution of an expression, a silent clone
  // of the proof_context is made and discarded at the end of the 
  // evaluation.
  bool evaluate( proof_context& ctx ) const { return impl->evaluate(ctx); }


  RINGING_FAKE_DEFAULT_CONSTRUCTOR(expression);

private:
  shared_pointer< node > impl;
};

// An exception thrown when executing a string literal containing a $$.
struct script_exception
{
  enum type {
    do_abort,
    do_stop,
    do_break
  };
  
  script_exception( type t ) : t(t) {}

  type t;
};


#endif // GSIRIL_EXPR_BASE_INCLUDED
