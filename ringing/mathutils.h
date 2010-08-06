// -*- C++ -*- mathutils.h - Mathematical utility functions
// Copyright (C) 2001, 2002, 2005, 2007, 2009, 2010
// Martin Bright <martin@boojum.org.uk>
// and Richard Smith <richard@ex-parrot.com>

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


#ifndef RINGING_MATH_H
#define RINGING_MATH_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

RINGING_START_NAMESPACE
  
RINGING_USING_STD

#define RINGING_PI  3.14159265358979323844

// Return the sign of a number
inline RINGING_API int sign(int a) {
  return a > 0 ? +1 : a < 0 ? -1 : 0;
}

// Declare a couple of random functions for hcf and lcm
RINGING_API int hcf(int a, int b);
inline RINGING_API int lcm(int a, int b)
{
  // Check for a couple of quick cases
  if(a == 1) return b;
  if(b == 1) return a;
  return a * b / hcf(a,b);
}

RINGING_API unsigned factorial(unsigned n);
RINGING_API unsigned fibonacci(unsigned n);

// Uniformly distributed, 0 <= random_int(max) < max
RINGING_API unsigned random_int(unsigned max);

// Returns true with probability ptrue
RINGING_API bool random_bool( double ptrue = 0.5 );

// Uniformally distributed between min and max, exclusive of end points.
RINGING_API double random_uniform_deviate( double min, double max );
RINGING_API double random_uniform_deviate( double max = 1.0 ); // min = 0

// Normally distributed with given mean and standard deviation
RINGING_API double random_normal_deviate( double mean, double stddev );

// Supply an alternative random number generator
// If rand is NULL, then just return the existing one
RINGING_API pair<int (*)(), int> random_fn( int (*randfn)(), int max_rand );

// Return base raised to the power of exp
RINGING_API RINGING_LLONG ipower( int base, int exp );

RINGING_END_NAMESPACE

#endif
