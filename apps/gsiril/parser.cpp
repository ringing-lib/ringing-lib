// parser.cpp - Tokenise and parse lines of input
// Copyright (C) 2002, 2003, 2004, 2005, 2007, 2008, 2009
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

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include "parser.h"
#include "tokeniser.h"
#include "stringutils.h"
#include "expr_base.h"
#include "expression.h"
#include "statement.h"
#include "prog_args.h"
#if RINGING_OLD_INCLUDES
#include <stdexcept.h>
#include <vector.h>
#else
#include <stdexcept>
#include <vector>
#endif
#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#include <cassert.h>
#else
#include <cctype>
#include <cassert>
#endif
#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE

RINGING_START_ANON_NAMESPACE

//////////////////////////////////////////////////////////

namespace tok_types
{
  enum enum_t 
  { 
    name        = 'A',
    num_lit     = '0',
    open_paren  = '(',
    close_paren = ')',
    open_brace  = '{',
    close_brace = '}',
    colon       = ':',
    comma       = ',',
    times       = '*',
    assignment  = '=',
    new_line    = '\n',
    semicolon   = ';',
    less        = '<',
    greater     = '>',
    plus        = '+',
    minus       = '-',
    divide      = '/',
    modulus     = '%',
    ldivide     = '\\',
    power       = '^',
    bang        = '!',
    comment     = tokeniser::first_token,
    string_lit,
    transp_lit,
    pn_lit,
    regex_lit,
    def_assign, /* ?= */
    logic_and,
    logic_or,
    less_eq,
    greater_eq,
    equals,
    not_equals,
    incr,
    decr
  };
}

class pn_impl : public tokeniser::basic_token
{
public:
  pn_impl(const char *init) : basic_token(init, tok_types::pn_lit) {}
  
private:
  virtual parse_result parse( string::const_iterator& i, 
			      string::const_iterator e, 
			      token& tok ) const
  {
    string::const_iterator j = i;
    if ( *j != '&' && *j != '+' ) return failed; ++j;
    while ( j < e && ( *j == '.' || *j == '-' || isalnum(*j) ) ) ++j;
    if ( j == i+1 ) return failed; // a lone & isn't valid
    tok.assign(i, j); tok.type( tok_types::pn_lit ); i = j;
    return done;
  }
};

class line_comment_impl : public tokeniser::basic_token
{
public:
  line_comment_impl( char const* delim )
    : basic_token( delim, tok_types::comment) {}
  
private:
  virtual parse_result parse( string::const_iterator& i, 
			      string::const_iterator e, 
			      token& tok ) const
  {
    string::const_iterator j = i;
    while ( j < e && *j != '\n' ) ++j;
    tok = string(i, j); tok.type( tok_types::comment ); i = j;
    return done;
  }
};

class mstokeniser : public tokeniser
{
public:
  mstokeniser( istream& in, arguments const& args )
    : tokeniser( in, keep_new_lines, 
		 args.case_insensitive ? case_insensitive : case_sensitive ), 
      args( args ),
      c( args.msiril_syntax ? "/" : "//" ), 
      r( "/",   tok_types::regex_lit,  string_token::one_line ),
      q( "'",   tok_types::transp_lit, string_token::one_line ), 
      qq( "\"", tok_types::string_lit, string_token::one_line ), 
      sym( "&" ), asym( "+" ), defass( "?=", tok_types::def_assign ),
      land( "&&", tok_types::logic_and ), lor( "||", tok_types::logic_or ),
      eq( "==", tok_types::equals ), ne( "!=", tok_types::not_equals ),
      le( "<=", tok_types::less_eq ), ge( ">=", tok_types::greater_eq ),
      inc( "++", tok_types::incr ), dec( "--", tok_types::decr )
  {
    // Note:  It is important that &sym is added after &land; and
    // similarly, that &r is added after &c.  This is because && is a
    // longer, and thus more specialised prefix than &, and similarly
    // for // and /.

    add_qtype(&c);
    if ( !args.msiril_syntax ) add_qtype(&r);
    add_qtype(&q);    add_qtype(&qq);
    add_qtype(&defass); add_qtype(&land); add_qtype(&lor);
    add_qtype(&sym);  add_qtype(&asym);
    add_qtype(&eq);  add_qtype(&ne);  add_qtype(&ge);  add_qtype(&le);
    add_qtype(&inc);  add_qtype(&dec);
  }

