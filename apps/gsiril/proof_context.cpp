// proof_context.cpp - Environment to evaluate expressions
// Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2011, 2012, 2014,
// 2019 Richard Smith <richard@ex-parrot.com>

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

#if RINGING_USE_TERMCAP
# include <curses.h>
# include <term.h>
# ifdef bell
#   undef bell
# endif
# define RINGING_TERMINFO_VAR( name ) \
    ( cur_term && (name) != (char const*)-1 ? (name) : NULL )
#else
# define RINGING_TERMINFO_VAR( name ) NULL
#endif


#include "expr_base.h" // Must be before execution_context.h because 
                       // of bug in MSVC 6.0
#include "proof_context.h"
#include "execution_context.h"

#include <ringing/streamutils.h>

RINGING_USING_NAMESPACE

proof_context::proof_context( const execution_context &ectx ) 
  : ectx(ectx), p( new prover(ectx.get_args().num_extents) ), parent( NULL ),
    output( &ectx.output() ),
    silent( ectx.get_args().everyrow_only || ectx.get_args().filter
            || ectx.get_args().quiet >= 2 ), 
    underline( false )
{
# if RINGING_USE_TERMCAP
  static bool terminfo_initialized = false;
  if ( !terminfo_initialized ) {
    setupterm(NULL, 1, NULL); 
    terminfo_initialized = true;
  }
# endif

  if ( ectx.bells() == -1 )
    throw runtime_error( "Must set number of bells before proving" ); 
  if ( ectx.rounds().bells() > ectx.bells() )
    throw runtime_error( "Rounds is on too many bells" ); 
  r = row(ectx.bells()) * ectx.rounds();
}

proof_context::~proof_context()
{
  if (output) {
    // Turn off any underlining, etc.
    termination_sequence(*output);

    // MSVC 7.1 does not line-buffer std::cout (there is no requirement
    // for it to).
    output->flush();
  }
}

void proof_context::termination_sequence(ostream& os) const
{
  if (underline) {
    if ( char const* e = RINGING_TERMINFO_VAR( exit_underline_mode ) ) {
      os << e; 
      underline = false; 
    }
  }
}

void proof_context::do_output( const string& str ) const
{
  if (!silent && output) 
    *output << str;
}

void proof_context::output_string( const string& str, bool to_parent ) const
{
  bool do_exit( false );
  std::string o( substitute_string(str, do_exit) );

  if ( to_parent && parent ) parent->do_output(o);
  else do_output(o);

  if (do_exit)
    throw script_exception( script_exception::do_abort );
}

void proof_context::execute_everyrow()
{
  // Temporarily disable silent flag if running with -E
  bool s = silent;
  if ( ectx.get_args().everyrow_only && !ectx.get_args().filter ) 
    silent = false;
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
  pctx.execute_everyrow();
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
  return r == ectx.rounds() && p->count_row(r) == ectx.extents(); 
}

void proof_context::execute_symbol( const string& sym, int dir )
{
  if ( ectx.get_args().show_lead_heads && output && sym != "everyrow" ) {
    if ( ectx.get_args().methods.size() ) {
      for ( vector<string>::const_iterator i = ectx.get_args().methods.begin(), 
                                           e = ectx.get_args().methods.end();
            i != e; ++i )
        if ( sym == *i ) {
          *output << r;
          if ( ectx.get_args().methods.size() > 1 )
            *output << "\t" << sym;
          *output << endl;
        }
    } else
      *output << r << "\t" << sym << endl;
  }

  expression e( lookup_symbol(sym) );
  e.execute( *this, dir );
}

