// expression.cpp - Nodes and factory function for expressions
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <ringing/common.h>
#include "expression.h"
#include "execution_context.h"
#include "util.h"
#include "parser.h"
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE

class list_node : public expression::node
{
public:
  list_node( const shared_pointer< expression::node > &car,
	     const shared_pointer< expression::node > &cdr )
    : car(car), cdr(cdr)
  {
  }

protected:
  virtual void debug_print( ostream &os ) const
  {
    os << "( ";
    car->debug_print( os );
    os << ", ";
    cdr->debug_print( os );
    os << " )";
  }

  virtual void execute( execution_context &ctx )
  {
    car->execute( ctx );
    cdr->execute( ctx );
  }

private:  
  shared_pointer< expression::node > car, cdr;
};

class nop_node : public expression::node
{
protected:
  virtual void debug_print( ostream &os ) const
  {
    os << "{null}";
  }

  virtual void execute( execution_context & )
  {
  }
};

class repeated_node : public expression::node
{
public:
  repeated_node( int count,
		 const shared_pointer< expression::node > &child )
    : count(count), child(child)
  {
  }

protected:
  virtual void debug_print( ostream &os ) const
  {
    os << count << " ";
    child->debug_print( os );
  }

  virtual void execute( execution_context &ctx )
  {
    for (int i=0; i<count; ++i) 
      child->execute( ctx );
  }

private:  
  int count;
  shared_pointer< expression::node > child;
};

class string_node : public expression::node
{
public:
  string_node( const string &str ) 
    : str(str)
  {
  }

  virtual void execute( execution_context &ctx )
  {
    bool do_exit = false;
    ctx.output() << ctx.substitute_string(str, do_exit) << endl;
    if (do_exit)
      throw script_exception();
  }

protected:
  virtual void debug_print( ostream &os ) const
  {
    os << "\"" << str << "\"";
  }

private:
  string str;
};

class pn_node : public expression::node
{
public:
  pn_node( int bells, const string &pn )
  {
    if ( bells == -1 )
      throw runtime_error( "Must set number of bells before using "
			   "place notation" );

    interpret_pn( bells, pn.begin(), pn.end(), back_inserter( changes ) );
  }

protected:
  virtual void debug_print( ostream &os ) const
  {
    copy( changes.begin(), changes.end(),
	  ostream_iterator< change >( os, "." ) );
  }

  virtual void execute( execution_context &ctx )
  {
    for_each( changes.begin(), changes.end(), 
	      ctx.permute_and_prove() );
  }

private:
  vector< change > changes;
};

class transp_node : public expression::node
{
public:
  transp_node( int bells, const string &r )
    : transp(bells)
  {
    if ( bells == -1 )
      throw runtime_error( "Must set number of bells before using "
			   "transpostions" );
    
    transp *= r;
    if ( transp.bells() != bells )
      throw runtime_error( "Transposition is on the wrong number of bells" );
  }

protected:
  virtual void debug_print( ostream &os ) const
  {
    os << "'" << transp << "'";
  }

  virtual void execute( execution_context &ctx )
  {
    ctx.permute_and_prove()( transp );
  }

private:
  row transp;
};

class symbol_node : public expression::node
{
public:
  symbol_node( const string &sym )
    : sym(sym)
  {
  }

protected:
  virtual void debug_print( ostream &os ) const
  {
    os << sym;
  }

  virtual void execute( execution_context &ctx )
  {
    ctx.execute_symbol(sym);
  }

private:
  string sym;
};

shared_pointer< expression::node > 
expression::make_expr( const parser &p,
		       vector< token >::const_iterator first, 
		       vector< token >::const_iterator last )
{
  typedef vector< token >::const_iterator iter_t;
  typedef shared_pointer< expression::node > shared_node_t;

  if ( first == last ) 
    throw runtime_error( "Expression expected" );

  // Parentheses first
  if ( first->first == token_type::open_paren && 
       (last-1)->first == token_type::close_paren )
    {
      int depth = 0;
      bool ok = true;
      for ( iter_t i(first); ok && i != last; ++i )
	{
	  if ( i->first == token_type::open_paren )
	    ++depth;
	  else if ( i->first == token_type::close_paren )
	    --depth;

	  if ( depth == 0 && i != last-1 ) 
	    ok = false;
	}

      if (ok && depth) 
	throw runtime_error( "Unmatched parentheses 1" );

      if (ok)
	return make_expr( p, first+1, last-1 );
    }

  // Comma is the lowest precedence operator
  // It is left associative
  {
    int depth = 0;
    for ( iter_t i(first); i != last; ++i )
      {
	if ( i->first == token_type::open_paren )
	  ++depth;
	else if ( i->first == token_type::close_paren )
	  --depth;
	else if ( i->first == token_type::comma && depth == 0 )
	  return shared_node_t
	    ( new list_node( make_expr( p, first, i ),
			     make_expr( p, i+1, last ) ) );
      }

    if (depth) 
      throw runtime_error( "Unmatched parentheses" );
  }

  // A number literal in a repeated block is the
  // only remaining construct that is not a single token.
  if ( first->first == token_type::num_lit )
    {
      if ( first+1 != last && (first+1)->first == token_type::times )
	return shared_node_t
	  ( new repeated_node( string_to_int( first->second ),
			       make_expr( p, first+2, last ) ) );
      else
	return shared_node_t
	  ( new repeated_node( string_to_int( first->second ),
			       make_expr( p, first+1, last ) ) );
    }

  // Everything left is a literal of some sort
  if ( last - first != 1 )
    throw runtime_error( "Parse error" );

  switch ( first->first )
    {
    case token_type::string_lit:
      return shared_node_t( new string_node( first->second ) );

    case token_type::name:
      return shared_node_t( new symbol_node( first->second ) );

    case token_type::pn_lit:
      return shared_node_t( new pn_node( p.bells(), first->second ) );

    case token_type::transp_lit:
      return shared_node_t( new transp_node( p.bells(), first->second ) );

    default:
      throw runtime_error( "Unknown token in input" );
      return shared_node_t(); // To keep MSVC 5 happy
    }
}

expression::expression( const parser &p, 
			const vector< token > &tokens )
  : impl( tokens.empty() 
	  ? shared_pointer< expression::node >( new nop_node )
	  : make_expr( p, tokens.begin(), tokens.end() ) )
{
}
