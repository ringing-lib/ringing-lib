// -*- C++ -*- bell.cpp - A simple class representing a bell's position
// Copyright (C) 2001 Martin Bright <martin@boojum.org.uk>

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// $Id$


#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma implementation
#endif

#include <ringing/bell.h>

RINGING_USING_STD
  
RINGING_START_NAMESPACE

char bell::symbols[] = "1234567890ETABCDFGHJKLMNPQRSUVWYZ";
const unsigned int bell::MAX_BELLS = 33;
    
bell::invalid::invalid()
  : invalid_argument("The bell supplied was invalid") {}

RINGING_END_NAMESPACE
