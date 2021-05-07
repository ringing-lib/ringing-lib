// parser.cpp - Tokenise and parse lines of input
// Copyright (C) 2002, 2003, 2004, 2005, 2007, 2008, 2010, 2011, 2012, 2013,
// 2019, 2020, 2021 Richard Smith <richard@ex-parrot.com>

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
#include "execution_context.h"
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
#else
#include <cctype>
#endif
#include <ringing/streamutils.h>
#include <ringing/place_notation.h>

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
    plus        = '+',
    minus       = '-',
    times       = '*',
    divide      = '/',
    modulo      = '%',
    append      = '.',
    assignment  = '=',
    new_line    = '\n',
    semicolon   = ';',
    reverse     = '~',
    greater     = '>',
    less        = '<',
    logic_not   = '!',
    merge       = '&',
    ctrl_z      = '\x1A',   /* EOF marker in microSIRIL */
    comment     = tokeniser::first_token,
    string_lit,
    transp_lit,
    pn_lit,
    def_assign,    /* ?= */
    imm_assign,    /* := */
    append_assign, /* .= */
    logic_and,     /* && */
    logic_or,      /* || */
    equals,        /* == */
    not_equals,    /* != */
    greater_eq,    /* >= */
    less_eq,       /* <= */
    increment,     /* ++ */
    decrement,     /* -- */
    left_shift,    /* << */
    right_shift,   /* >> */
    regex_lit      /* /.../ */
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
    string::const_iterator j(i);
    if ( *j != '&' && *j != '+' ) return failed; ++j;
    while (j != e) {
      if ( *j == '{' ) j = read_bell_expr(j, e);
      else if ( *j == '.' || *j == '-' || isalnum(*j) ) ++j;
      else break;
    }
    if ( j - i < 2 ) return failed;
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
      immass( ":=", tok_types::imm_assign ),
      appass( ".=", tok_types::append_assign ),
      land( "&&", tok_types::logic_and ), lor( "||", tok_types::logic_or ),
      cmpeq( "==", tok_types::equals ), cmpne( "!=", tok_types::not_equals ),
      cmpge( ">=", tok_types::greater_eq ), cmple( "<=", tok_types::less_eq ),
      lsh( "<<", tok_types::left_shift ), rsh( "<<", tok_types::right_shift ),
      inc( "++", tok_types::increment ), dec( "--", tok_types::decrement )
  {
    // Note:  It is important that &sym is added after &land; and
    // similarly, that &r is added after &c.  This is because && is a
    // longer, and thus more specialised prefix than &, and similarly
    // for // and /.

    add_qtype(&c);
    // The pattern syntax conflicts with MicroSiril comments
    if ( !args.msiril_syntax ) add_qtype(&r);
    add_qtype(&q);      add_qtype(&qq);
    add_qtype(&defass); add_qtype(&immass);
    add_qtype(&appass); 
    add_qtype(&land);   add_qtype(&lor);
    add_qtype(&cmpeq);  add_qtype(&cmpne);
    add_qtype(&cmpge);  add_qtype(&cmple);
    add_qtype(&inc);    add_qtype(&dec);
    add_qtype(&lsh);    add_qtype(&rsh);
    add_qtype(&sym);    add_qtype(&asym);

    // Properly MicroSiril mode should disable _, but that would be unhelpful
    if ( args.msiril_syntax ) 
      set_id_chars( // Characters allowed at the start of an ID:
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz"
                    "_",
                    // Characters allowed within an ID:
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz"
                    "01234567890"  "_%-!" );
  }

  virtual void validate( const token& t ) const
  {
    using namespace tok_types;

    switch ( t.type() ) {
    case name: case num_lit: case open_paren: case close_paren:
    case comma: case times: case assignment: case new_line: case semicolon:
    case comment: case string_lit: case transp_lit: case pn_lit: 
    case def_assign: case imm_assign: case append_assign: case reverse:
      return;

    case ctrl_z:
      // Obsolete microSIRIL features that we don't want to support
      if ( args.msiril_syntax ) return;
      break;

    case plus: case minus: case modulo: case append: 
    case merge: case left_shift: case right_shift:
    case regex_lit: case open_brace: case close_brace: case colon:
    case logic_and: case logic_or: case equals: case not_equals: 
    case less: case greater: case less_eq: case greater_eq:
    case increment: case decrement: case logic_not:
      // These can only work when msiril comments are disabled.
      // Regexp literals are disabled because they conflict with 
      // the MicroSiril comment; alternative blocks are disabled because
      // they're essentially useless without regexp literals to put in 
      // the test; and logical operators and comparisons are disabled 
      // because without alternative blocks, there's nowhere to put them. 
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
  basic_token defass, immass, appass, land, lor, cmpeq, cmpne, cmpge, cmple, 
              lsh, rsh, inc, dec;
};


//////////////////////////////////////////////////////////

class msparser : public parser
{
public:
  msparser( istream& in, execution_context& e ) 
    : ectx(e), args(e.get_args()), tok(in, args),
      tokiter(tok.begin()), tokend(tok.end())
  {}

private:
  virtual statement parse();
  virtual int line() const { return tok.line(); }

  int bells() const { return ectx.bells(); }

  vector< token > tokenise_command();

  expression make_expr( vector< token >::const_iterator first, 
			vector< token >::const_iterator last ) const;

  vector<expression> 
  make_args( vector< token >::const_iterator first, 
             vector< token >::const_iterator last ) const;

  bool is_enclosed( vector< token >::const_iterator first, 
		    vector< token >::const_iterator last,
		    tok_types::enum_t open, tok_types::enum_t close,
		    string const& name ) const;

  bool is_bin_only_operator( int tok_type ) const;

  enum first_or_last { find_first, find_last };

  bool find_one_of( vector< token >::const_iterator first, 
                    vector< token >::const_iterator last,
                    vector< tok_types::enum_t > const& tts,
                    vector< token >::const_iterator &result,
                    first_or_last = find_first ) const;

  bool find( vector< token >::const_iterator first, 
             vector< token >::const_iterator last,
             tok_types::enum_t tt,
             vector< token >::const_iterator &result,
             first_or_last fl = find_first  ) const {
    return find_one_of( first, last, vector<tok_types::enum_t>(1, tt), 
                        result, fl );
  }

  // Data members
  execution_context& ectx;
  arguments args;
  mstokeniser tok;
  mstokeniser::const_iterator tokiter, tokend;
};

bool msparser::is_bin_only_operator( int tok ) const {
  return tok == tok_types::comma || tok == tok_types::append;
}

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
  while ( !toks.empty() && ( is_bin_only_operator( toks.back().type() )
	                       || nesting ) && tokiter != tokend );

  if ( !toks.empty() && ( toks.back().type() == tok_types::comma || nesting ) )
    throw runtime_error( make_string() 
      << "Unexpected end of file midway through command" );
   
  return toks;
}