  virtual void validate( const token& t ) const
  {
    using namespace tok_types;

    switch ( t.type() ) {
    case name: case num_lit: case open_paren: case close_paren:
    case comma: case times: case assignment: case new_line: case semicolon:
    case comment: case string_lit: case transp_lit: case pn_lit: 
    case def_assign: case logic_and: case logic_or: case greater: case less:
    case equals: case not_equals: case less_eq: case greater_eq:
    case plus: case minus: case divide: case ldivide: case modulus: 
    case power: case bang: case incr: case decr:
      return;

    case regex_lit: case open_brace: case close_brace: case colon:
      // These can only work when msiril comments are disabled
      if ( !args.msiril_syntax ) return;
      break;
    }
     
    throw runtime_error( make_string() << "Unknown token in input: " << t );
  }

private:
  const arguments& args;

  line_comment_impl c;
  string_token r, q, qq;
  pn_impl sym, asym;
  basic_token defass, land, lor, eq, ne, ge, le, inc, dec;
};


//////////////////////////////////////////////////////////

class msparser : public parser
{
public:
  msparser( istream& in, const arguments& args ) 
    : args(args), tok(in, args),
      tokiter(tok.begin()), tokend(tok.end())
  {}

private:
  virtual statement parse();

  int bells() const { return args.bells; }
  void bells(int new_b);

  typedef vector< token >::const_iterator iter_t;

  vector< token > tokenise_command();

  expression make_expr( iter_t first, iter_t last ) const;

  void check_binary_expr( iter_t first, iter_t mid, iter_t last,
                          string const& opname ) const;

  template <class NodeType>
  expression make_binary_expr( iter_t first, iter_t mid, iter_t last,
                               string const& opname ) const;

  expression make_binary_expr( iter_t first, iter_t mid, iter_t last ) const;

  expression make_assign_expr( iter_t first, iter_t mid, iter_t last ) const;

  expression make_alternatives_expr( iter_t first, iter_t last ) const;

  expression make_count_expr( iter_t first, iter_t last, iter_t& split ) const;

  bool find_close_paren( iter_t first, iter_t last,
		         tok_types::enum_t open, tok_types::enum_t close,
		         string const& name,
                         iter_t& result ) const;

  bool is_enclosed( iter_t first, iter_t last,
		    tok_types::enum_t open, tok_types::enum_t close,
		    string const& name ) const;

  bool find_first( iter_t first, iter_t last,
		   tok_types::enum_t tt,
		   iter_t &result ) const;

  template <size_t N>
  bool find_first( iter_t first, iter_t last,
		   tok_types::enum_t (&tt_list)[N],
		   iter_t &result ) const;

  bool find_last( iter_t first, iter_t last,
		  tok_types::enum_t tt,
		  iter_t &result ) const;

  static bool is_unary_op( int t );


  // Data members
  arguments args;
  mstokeniser tok;
  mstokeniser::const_iterator tokiter, tokend;
};

vector< token > msparser::tokenise_command()
{
  vector< token > toks;
  int nesting = 0;

  do 
    {
      // Skip empty commands
      while ( tokiter != tokend && 
	      ( tokiter->type() == tok_types::new_line ||
		tokiter->type() == tok_types::semicolon ||
		tokiter->type() == tok_types::comment ) )
	++tokiter;

      while ( tokiter != tokend && 
	      ( tokiter->type() != tok_types::new_line && 
		tokiter->type() != tok_types::semicolon 
		|| nesting ) )
	{
	  // TODO:  Get the tokeniser to discard comments these automatically
	  if ( tokiter->type() != tok_types::comment &&
	       tokiter->type() != tok_types::new_line ) {
	    try {
	      tok.validate(*tokiter);
	    } catch (...) {
	      ++tokiter; // skip over the bogus token
	      throw;
	    }
	    toks.push_back(*tokiter);
	  }

	  // Keep track of nesting
	  if ( tokiter->type() == tok_types::open_paren || 
	       tokiter->type() == tok_types::open_brace ) 
	    ++nesting;
	  if ( tokiter->type() == tok_types::close_paren || 
	       tokiter->type() == tok_types::close_brace ) 
	    if ( nesting > 0 )
	      --nesting;

	  ++tokiter;
	}
    }
  while  ( !toks.empty() && ( toks.back().type() == tok_types::comma 
			      || nesting ) );
   
  return toks;
}

