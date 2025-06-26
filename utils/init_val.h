// -*- C++ -*- init_val.h - helper template to initialse built-in types
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

#ifndef RINGING_INIT_VAL_INCLUDED
#define RINGING_INIT_VAL_INCLUDED

#include <ringing/common.h>

template <class Type>
class init_val_base {
public:
  operator Type() const { return val; }
  init_val_base &operator=( Type x ) { val = x; return *this; }
  
  const Type &get() const { return val; }
  Type &get() { return val; }

protected:
  init_val_base( Type val ) : val( val ) {}

private:
  Type val;
};

template <class Type, Type Init>
class init_val : public init_val_base<Type> {
public:
  init_val() : init_val_base<Type>( Init ) {}
  init_val( Type val ) : init_val_base<Type>( val ) {}

  init_val<Type, Init>& operator=( Type val )
    { init_val_base<Type>::operator=(val); return *this; }
};

#endif // RINGING_INIT_VAL_INCLUDED
