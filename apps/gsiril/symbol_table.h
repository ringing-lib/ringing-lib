// -*- C++ -*- symbol_table.h - Table of symbols and their definitions
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

#ifndef GSIRIL_SYMBOL_TABLE_INCLUDED
#define GSIRIL_SYMBOL_TABLE_INCLUDED

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

#if RINGING_OLD_INCLUDES
#include <map.h>
#else
#include <map>
#endif
#include <string>
#include "expr_base.h" // MSVC 6 requires expression to be complete.

RINGING_USING_STD
RINGING_USING_NAMESPACE

class expression;

class symbol_table
{
public:
  expression lookup( const string& sym ) const;

  // Returns true for a redefinition and false otherwise
  bool define( const pair< const string, expression >& defn );

  void undefine( const string& sym );

private:
  typedef map< string, expression > sym_table_t;
  sym_table_t sym_table;
};

#endif // GSIRIL_SYMBOL_TABLE_INCLUDED