void msparser::bells(int b)
{
  if ( b > (int) bell::MAX_BELLS )
    throw runtime_error( make_string() 
			 << "Number of bells must be less than "
			 << bell::MAX_BELLS + 1 );
  else if ( b <= 1 )
    throw runtime_error( "Number of bells must be greater than 1" );

  args.bells = b;
}

statement msparser::parse()
{
  vector<token> cmd( tokenise_command() );

  // EOF
  if ( cmd.size() == 0 || cmd.size() == 1 && 
       ( cmd[0] == "end" || cmd[0] == "quit" || cmd[0] == "exit" ) )
    return statement( NULL );

  // Version directive
  if ( cmd.size() == 1 && cmd[0].type() == tok_types::name
       && cmd[0] == "version" )
    {
      cout << "Version: " RINGING_VERSION "\n";
      return statement( new null_stmt );
    }

  // Bells directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::num_lit
       && cmd[1].type() == tok_types::name && cmd[1] == "bells" )
    {
      bells( string_to_int( cmd[0] ) );
      return statement( new bells_stmt(args.bells) );
    }

  // Extents directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::num_lit
       && cmd[1].type() == tok_types::name && cmd[1] == "extents" )
    {
      return statement( new extents_stmt( string_to_int(cmd[0]) ) );
    }

  // Rounds directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::name 
       && cmd[0] == "rounds" && cmd[1].type() == tok_types::transp_lit )
    {
      return statement( new rounds_stmt(cmd[1]) );
    }

  // Import directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::name
       && cmd[0] == "import" && ( cmd[1].type() == tok_types::name ||
				  cmd[1].type() == tok_types::string_lit ) )
    return statement( new import_stmt(cmd[1]) );
  

  // Prove command
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[0] == "prove" 
       // Make a half-hearted attempt to support a variable called 'prove':
       && cmd[1].type() != tok_types::assignment
       && cmd[1].type() != tok_types::def_assign )
    return statement
      ( new prove_stmt( make_expr( cmd.begin() + 1, cmd.end() ) ) );

  // Print command
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[0] == "print" 
       // Make a half-hearted attempt to support a variable called 'prove':
       && cmd[1].type() != tok_types::assignment
       && cmd[1].type() != tok_types::def_assign )
    return statement
      ( new print_stmt( make_expr( cmd.begin() + 1, cmd.end() ) ) );

  // Definition
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[1].type() == tok_types::assignment )
    return statement
      ( new definition_stmt
        ( cmd[0], cmd.size() == 2 
	            ? expression( new nop_node )
	            : make_expr( cmd.begin() + 2, cmd.end() ) ) );

  // Default definition
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[1].type() == tok_types::def_assign )
    return statement
      ( new default_defn_stmt
        ( cmd[0], cmd.size() == 2 
	            ? expression( new nop_node )
	            : make_expr( cmd.begin() + 2, cmd.end() ) ) );

  throw runtime_error( "Unknown command" );
}



//////////////////////////////////////////////////////////
bool 
msparser::find_close_paren( vector< token >::const_iterator const first,
                            vector< token >::const_iterator const last,
                            tok_types::enum_t open, tok_types::enum_t close,
                            string const& name,
                            vector< token >::const_iterator& result ) const
{
  if ( first->type() != open )
    return false;

  int depth = 0;
  for ( result = first; result != last; ++result )
    {
      if      ( result->type() == open  ) ++depth;
      else if ( result->type() == close ) --depth;
      
      if ( depth == 0 ) 
        {
          if (first+1 == result)
            throw runtime_error( make_string() << "Empty " << name );
	  return true;
        }
    }

  assert(depth);
  throw runtime_error( make_string() << "Unmatched " << name );
}

                                 

// Is the range [first, last) enclosed in a correctly-matched (open, close)
// pair?  E.g. ``(&-6-6-6,+2)'' is enclosed in parentheses; 
// ``(&-6-6-6),(+2)'' is not.
bool msparser::is_enclosed( vector< token >::const_iterator first, 
			    vector< token >::const_iterator const last,
			    tok_types::enum_t open, tok_types::enum_t close,
			    string const& name ) const
{
  if ( !msparser::find_close_paren(first, last, open, close, name, first) )
    return false;
 
  if ( first != last-1 ) 
    return false;
 
  return true;
}

