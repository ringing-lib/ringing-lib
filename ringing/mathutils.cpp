// mathutils.cpp - Mathematical utility function
// Copyright (C) 2001, 2002, 2003, 2005, 2007, 2009
// Martin Bright <martin@boojum.org.uk> 
// and Richard Smith <richard@ex-parrot.com>.

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

#if RINGING_OLD_C_INCLUDES
#include <stdlib.h>
#include <assert.h>
#else
#include <cstdlib>
#include <cassert>
#endif

#if RINGING_OLD_INCLUDES
#include <algo.h>
#else
#include <algorithm>
#endif

#include <ringing/mathutils.h>

RINGING_USING_STD

RINGING_START_NAMESPACE

// An hcf function. Uses, ahhhh, ze Euclidean algorizm.
int hcf(int a, int b)
{
  // Make sure we have a >= b
  if(a < b) RINGING_PREFIX_STD swap(a, b);

  int r;
  do {
    r = a % b;
    a = b;
    b = r;
  } while(r > 0);

  // a is now the last non-zero remainder we had.
  return a;
}

unsigned factorial(unsigned n)
{
  if (!n) return 1;
  unsigned f = n;
  while (--n)
    f *= n;
  return f;
}

unsigned fibonacci(unsigned n)
{
  if (!n) return 1;
  unsigned f1=1, f2=1;
  while (--n)
    {
      unsigned sum =f1+f2;
      f1 = f2;
      f2 = sum;
    }
  return f2;
}

RINGING_START_ANON_NAMESPACE

static int (*rand_fn)() = &rand;
static int rand_max    = RAND_MAX;

RINGING_END_ANON_NAMESPACE

pair<int (*)(), int> random_fn( int (*randfn)(), int randmax )
{
  pair< int(*)(), int > old = make_pair( rand_fn, rand_max );

  if (randfn) {
    rand_fn  = randfn;
    rand_max = randmax;
  }
 
  return old;
}

unsigned random_int(unsigned max)
{
  assert( max );

  unsigned r;
  do
    r = (unsigned)( (double)rand_fn() * ((double)max / (double)rand_max) );
  while ( r == max );

  assert( r < max );
  return r;
}

bool random_bool( double ptrue )
{
  assert( ptrue <= 1 );
  return rand_fn() < rand_max * ptrue;
}

// Use a recursive implementaion which is O(ln n) in both space and size
RINGING_LLONG ipower( int base, int exp )
{
  if (exp == 0)
    return 1;
  else if (exp < 0)
    return 0;  // inevitably rounds to zero
  else if (exp % 2)
    return base * ipower(base, exp - 1);
  else {
    int temp = ipower(base, exp / 2);
    return temp * temp;
  }
}

RINGING_END_NAMESPACE
