/********************************************************************
 * File            : proof.cc
 * Last Modified by: Mark Banner
 * Last Modified   : 24/04/01
 * Description     :
 *     This is the implementation part of proof.h. As proof is a
 * template class, most of it is defined in the header, however the
 * hash function must be defined here.
 ********************************************************************/
#ifdef __GNUG__
#pragma implementation
#endif

#include "proof.h"

// Our default hash function
int our_hash(const row& r)
{
  int sum = 0;
  int factor = 1;
  for (int i = 0; i < r.bells(); i++)
    {
      sum += (r[i] + 1) * factor;
      factor *= 10;
    }
  return sum;
}