bool msparser::find_last( vector< token >::const_iterator first, 
			  vector< token >::const_iterator const last,
			  tok_types::enum_t tt,
			  vector< token >::const_iterator &result ) const
{
  int depth( 0 );
  bool found( false );
  for ( ; first != last; ++first )
    if      ( first->type() == tok_types::open_brace ||
	      first->type() == tok_types::open_paren )
      ++depth;
    else if ( first->type() == tok_types::close_brace ||
	      first->type() == tok_types::close_paren )
      --depth;
    else if ( !depth && first->type() == tt ) 
      result = first, found = true;

  if (depth) 
    throw runtime_error( "Syntax Error (Unmatched brackets?)" );

  return found;
}

bool msparser::find_first( vector< token >::const_iterator first,
                           vector< token >::const_iterator const last,
                           tok_types::enum_t tt,
                           vector< token >::const_iterator &result ) const
{
  tok_types::enum_t tt_list[1] = { tt };
  return find_first( first, last, tt_list, result );
}

template <size_t N>
bool msparser::find_first( vector< token >::const_iterator first, 
			   vector< token >::const_iterator const last,
			   tok_types::enum_t (&tt_list)[N],
			   vector< token >::const_iterator &result ) const
{
  int depth( 0 );
  for ( ; first != last; ++first )
    if      ( first->type() == tok_types::open_brace ||
	      first->type() == tok_types::open_paren )
      ++depth;
    else if ( first->type() == tok_types::close_brace ||
	      first->type() == tok_types::close_paren )
      --depth;
    else if ( !depth )
      for ( size_t i = 0u; i<N; ++i ) 
        if ( first->type() == tt_list[i] ) {
          result = first;
          return true;
        }

  if (depth) 
    throw runtime_error( "Syntax Error (Unmatched brackets?)" );

  return false;
}

void 
msparser::check_binary_expr( vector< token >::const_iterator first,
                             vector< token >::const_iterator mid,
                             vector< token >::const_iterator last,
                             string const& opname ) const
{
  if ( first == mid )
    throw runtime_error
      ( make_string() << "Binary operator \"" << opname 
                      << "\" needs first argument" );

  if ( mid+1 == last)
    throw runtime_error
      ( make_string() << "Binary operator \"" << opname 
                      << "\" needs second argument" );
}

template <class NodeType>
inline expression
msparser::make_binary_expr( vector< token >::const_iterator first,
                            vector< token >::const_iterator mid,
                            vector< token >::const_iterator last,
                            string const& opname ) const
{
  check_binary_expr( first, mid, last, opname );

  return expression( new NodeType( make_expr( first, mid ),
                                   make_expr( mid+1, last ) ) );
}

expression
msparser::make_assign_expr( vector< token >::const_iterator first,
                            vector< token >::const_iterator mid,
                            vector< token >::const_iterator last ) const
{
  if ( first == mid )
    throw runtime_error
      ( "Assignment operator needs first argument" );
 
  if ( first->type() != tok_types::name ||
       first+1 != mid )
    throw runtime_error
      ( "First argument of assignment operator must be"
        " a variable name" );

  if ( mid+1 == last)
    return expression
      ( new assign_node( *first, expression( new nop_node ) ) );
  else
    return expression
      ( new assign_node( *first, make_expr( mid+1, last ) ) );
}

expression
msparser::make_binary_expr( vector< token >::const_iterator first,
                            vector< token >::const_iterator mid,
                            vector< token >::const_iterator last ) const
{
  switch ( mid->type() ) {
    // Assignment is handled differently because it needn't have a 
    // second argument
    case tok_types::assignment:
      return make_assign_expr( first, mid, last );

# define RINGING_GSIRIL_BINARY_OP( token, node, str )                        \
    case tok_types::token:                                                   \
      return make_binary_expr<node>( first, mid, last, str )
 
    RINGING_GSIRIL_BINARY_OP( comma, list_node, "," );
    RINGING_GSIRIL_BINARY_OP( logic_or, or_node, "||" );
    RINGING_GSIRIL_BINARY_OP( logic_and, and_node, "&&" );
    RINGING_GSIRIL_BINARY_OP( equals, cmp_node<cmp_node_base::eq>, "==" );
    RINGING_GSIRIL_BINARY_OP( not_equals, cmp_node<cmp_node_base::ne>, "!=" );
    RINGING_GSIRIL_BINARY_OP( less, cmp_node<cmp_node_base::lt>, "<" );
    RINGING_GSIRIL_BINARY_OP( less_eq, cmp_node<cmp_node_base::le>, "<=" );
    RINGING_GSIRIL_BINARY_OP( greater, cmp_node<cmp_node_base::lt>, ">" );
    RINGING_GSIRIL_BINARY_OP( greater_eq, cmp_node<cmp_node_base::le>, ">=" );
    RINGING_GSIRIL_BINARY_OP( plus, add_node, "+" );
    RINGING_GSIRIL_BINARY_OP( minus, subtract_node, "-" );
    RINGING_GSIRIL_BINARY_OP( times, repeat_or_multiply_node, "*" );
    RINGING_GSIRIL_BINARY_OP( divide, divide_node, "/" );
    RINGING_GSIRIL_BINARY_OP( ldivide, ldivide_node, "\\" );
    RINGING_GSIRIL_BINARY_OP( modulus, modulus_node, "%" );

#   undef RINGING_GSIRIL_BINARY_OP
    default: abort();
  }
}


