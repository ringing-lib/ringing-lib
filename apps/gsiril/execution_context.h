// -*- C++ -*-

#ifndef GSIRIL_EXECUTION_CONTEXT_INCLUDED
#define GSIRIL_EXECUTION_CONTEXT_INCLUDED

#include <ringing/common.h>
#if RINGING_OLD_INCLUDES
#include <map.h>
#else
#include <map>
#endif
#if RINGING_HAVE_OLD_IOSTREAM
#include <ostream.h>
#else
#include <iosfwd>
#endif
#include <string>
#include <ringing/row.h>
#include <ringing/proof.h>
#include <ringing/pointers.h>

RINGING_USING_STD
RINGING_USING_NAMESPACE

class parser;

class execution_context
{
public:
  struct permute_and_prove_t
  {
  public:
    typedef change argument_type;
    typedef bool result_type;
  
    bool operator()( const change &c );

  private:
    friend class execution_context;
    permute_and_prove_t( row &r, prover &p, execution_context &ex );
  
    row &r;
    prover &p;
    execution_context &ex;
  };

  execution_context( const parser &pa, ostream &os );
  
  ostream &output() { return os; }

  permute_and_prove_t permute_and_prove();

  void execute_symbol( const string &sym );

  string final_symbol() const;

  string substitute_string( const string &str, bool &do_exit );

private:
  const parser &pa;
  ostream &os;
  row r;
  prover p;
};

#endif // GSIRIL_EXECUTION_CONTEXT_INCLUDED
