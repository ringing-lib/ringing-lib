#include <ringing/common.h>
#include <ringing/streamutils.h>
#include "execution_context.h"
#include "expression.h"
#include "parser.h"

RINGING_USING_NAMESPACE

execution_context::execution_context( const parser &pa, ostream &os ) 
  : pa(pa), os(os), r(pa.bells())
{}

bool execution_context::permute_and_prove_t::operator()( const change &c )
{
  bool rv = p.add_row( r *= c ); 
  ex.execute_symbol("everyrow");
  if ( r.isrounds() ) ex.execute_symbol("rounds");
  if ( !rv ) ex.execute_symbol("conflict");
  return rv;
}

execution_context::permute_and_prove_t::
permute_and_prove_t( row &r, prover &p, execution_context &ex ) 
  : r(r), p(p), ex(ex)
{
}

execution_context::permute_and_prove_t 
execution_context::permute_and_prove()
{
  return permute_and_prove_t( r, p, *this );
}

void execution_context::execute_symbol( const string &sym ) 
{
  pa.lookup_symbol( sym ).execute( *this );
}

string execution_context::final_symbol() const
{
  if ( p.truth() && r.isrounds() ) 
    return "true";
  else if ( p.truth() )
    return "notround";
  else
    return "false";
}

string execution_context::substitute_string( const string &str, bool &do_exit )
{
  make_string os;

  for ( string::const_iterator i( str.begin() ), e( str.end() ); i != e; ++i )
    switch (*i)
      {
      case '@':
	os << r;
	break;
      case '$': 
	if ( i+1 == e || i[1] != '$' )
	  os << p.duplicates();
	else
	  ++i, do_exit = true;
	break;
      case '#' : os << p.size();        break;
      default:   os << *i;              break;
      }

  return os;
}
