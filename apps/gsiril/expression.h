// -*- C++ -*-

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
    string_lit,
    pn_lit,
    num_lit,
    name
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
