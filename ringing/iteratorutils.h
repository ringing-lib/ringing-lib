// -*- C++ -*- iteratorutils.h - Some trivial iterator utility functions
// Copyright (C) 2010 Richard Smith

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

// The next and prior function in this file are taken from the boost 
// utility library [see http://www.boost.org for details], and are under 
// the following copyright:

// (C) Copyright boost.org 1999. Permission to copy, use, modify, sell
// and distribute this software is granted provided this copyright
// notice appears in all copies. This software is provided "as is" without
// express or implied warranty, and with no claim as to its suitability for
// any purpose.

// $Id$

#ifndef RINGING_ITERATOR_UTILS_H
#define RINGING_ITERATOR_UTILS_H

#include <ringing/common.h>

#if RINGING_HAS_PRAGMA_ONCE
#pragma once
#endif

RINGING_START_NAMESPACE

RINGING_USING_STD

//  Helper functions for iterator classes not supporting
//  operator+ and operator-.

template <class ForwardIterator>
inline ForwardIterator next(ForwardIterator x) { return ++x; }

template <class BidirectionalIterator>
inline BidirectionalIterator prior(BidirectionalIterator x) { return --x; }

RINGING_END_NAMESPACE

#endif // RINGING_ITERATOR_UTILS_H
