// symbol_table.cpp - Environment to evaluate expressions
// Copyright (C) 2002, 2003, 2004, 2005 Richard Smith <richard@ex-parrot.com>

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

#include "symbol_table.h"
#include "expr_base.h" // Must be before execution_context.h because 
                       // of bug in MSVC 6.0

RINGING_USING_NAMESPACE


expression symbol_table::lookup( const string& sym ) const
{
  sym_table_t::const_iterator i = sym_table.find(sym);
  if ( i == sym_table.end() )
    return expression( NULL );
  return i->second;
}

bool symbol_table::define( const pair<const string, expression>& defn )
{
  sym_table_t::iterator i = sym_table.find( defn.first );

  // Is it a redefinition?
  if ( i != sym_table.end() )
    {
      i->second = defn.second;
      return true;
    }
  else
    {
      sym_table.insert( defn );
      return false;
    }
}

void symbol_table::undefine( const string& sym )
{
  sym_table.erase(sym);
}