statement msparser::parse()
{
  vector<token> cmd( tokenise_command() );

  // EOF
  if ( cmd.size() == 0 || cmd.size() == 1 && 
       ( cmd[0] == "end" || cmd[0] == "quit" || cmd[0] == "exit" 
         || cmd[0].type() == tok_types::ctrl_z ) )
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
    return statement( new bells_stmt( string_to_int(cmd[0]) ) );

  // Extents directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::num_lit
       && cmd[1].type() == tok_types::name && cmd[1] == "extents" )
    return statement( new extents_stmt( string_to_int(cmd[0]) ) );

  // Rows directive (simple form)
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::num_lit
       && cmd[1].type() == tok_types::name && cmd[1] == "rows" )
    return statement( new rows_stmt( string_to_int(cmd[0]) ) );

  // Rows directive (range form)
  if ( cmd.size() == 4 && cmd[0].type() == tok_types::num_lit
       && cmd[1].type() == tok_types::name && cmd[1] == "to" 
       && cmd[2].type() == tok_types::num_lit
       && cmd[3].type() == tok_types::name && cmd[3] == "rows" )
    return statement( new rows_stmt( string_to_int(cmd[0]),
                                     string_to_int(cmd[1]) ) );

  // Rounds directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::name 
       && cmd[0] == "rounds" && cmd[1].type() == tok_types::transp_lit )
    return statement( new rounds_stmt(cmd[1]) );

  // Row mask directive
  if ( cmd.size() == 2 && cmd[0].type() == tok_types::name 
       && cmd[0] == "row_mask" && cmd[1].type() == tok_types::regex_lit )
    return statement( new row_mask_stmt(cmd[1]) );

  // Import directive
  if ( !args.disable_import
       && cmd.size() == 2 && cmd[0].type() == tok_types::name
       && cmd[0] == "import" && ( cmd[1].type() == tok_types::name ||
				  cmd[1].type() == tok_types::string_lit ) )
    return statement( new import_stmt(cmd[1]) );
  
  // Prove command
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && cmd[0] == "prove" 
       // Make a half-hearted attempt to support a variable called 'prove':
       && cmd[1].type() != tok_types::assignment
       && cmd[1].type() != tok_types::imm_assign 
       && cmd[1].type() != tok_types::def_assign )
    return statement
      ( new prove_stmt( make_expr( cmd.begin() + 1, cmd.end() ) ) );

  // Echo directive
  if ( cmd[0].type() == tok_types::name && 
       ( cmd[0] == "echo" || cmd[0] == "warn" || cmd[0] == "error" ) )
    return statement
      ( new echo_stmt( cmd.size() == 1 
                         ? expression( new string_node("") )
                         : make_expr( cmd.begin() + 1, cmd.end() ),
                       cmd[0] ) );

  // Conditional statements:  if, elseif, else, endif
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name && cmd[0] == "if" )
    return statement
      ( new if_stmt( if_stmt::push_if, 
                     make_expr(  cmd.begin() + 1, cmd.end() ) ) );

  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name && 
       cmd[0] == "elseif" )
    return statement
      ( new if_stmt( if_stmt::chain_else_if, 
                     make_expr(  cmd.begin() + 1, cmd.end() ) ) );

  if ( cmd.size() == 1 && cmd[0].type() == tok_types::name && cmd[0] == "else" )
    return statement( new if_stmt( if_stmt::chain_else ) );

  if ( cmd.size() == 1 && cmd[0].type() == tok_types::name && 
       cmd[0] == "endif" )
    return statement( new if_stmt( if_stmt::pop_if ) );

  // Definition
  if ( cmd.size() > 1 && cmd[0].type() == tok_types::name
       && ( cmd[1].type() == tok_types::assignment ||
            cmd[1].type() == tok_types::def_assign ||
            cmd[1].type() == tok_types::imm_assign ) ) {
    expression val( cmd.size() == 2 ? expression( new nop_node )
                    : make_expr( cmd.begin() + 2, cmd.end() ) );
    switch ( cmd[1].type() ) {
      case tok_types::assignment:
        return statement( new definition_stmt( cmd[0], val ) );
      case tok_types::def_assign:
        return statement( new default_defn_stmt( cmd[0], val ) );
      case tok_types::imm_assign:
        return statement( new immediate_defn_stmt( cmd[0], val ) );
    }
  }

  throw runtime_error( "Unknown command: " + cmd[0] + " ..." );
}



