// proof_context.cpp - Environment to evaluate expressions
// Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007
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

#if RINGING_USE_READLINE
// TODO: Should allow terminfo without readline, and should cope with the 
// possibility of readline not linking against terminfo.
# include <term.h>
# ifdef bell
#   undef bell
# endif
# define RINGING_TERMINFO_VAR( name ) \
    ( (name) == (char const*)-1 ? 0 : (name) )
#else
# define RINGING_TERMINFO_VAR( name ) 0
#endif


#include "expr_base.h" // Must be before execution_context.h because 
                       // of bug in MSVC 6.0
#include "proof_context.h"
#include "execution_context.h"

#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE

proof_context::proof_context( const execution_context &ectx ) 
  : ectx(ectx), p( new prover(ectx.get_args().num_extents) ), 
    output( &ectx.output() ),
    silent( ectx.get_args().everyrow_only ), underline( false )
{
  if ( ectx.bells() == -1 )
    throw runtime_error( "Must set number of bells before proving" ); 
  if ( ectx.rounds().bells() > ectx.bells() )
    throw runtime_error( "Rounds is on too many bells" ); 
  r = row(ectx.bells()) * ectx.rounds();
}

proof_context::~proof_context()
{
  // MSVC 7.1 does not line-buffer std::cout (there is no requirement
  // for it to).
  if (output) {
    termination_sequence(*output);
    output->flush();
  }
}

void proof_context::termination_sequence(ostream& os)
{
  if (underline) {
    if ( char const* e = RINGING_TERMINFO_VAR( exit_underline_mode ) ) {
      os << e; 
      underline = false; 
    }
  }
}

void proof_context::output_string( const string& str )
{
  bool do_exit( false );
  std::string o( substitute_string(str, do_exit) );
  if ( !silent && output )
    *output << o;
  if (do_exit)
    throw script_exception( script_exception::do_abort );
}

void proof_context::execute_everyrow()
{
  // Temporarily disable silent flag if running with -E
  bool s = silent;
  if ( ectx.get_args().everyrow_only ) silent = false;
  execute_symbol("everyrow");
  silent = s;
}

bool proof_context::permute_and_prove_t::operator()( const change &c )
{
  bool rv = p.add_row( r *= c ); 
  pctx.execute_everyrow();
  if ( pctx.isrounds() ) pctx.execute_symbol("rounds");
  if ( !rv ) pctx.execute_symbol("conflict");
  return rv;
}

bool proof_context::permute_and_prove_t::operator()( const row &c )
{
  bool rv = p.add_row( r *= c ); 
  pctx.execute_symbol("everyrow");
  if ( pctx.isrounds() ) pctx.execute_symbol("rounds");
  if ( !rv ) pctx.execute_symbol("conflict");
  return rv;
}

proof_context::permute_and_prove_t::
permute_and_prove_t( row &r, prover &p, proof_context &pctx ) 
  : r(r), p(p), pctx(pctx)
{
}

proof_context::permute_and_prove_t 
proof_context::permute_and_prove()
{
  return permute_and_prove_t( r, *p, *this );
}

bool proof_context::isrounds() const 
{
  return r == ectx.rounds(); 
}

void proof_context::execute_symbol( const string &sym ) 
{
  expression e( dsym_table.lookup(sym) );
  if ( e.isnull() ) e = ectx.lookup_symbol(sym);
  e.execute( *this );
}

void proof_context::define_symbol( const pair<const string, expression>& defn )
{
  dsym_table.define(defn);
}

proof_context::proof_state proof_context::state() const
{
  if ( p->truth() && isrounds() ) 
    return rounds;
  else if ( p->truth() )
    return notround;
  else
    return isfalse;
}

string proof_context::substitute_string( const string &str, bool &do_exit )
{
  make_string os;
  bool nl = true;

  for ( string::const_iterator i( str.begin() ), e( str.end() ); i != e; ++i )
    switch (*i)
      {
      case '@':
	os << r;
	break;
      case '$': 
	if ( i+1 == e || i[1] != '$' )
	  os << p->duplicates();
	else
	  ++i, do_exit = true;
	break;
      case '#':
	os << p->size();
	break;
      case '\\':
	if (i+1 == e) 
	  nl = false;
	else if (i[1] == 'n') 
	  ++i, os << '\n';
	else if (i[1] == 't') 
	  ++i, os << '\t';
	else if (i[1] == '\'' )
	  ++i, os << '"';
	else
	  os << *++i;
	break;
      case '_':
        if (ectx.get_args().sirilic_syntax) {
          if ( char const* e = RINGING_TERMINFO_VAR( enter_underline_mode ) ) {
            os << e; underline = true; break; 
          }
        } // fall through
      default:
	os << *i;
	break;
      }

  if (nl) {
    termination_sequence(os.out_stream());
    os << '\n';
  }
  return os;
}

proof_context proof_context::silent_clone() const
{
  proof_context copy( *this );
  copy.p = prover::create_branch(copy.p);
  copy.silent = true;
  copy.output = NULL;
  return copy;
}