expression
msparser::make_alternatives_expr( vector< token >::const_iterator first,
                                  vector< token >::const_iterator last ) const
{
  expression chain( NULL );
  --last;
  while ( last != first && (last-1)->type() == tok_types::semicolon ) 
    --last;

  // Doesn't use recursion because the contents of the braces is 
  // not a single valid expression.
  while ( last != first ) {
    // The last semicolon, or the opening brace (FIRST) if there is none.
    iter_t i( first ); find_last( first+1, last, tok_types::semicolon, i );
    iter_t j( i );

    // Is there a test?
    expression test( NULL );
    if ( find_first( i+1, last, tok_types::colon, j ) )
      test = make_expr( i+1, j );
    else
      test = expression( new pattern_node( bells(), "*" ) );
      
    // Is there a expression?
    expression expr( NULL );
    if ( j+1 != last )
      expr = make_expr( j+1, last );
    else
      expr = expression( new nop_node );

    chain = expression( new if_match_node( test, expr, chain ) );

    last = i;
    while ( last != first && (last-1)->type() == tok_types::semicolon ) 
      --last;
  }
  if ( chain.isnull() )
	throw runtime_error( "Empty braces" );
  return chain;
}

bool msparser::is_unary_op( int t_int ) 
{
  using namespace tok_types;
  enum_t t = (enum_t) t_int;
  return t == bang || t == incr || t == decr;
}

expression
msparser::make_count_expr( vector< token >::const_iterator first,
                           vector< token >::const_iterator last,
                           vector< token >::const_iterator& split ) const
{
  // As with C/C++, we give postfix operators higher precedence 
  // than prefix operators.  This means that !n! means !(n!) -- i.e.
  // not(factorial(n)).  Therefore, we skip over prefix operators
  // and come back to process them after processing postfix operators.

  iter_t pfxend = first;
  while ( pfxend != last && is_unary_op( pfxend->type() ) ) 
    ++pfxend;
  if (pfxend == last)
    throw runtime_error( "Unary prefix operator without argument" );

  // Either a single token (literal or name) or a parethesised group
  split = pfxend;
  if ( split->type() == tok_types::open_paren )
    find_close_paren( split, last, tok_types::open_paren,
                      tok_types::close_paren, "parentheses", split );
  ++split; // Either pass over the ) or pass over the single token (literal)

  expression base( make_expr( pfxend, split ) );

  // Handle all unary postfix operators -- lvalue first
  for ( ; split != last && is_unary_op( split->type() ); ++split )
    if ( split->type() == tok_types::bang )
      base = expression( new factorial_node(base) );
    else if ( split->type() == tok_types::incr )
      base = expression( new incr_node( base, +1, incr_node::postfix ) );
    else if ( split->type() == tok_types::decr )
      base = expression( new incr_node( base, -1, incr_node::postfix ) );
  
  // Now go back over the prefix operators applying them
  while ( pfxend-- != first )
    if ( pfxend->type() == tok_types::bang )
      base = expression( new not_node(base) );
    else if ( pfxend->type() == tok_types::incr )
      base = expression( new incr_node( base, +1, incr_node::prefix ) );
    else if ( pfxend->type() == tok_types::decr )
      base = expression( new incr_node( base, -1, incr_node::prefix ) );

  // This makes exponentiation right associative because that is more
  // useful than left associative.  I.e. we want a^b^c to mean a^(b^c) 
  // because (a^b)^c = a^(b*c).  This is in agreement how bc(1) handles
  // the ^ operator according to POSIX.
  iter_t mid = split;
  if ( mid != last && mid->type() == tok_types::power ) {
    expression exp( make_count_expr( mid+1, last, split ) );  // sets split
    check_binary_expr( first, mid, split, "^" );
    return expression( new exponent_node( base, exp ) );
  }

  return base;
}