//////////////////////////////////////////////////////////

// Is the range [first, last) enclosed in a correctly-matched (open, close)
// pair?  E.g. ``(&-6-6-6,+2)'' is enclosed in parentheses; 
// ``(&-6-6-6),(+2)'' is not.
bool msparser::is_enclosed( vector< token >::const_iterator first, 
			    vector< token >::const_iterator const last,
			    tok_types::enum_t open, tok_types::enum_t close,
			    string const& name ) const
{
  if ( first->type() != open || (last-1)->type() != close )
    return false;

  int depth = 0;
  for ( ; first != last; ++first )
    {
      if      ( first->type() == open  ) ++depth;
      else if ( first->type() == close ) --depth;
      
      if ( depth == 0 && first != last-1 ) 
	return false;
    }

  if (depth) 
    throw runtime_error( make_string() << "Unmatched " << name );

  if (first+1 == last-1)
    throw runtime_error( make_string() << "Empty " << name );

  return true;
}

bool msparser::find_one_of( vector< token >::const_iterator first, 
                            vector< token >::const_iterator const last,
                            vector< tok_types::enum_t > const& tts,
                            vector< token >::const_iterator &result,
                            msparser::first_or_last fl ) const
{
  int depth = 0;
  bool found = false;
  for ( ; first != last; ++first )
    if      ( first->type() == tok_types::open_brace ||
	      first->type() == tok_types::open_paren )
      ++depth;
    else if ( first->type() == tok_types::close_brace ||
	      first->type() == tok_types::close_paren )
      --depth;
    else if ( depth == 0 )
      for ( vector<tok_types::enum_t>::const_iterator 
              i=tts.begin(), e=tts.end(); i!=e; ++i )
        if (first->type() == *i ) {
          result = first;
          if (fl == find_first) return true;
          found = true;
        }

  if (depth) 
    throw runtime_error( "Syntax Error (Unmatched brackets?)" );

  return found;
}

