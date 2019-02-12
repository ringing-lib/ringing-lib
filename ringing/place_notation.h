// -*- C++ -*- place_notation.h - Functions for parsing placenotation
// Copyright (C) 2001, 2009, 2019 Martin Bright <martin@boojum.org.uk> and
// Richard Smith <richard@ex-parrot.com>

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


#ifndef RINGING_PLACE_NOTATION_H
#define RINGING_PLACE_NOTATION_H

#include <ringing/common.h>

#if RINGING_OLD_C_INCLUDES
#include <ctype.h>
#else
#include <cctype>
#endif

#if RINGING_OLD_INCLUDES
#include <algo.h>
#include <list.h>
#include <stdexcept.h>
#else
#include <algorithm>
#include <list>
#include <stdexcept>
#endif
#include <string>

#include <ringing/change.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

#if RINGING_HAS_PRAGMA_INTERFACE
#pragma interface
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

#if RINGING_AS_DLL && defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4275 )
#endif

struct place_notation { 
  // Exception thrown if place-notation is malformed 
  struct RINGING_API invalid : public invalid_argument {
    invalid();
    invalid(const string& s);
  };
};

#if RINGING_AS_DLL && defined(_MSC_VER)
#pragma warning( pop )
#endif

RINGING_START_DETAILS_NAMESPACE
template <class ForwardIterator>
void skip_ws(ForwardIterator& first, ForwardIterator last) {
  while (first != last && isspace(*first)) ++first;
}

// Utility function to skip a single character separator, optionally
// surrounded with white space, and return true if it was found.
template <class ForwardIterator>
bool skip_sep(ForwardIterator& first, ForwardIterator last, char sep) {
  bool found = false;
  skip_ws(first, last);
  if (first != last && *first == sep) { found = true; ++first; }
  skip_ws(first, last);
  return found;
}
RINGING_END_DETAILS_NAMESPACE

template <class ForwardIterator>
ForwardIterator read_bell_expr(ForwardIterator first, ForwardIterator last) {
  if ( *first == '{' ) {  
    ++first;
    for ( int depth = 0; first < last && !(*first == '}' && depth == 0);
          ++first ) {
      if (*first == '{') ++depth;
      else if (*last == '}') --depth;
    }
    if ( first == last || *first != '}') throw bell::invalid();
    ++first;
  }
  return first;
}

// Take the place notation between start and last, and send it as
// a sequence of changes to out.
template <class OutputIterator, class ForwardIterator>
void interpret_pn(int num, ForwardIterator first, ForwardIterator last,
                  OutputIterator out) {
  RINGING_DETAILS_PREFIX skip_ws(first, last);
  while (first != last) {
    list<change> block;
    // See whether it's a symmetrical block or not
    bool symblock = (*first == '&');
    // Allow MicroSIRIL style '+' prefix
    if (*first == '&' || *first == '+') {
      ++first;
      // Arguably we shouldn't allow a . immediately after the & or +.
      RINGING_DETAILS_PREFIX skip_sep(first, last, '.');
    }
    while (first != last && (isalnum(*first) || *first == '-' 
                                             || *first == '{')) {
      if (*first == 'X' || *first == 'x' || *first == '-') {
        ++first;
        block.push_back(change(num,"X"));
      }
      else {
        ForwardIterator j(first);
        while (j != last) {
          if (*j == '{') j = read_bell_expr(j, last);
          else if (isalnum(*j) && *j != 'X' && *j != 'x') ++j;
          else break;
        }
        if (j != first) block.push_back(change(num, string(first, j)));
        first = j;
      }
      RINGING_DETAILS_PREFIX skip_sep(first, last, '.');
    }
    // Now output the block
    copy(block.begin(), block.end(), out);
    if (symblock && !block.empty()) {
      block.pop_back();
      copy(block.rbegin(), block.rend(), out);
    }
    if (first != last) {
      if (*first != ',') throw place_notation::invalid( string(1u, *first) );
      RINGING_DETAILS_PREFIX skip_sep(first, last, ',');
    }
  }
}

RINGING_END_NAMESPACE

#endif