expression 
msparser::make_expr( vector< token >::const_iterator first, 
		     vector< token >::const_iterator last ) const
{
  if ( first == last ) 
    throw runtime_error( "Expression expected" );

  // Parentheses first
  if ( is_enclosed( first, last, tok_types::open_paren, 
		    tok_types::close_paren, "parentheses" ) )
    return make_expr( first+1, last-1 );


  // Alternative blocks are enclosed in braces
  if ( is_enclosed( first, last, tok_types::open_brace, 
		    tok_types::close_brace, "braces" ) )
    return make_alternatives_expr( first, last );
     

# define RINGING_GSIRIL_PRECEDENCE_LEVEL( ... )                              \
  {                                                                          \
    iter_t split;                                                            \
    using namespace tok_types;                                               \
    tok_types::enum_t level[] = { __VA_ARGS__ };                             \
    if ( find_first( first, last, level, split ) )                           \
      return make_binary_expr( first, split, last );                         \
  }

  // Binary operators, from lowest to highest precedence:
  RINGING_GSIRIL_PRECEDENCE_LEVEL( assignment )
  RINGING_GSIRIL_PRECEDENCE_LEVEL( comma )
  RINGING_GSIRIL_PRECEDENCE_LEVEL( logic_or )
  RINGING_GSIRIL_PRECEDENCE_LEVEL( logic_and )
  RINGING_GSIRIL_PRECEDENCE_LEVEL( equals, not_equals )
  RINGING_GSIRIL_PRECEDENCE_LEVEL( less, greater, less_eq, greater_eq )
  RINGING_GSIRIL_PRECEDENCE_LEVEL( plus, minus )
  RINGING_GSIRIL_PRECEDENCE_LEVEL( times, divide, ldivide, modulus )

# undef RINGING_GSIRIL_PRECEDENCE_LEVEL

  // Juxtaposition is the next highest precedence operator, and we want it
  // to be right associative -- i.e. a b c == a (b c).  However, life gets 
  // complicated because there are higher precedence operators -- in 
  // particular, the binary ^ operator -- i.e. 2^2 n == 4n.  

  if ( last != first + 1 ) {
    // This case is special because repeat isn't handled in any normal way.
    // A null expr is interpreted by repeated_node to me repeat indefintely.
    if ( first->type() == tok_types::name && *first == "repeat" ) 
      return expression
        ( new repeated_node( expression( new nop_node ), 
                             make_expr( first+1, last ) ) );

    // This will locate any exponentiations and unary operators recursively
    // calling back to here for parentheses or literals (i.e. single tokens)
    iter_t split;
    expression expr( make_count_expr( first, last, split ) );
    // Have we parsed the whole sequence or is anything left?
    // If there's anything left, we must be part of a juxtaposition operator
    // and the expression we've just read is the count.
    if ( split != last )
      return expression( new repeated_node( expr, make_expr( split, last ) ) );
    else
      return expr;
  }

  // Literals
  switch ( first->type() )
    {
    case tok_types::num_lit:
      return expression( new int_node( *first ) );

    case tok_types::string_lit:
      return expression( new string_node( *first ) );

    case tok_types::name:
      if ( *first == "break" )
	return expression( new exception_node( script_exception::do_break ) );
      else if ( *first == "rounds" )
	return expression( new isrounds_node() );
      else
	return expression( new symbol_node( *first ) );

    case tok_types::pn_lit:
      return expression( new pn_node( bells(), *first ) );

    case tok_types::transp_lit:
      return expression( new transp_node( bells(), *first ) );

    case tok_types::regex_lit:
      return expression( new pattern_node( bells(), *first ) );

    default:
      throw runtime_error( "Unknown token in input" );
      return expression( NULL ); // To keep MSVC 5 happy
    }
}

//////////////////////////////////////////////////////////

RINGING_END_ANON_NAMESPACE

shared_pointer<parser> 
make_default_parser( istream& in, const arguments& args )
{
  return shared_pointer<parser>( new msparser( in, args ) );
}