static void 
check_bin_op( vector< token >::const_iterator const& first,
              vector< token >::const_iterator const& split,
              vector< token >::const_iterator const& last,
              const char* op ) {
  if ( first == split )
    throw runtime_error
      ( make_string() << "Binary operator \"" << op 
                      << "\" needs first argument" );
      
  if ( split+1 == last)
    throw runtime_error
      ( make_string() << "Binary operator \"" << op 
                      << "\" needs second argument" );
}

static void 
check_unary_op( vector< token >::const_iterator const& first,
                vector< token >::const_iterator const& last,
                const char* op,
                tok_types::enum_t arg_type = (tok_types::enum_t)(-1),
                const char* arg_type_name = NULL ) {
  if ( first+1 == last)
    throw runtime_error
      ( make_string() << "Unary prefix operator \"" << op 
                      << "\" needs an argument" );
  if ( arg_type != (tok_types::enum_t)(-1) && (first + 1)->type() != arg_type )
    throw runtime_error
      ( make_string() << "The argument to the \"" << op << "\" operator "
                         "must be " << arg_type_name );
}

expression 
msparser::make_expr( vector< token >::const_iterator first, 
		     vector< token >::const_iterator last ) const
{
  typedef vector< token >::const_iterator iter_t;

  if ( first == last ) 
    throw runtime_error( "Expression expected" );

  // Parentheses first
  if ( is_enclosed( first, last, tok_types::open_paren, 
		    tok_types::close_paren, "parentheses" ) )
    return make_expr( first+1, last-1 );


  // Alternative blocks are enclosed in braces
  if ( is_enclosed( first, last, tok_types::open_brace, 
		    tok_types::close_brace, "braces" ) )
    {
      expression chain( NULL );
      --last;
      while ( last != first && (last-1)->type() == tok_types::semicolon ) 
	--last;

      // Doesn't use recursion because the contents of the braces is 
      // not a single valid expression.
      while ( last != first ) {
	// The last semicolon, or the opening brace (FIRST) if there is none.
	iter_t i( first ); 
        find( first+1, last, tok_types::semicolon, i, find_last );
	iter_t j( i );

	// Is there a test?
	expression test( NULL );
	if ( find( i+1, last, tok_types::colon, j ) )
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

  iter_t split;

  // The various assignment operators (e.g. =, .=) are jointly the lowest 
  // precedence operator
  //
  // Note: this can be counter-intuitive.  The logic is that an assignment
  // statement takes its whole argument, including commas, e.g. `p = m, +2'.
  // It seemed logical to want `plh = "@", (p=m, +2)' to work similarly.
  // However this means the comma operator has higher precedence than 
  // assignment, so you need to write `b = +4, (sym="-\"), head', etc.
  // I don't know whether this was the right decision.
  //
  vector<tok_types::enum_t> assops;
  assops.push_back( tok_types::assignment );
  assops.push_back( tok_types::append_assign );
  assops.push_back( tok_types::imm_assign );
  if ( find_one_of( first, last, assops, split ) )
    {
      if ( first == split )
	throw runtime_error
	  ( "Assignment operator needs first argument" );
      
      if ( first->type() != tok_types::name || 
	   first+1 != split )
	throw runtime_error
	  ( "First argument of assignment operator must be"
	    " a variable name" );
     
      if ( split->type() == tok_types::assignment ) {
        if ( split+1 == last)
          return expression
            ( new assign_node( *first, expression( new nop_node ) ) );
        else
          return expression
            ( new assign_node( *first, make_expr( split+1, last ) ) );
      }
      else {
        check_bin_op( first, split, last, ".=" );
        if ( split->type() == tok_types::append_assign )
          return expression
            ( new append_assign_node( *first, make_expr( split+1, last ) ) );
        else
          return expression
            ( new immediate_assign_node( *first, make_expr( split+1, last ) ) );
      }
    }

  // Comma is the next lowest precedence operator
  // It is left associative
  if ( find( first, last, tok_types::comma, split ) ) {
    check_bin_op( first, split, last, "," );
    return expression( new list_node( make_expr( first, split ),
                                      make_expr( split+1, last ) ) );
  }

  // Logical operators -- Or (||) is lowest precedence
  if ( find( first, last, tok_types::logic_or, split ) ) {
    check_bin_op( first, split, last, "||" );
    return expression( new or_node( make_expr( first, split ),
                                    make_expr( split+1, last ) ) );
  }

  // Logical operators -- And (&&) is next lowest precedence
  if ( find( first, last, tok_types::logic_and, split ) ) {
    check_bin_op( first, split, last, "&&" );
    return expression( new and_node( make_expr( first, split ),
                                     make_expr( split+1, last ) ) );
  }

  // Comparison operators.  We handle these at a single precedence level,
  // unlike C, C++, Perl, Java, JS, etc. which all give == and != lower 
  // precedence.  Python does the same as us and flattens its comparisons 
  // to a single precedence level.
  vector<tok_types::enum_t> cmpops;
  cmpops.push_back( tok_types::equals );
  cmpops.push_back( tok_types::not_equals );
  cmpops.push_back( tok_types::greater );
  cmpops.push_back( tok_types::less );
  cmpops.push_back( tok_types::greater_eq );
  cmpops.push_back( tok_types::less_eq );
  if ( find_one_of( first, last, cmpops, split ) ) {
    cmp_node::cmp_t cmp;
    switch (split->type()) {
      case tok_types::equals:     cmp = cmp_node::equals;     break;
      case tok_types::not_equals: cmp = cmp_node::not_equals; break;
      case tok_types::greater:    cmp = cmp_node::greater;    break;
      case tok_types::less:       cmp = cmp_node::less;       break;
      case tok_types::greater_eq: cmp = cmp_node::greater_eq; break;
      case tok_types::less_eq:    cmp = cmp_node::less_eq;    break;
      default: abort();
    }
    check_bin_op( first, split, last, cmp_node::symbol(cmp) );
    return expression( new cmp_node( make_expr( first, split ),
                                     make_expr( split+1, last ), cmp ) );
  }

  // Replacement operators come next.  These use &, << and >> which in C-like
  // languages would be lower precedent than comparison operators.  That's 
  // really a bug in C due to its legacy from B, and we can fix it here.
  if ( find( first, last, tok_types::merge, split ) ) {
    check_bin_op( first, split, last, "&" );
    return expression( new merge_node( make_expr( first, split ),
                                       make_expr( split+1, last ) ) );
  }
  
  vector<tok_types::enum_t> shiftops;
  shiftops.push_back( tok_types::left_shift );
  shiftops.push_back( tok_types::right_shift );
  if ( find_one_of( first, last, shiftops, split ) ) {
    check_bin_op( first, split, last, 
                  split->type() == tok_types::left_shift ? "<<" : ">>" );
    return expression( new replacement_node
      ( make_expr( first, split ), make_expr( split+1, last ),
        split->type() == tok_types::left_shift 
          ? replacement_node::end : replacement_node::begin ) );
  }

  // Addition operators are next
  vector<tok_types::enum_t> addops;
  addops.push_back( tok_types::plus );
  addops.push_back( tok_types::minus );
  if ( find_one_of( first, last, addops, split ) ) {
    if ( split->type() == tok_types::plus ) {
      check_bin_op( first, split, last, "+" );
      return expression( new add_node( make_expr( first, split ),
                                       make_expr( split+1, last ), +1 ) );
    } else {
      check_bin_op( first, split, last, "-" );
      return expression( new add_node( make_expr( first, split ),
                                       make_expr( split+1, last ), -1 ) );
    }
  }

  // Now multiplication operators: currently only % and .
  vector<tok_types::enum_t> multops;
  multops.push_back( tok_types::modulo );
  multops.push_back( tok_types::append );
  if ( find_one_of( first, last, multops, split ) ) {
    if ( split->type() == tok_types::modulo ) {
      check_bin_op( first, split, last, "%" );
      return expression( new mod_node( make_expr( first, split ),
                                       make_expr( split+1, last ) ) );
    } else {
      check_bin_op( first, split, last, "." );
      return expression( new append_node( make_expr( first, split ),
                                          make_expr( split+1, last ) ) );
    }
  }

  // The defined operator
  if ( first->type() == tok_types::name && *first == "defined" &&
       ( first + 4 == last && (first+1)->type() == tok_types::open_paren 
                           && (first+3)->type() == tok_types::close_paren || 
         first + 2 == last )
       && ( first + ( first + 4 == last ? 2 : 1 ) )->type() == tok_types::name )
    return expression( new defined_node( 
      *( first + ( first + 4 == last ? 2 : 1 ) ) ) );

  // TODO: Support a integer expression in a repeat block

  // A repeated block is the only remaining construct that is not a single
  // token.
  if ( first->type() == tok_types::num_lit && first + 1 != last ||
       first->type() == tok_types::name && *first == "repeat" )
    {
      iter_t begin_arg = first + 1;

      if ( begin_arg != last && begin_arg->type() == tok_types::times )
	++begin_arg;

      if ( begin_arg == last )
	throw runtime_error
	  ( make_string() << "The repetition operator requires an argument, "
            "or did you mean +" << *first << "?" );

      int times(-1);
      if ( first->type() == tok_types::num_lit )
	times = string_to_int( *first );

      return expression
	( new repeated_node( times,
			     make_expr( begin_arg, last ) ) );
    }

  // Unary opreators

  if ( first->type() == tok_types::increment || 
       first->type() == tok_types::decrement ) {
    char const* sym = "++";  int val = +1;
    if (first->type() == tok_types::decrement) sym = "--", val = -1;
    check_unary_op(first, last, sym, tok_types::name, "a symbol name");
    return expression( new increment_node( *(first+1), val ) );
  }

  if ( first->type() == tok_types::name && *first == "echo" ) {
    check_unary_op(first, last, "echo",
                   tok_types::string_lit, "a string");
    return expression( new string_node( *(first+1), 
      string_node::interpolate | string_node::to_parent ) );
  }

  if ( first->type() == tok_types::reverse ) {
    check_unary_op(first, last, "~");
    return expression( new reverse_node( make_expr( first+1, last ) ) );
  }

  if ( first->type() == tok_types::logic_not ) {
    check_unary_op(first, last, "!");
    return expression( new not_node( make_expr( first+1, last ) ) );
  }

  // Is it a function call?
  if ( last - first >= 3 && first->type() == tok_types::name && 
       is_enclosed( first+1, last, tok_types::open_paren, 
                    tok_types::close_paren, "parentheses" ) )
    return expression( new call_node(*first, make_args(first+2, last-1) ) );

  // Everything left is a literal of some sort
  if ( last - first != 1 )
    throw runtime_error( make_string() << "Parse error before " << *first  );

  switch ( first->type() ) {
    case tok_types::string_lit:
      return expression( new string_node( *first, string_node::interpolate ) );

    case tok_types::name:
      if ( *first == "break" )
	return expression( new exception_node( script_exception::do_break ) );
      else if ( *first == "rounds" )
	return expression( new isrounds_node() );
      else if ( *first == "true" )
	return expression( new boolean_node(true) );
      else if ( *first == "false" )
	return expression( new boolean_node(false) );
      else if ( *first == "bells" )
	return expression( new bells_node() );
      else if ( *first == "endproof" )
	return expression( new endproof_node() );
      else if ( *first == "proving" )
	return expression( new isproving_node() );
      else
	return expression( new symbol_node( *first ) );

    case tok_types::pn_lit:
      return expression( new pn_node( bells(), *first ) );

    case tok_types::transp_lit:
      return expression( new transp_node( bells(), *first ) );

    case tok_types::regex_lit:
      return expression( new pattern_node( bells(), *first ) );

    case tok_types::num_lit:
      return expression( new integer_node( string_to_int(*first) ) );

    default:
      throw runtime_error( "Unknown token in input" );
      return expression( NULL ); // To keep MSVC 5 happy
  }
}

vector<expression>
msparser::make_args( vector< token >::const_iterator first, 
		     vector< token >::const_iterator last ) const
{
  vector<expression> args;
  if ( first != last ) {
    while (true) {
      vector< token >::const_iterator split = last;
      find( first, last, tok_types::comma, split );
      if ( first == split )
        throw runtime_error("Empty argument in function call");
      args.push_back( make_expr( first, split ) );
      if ( split == last ) break;
      first = split + 1;
    }
  }
  return args;
}

//////////////////////////////////////////////////////////

RINGING_END_ANON_NAMESPACE

shared_pointer<parser> 
make_default_parser( istream& in, execution_context& e )
{
  return shared_pointer<parser>( new msparser( in, e ) );
}

int parser::run( execution_context& e, const string& filename,
                 error_policy ep  ) 
{
  int count(0);

  while (true) {
    try {
      statement s( parse() );
      if ( s.eof() ) break;
      s.execute(e);
      ++count;
    }
    catch (const exception& ex ) {
      if (filename.empty())
        cerr << "Error: " << ex.what() << endl;
      else
        cerr << filename << ':' << line() << ": " << ex.what() << endl;
 
      if (ep == fatal)
        exit(2);
      else if (ep == propagate)
        throw runtime_error( "Error importing module: " + filename );
    }
  }

  return count;
}
