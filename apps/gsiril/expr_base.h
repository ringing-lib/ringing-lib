// -*- C++ -*- expr_base.h - Expression and statement interfaces
// Copyright (C) 2002, 2003, 2004, 2005, 2011, 2012, 2019, 2020, 2021, 2022
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

#ifndef GSIRIL_EXPR_BASE_INCLUDED
#define GSIRIL_EXPR_BASE_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#include <vector>
#include <iosfwd>

#include <ringing/pointers.h>

RINGING_START_NAMESPACE
class change;
class music;
RINGING_END_NAMESPACE

RINGING_USING_NAMESPACE

class proof_context;
class execution_context;
class parser;

// Represents a whole statement (e.g. a definition, proof, ...)
class statement 
{
public:
  class impl {
  public:
    virtual ~impl() {}
    virtual void execute( execution_context& ) = 0;
    virtual bool is_definition() const { return false; }
  };

  statement( impl* pimpl = 0 ) : pimpl(pimpl) {}

  void execute( execution_context& e );
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
    integer,
    transp,
    no_type
  };

  class node {
  public:
    virtual ~node() {}
    virtual void debug_print( ostream &os ) const = 0;
    virtual void execute( proof_context &ctx, int dir ) const = 0;
    virtual bool bool_evaluate( proof_context &ctx ) const;
    virtual RINGING_LLONG int_evaluate( proof_context &ctx ) const;
    virtual string string_evaluate( proof_context &ctx ) const;
    virtual string string_evaluate( proof_context &ctx, bool *no_nl ) const;
    virtual vector<change> pn_evaluate( proof_context &ctx ) const;
    virtual music music_evaluate( proof_context &ctx ) const;
    virtual expression evaluate( proof_context &ctx ) const;
    virtual expression call( proof_context& ctx, 
                             vector<expression> const& args ) const;
    virtual void apply_replacement( proof_context& ctx, 
                                    vector<change>& pn ) const;
    virtual bool isnop() const { return false; }
    virtual type_t type( proof_context &ctx ) const { return no_type; }

  protected:
#   if __cplusplus >= 201103L
    [[noreturn]]
#   endif
    void unable_to( char const* what ) const;
  };

  class bnode : public node {
  public:
    virtual void execute( proof_context &ctx, int dir ) const;
    virtual expression evaluate( proof_context &ctx ) const;
    virtual bool bool_evaluate( proof_context &ctx ) const = 0;
    virtual type_t type( proof_context &ctx ) const { return boolean; }
  };

  class inode : public node {
  public:
    virtual void execute( proof_context &ctx, int dir ) const;
    virtual expression evaluate( proof_context &ctx ) const;
    virtual RINGING_LLONG int_evaluate( proof_context &ctx ) const = 0;
    virtual string string_evaluate( proof_context &ctx ) const;
    virtual type_t type( proof_context &ctx ) const { return integer; }
  };

  class snode : public node {
  public:
    virtual void execute( proof_context &ctx, int dir ) const;
    virtual expression evaluate( proof_context &ctx ) const;
    virtual string string_evaluate( proof_context &ctx ) const = 0;
  };

  // Create an expression handle
  explicit expression( node* impl = 0 ) : impl(impl) {}

  bool isnull() const { return !impl; }
  bool isnop() const { return !impl || impl->isnop(); }
  type_t type( proof_context &ctx ) const 
    { return impl ? impl->type(ctx) : no_type; }

  void debug_print( ostream &os ) const    { impl->debug_print(os); }

  // execute an expression, possibly adding to the current proof
  void execute( proof_context &ctx, int dir ) const;

  // Evaluate a const expression in boolean, integer or stringcontext.
  // If evaluation requires execution of an expression, a silent clone
  // of the proof_context is made and discarded at the end of the evaluation.
  expression evaluate( proof_context& ctx ) const;
  bool bool_evaluate( proof_context& ctx ) const;
  RINGING_LLONG int_evaluate( proof_context& ctx ) const;
  string string_evaluate( proof_context& ctx ) const;
  string string_evaluate( proof_context& ctx, bool* no_nl ) const;
  vector<change> pn_evaluate( proof_context &ctx ) const;
  music music_evaluate( proof_context &ctx ) const;

  expression call( proof_context& ctx, vector<expression> const& args ) const;
  void apply_replacement( proof_context& ctx, vector<change>& m ) const;

  RINGING_FAKE_DEFAULT_CONSTRUCTOR(expression);

private:
  shared_pointer< node > impl;
};

// An exception thrown when executing a string literal containing a $$.
struct script_exception
{
  enum type {
    do_abort,
    do_break
  };
  
  explicit script_exception( type t ) : t(t) {}

  type t;
};


#endif // GSIRIL_EXPR_BASE_INCLUDED