expression proof_context::lookup_symbol( const string& sym ) const
{
  expression e( dsym_table.lookup(sym) );
  if ( e.isnull() ) e = ectx.lookup_symbol(sym);
  return e;
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

// When called, i will point to the initial \, and len will be set according
// to i[1] (to 2 for \x, to 4 for \u or to 8 for \U).
static string do_escape_seq(string::const_iterator& i, string::const_iterator e,
                            size_t len) {
  if (i+1+len >= e)
    throw runtime_error("Incomplete \\" + string(1, i[1]) + " escape sequence");

  RINGING_ULLONG val = 0;
  for (int n=0; n<len; ++n) {
    char c = i[2+n];
    if (c >= '0' && c <= '9') val = (val<<4) | (c-'0');
    else if (c >= 'A' && c <= 'F') val = (val<<4) | (c-'A'+10);
    else if (c >= 'a' && c <= 'f') val = (val<<4) | (c-'a'+10);
    else throw runtime_error("Invalid hexadecimal character "
                             "'" + string(1u, c) + "' in "
                             "\\" + string(1u, i[1]) + " escape");
  }
  i += 1+len;

  // Convert to UTF-8
  make_string os;
  if (val < (RINGING_ULLONG)0x80)
    os << char(val);
  else if (val < (RINGING_ULLONG)0x800)
    os << (char)(unsigned char)(0xC0|(val>>6))
       << (char)(unsigned char)(0x80|(val&0x3F));
  else if (val < (RINGING_ULLONG)0x10000)
    os << (char)(unsigned char)(0xE0|(val>>12))
       << (char)(unsigned char)(0x80|((val>>6)&0x3F))
       << (char)(unsigned char)(0x80|(val&0x3F));
  else if (val < (RINGING_ULLONG)0x110000)
    os << (char)(unsigned char)(0xF0|(val>>18))
       << (char)(unsigned char)(0x80|((val>>12)&0x3F))
       << (char)(unsigned char)(0x80|((val>>6)&0x3F))
       << (char)(unsigned char)(0x80|(val&0x3F));
  else throw runtime_error(make_string() << "Character U+" << val
                             << " out of range");
  return os;
}

static string string_escapes( const string &str ) {
  make_string os;
  for ( string::const_iterator i( str.begin() ), e( str.end() ); i != e; ++i )
    switch (*i) {
      case '\\':
	if (i+1 == e) 
	  throw runtime_error("Unexpected backslash");
	else if (i[1] == 'n') 
	  ++i, os << '\n';
	else if (i[1] == 't') 
	  ++i, os << '\t';
        else if (i[1] == 'x') 
          os << do_escape_seq(i, e, 2);
        else if (i[1] == 'u') 
          os << do_escape_seq(i, e, 4);
        else if (i[1] == 'U') 
          os << do_escape_seq(i, e, 8);
	else if (i[1] == '\'' )
	  ++i, os << '"';
	else
	  os << *++i;
	break;
      default:
        os << *i;
    }
  return os;
}

string proof_context::substitute_string( const string &str, 
                                         bool &do_exit ) const
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
        if ( i+1 != e && i[1] == '{' 
             && !ectx.get_args().msiril_syntax
             && !ectx.get_args().sirilic_syntax) {
          string::const_iterator j = std::find(i+2, e, '}');
          if (j == e) throw runtime_error("Incomplete variable interpolation "
                                          "in string '" + str + "'");
          expression e( lookup_symbol( string(i+2, j) ) );
          proof_context ctx2( silent_clone() );
          os << e.string_evaluate(ctx2);
          i = j;
        }
	else if ( i+1 != e && i[1] == '$' )
	  ++i, do_exit = true;
	else
	  os << p->duplicates();
	break;
      case '#':
	os << p->size();
	break;
      case '\\':
	if (i+1 == e) 
	  nl = false;
	else 
	  os << *i;   // It will be handled by string_escapes
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
  return string_escapes(os);
}

proof_context proof_context::silent_clone() const
{
  proof_context copy( *this );
  copy.p = prover::create_branch(copy.p);
  copy.p->disable_proving();
  copy.silent = true;
  copy.output = NULL;
  copy.parent = this;
  return copy;
}

void proof_context::increment_node_count() const
{
  return ectx.increment_node_count();
}

