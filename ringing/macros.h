// -*- C++ -*- macros.h - Macros to hide system specific constructs
//
// Copyright (C) 2001, 2002, 2003, 2006, 2007, 2008, 2009, 2010
// Martin Bright <martin@boojum.org.uk> and
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

// Part of this file is taken from the boost preprocessor library,
// version 1.33.1 [see http://www.boost.org for details], and is under
// the following copyright:

//  Copyright (C) 2001, Housemarque Oy http://www.housemarque.com
//
//  Distributed under the Boost Software License, Version 1.0. 
//  (See copy at http://www.boost.org/LICENSE_1_0.txt.)
//
//  Revised by Paul Mensonides (2002)

// $Id$

#ifndef RINGING_MACROS_H
#define RINGING_MACROS_H

#ifndef RINGING_INSIDE_COMMON_H
# error This header should not be included directly.
#endif

#ifdef _MSC_VER
# // Turn off ``identifier was truncated to 255 characters in the debug 
# // information'' warning generated in any template-rich code.
# pragma warning( disable: 4786 )
# // Turn off ``forcing value to bool 'true' or 'false' (performance warning)''
# // warning.  This warning is a complete waste of time.
# pragma warning( disable: 4800 )
#endif

// Support for the ringing namespace
#if RINGING_USE_NAMESPACES
# define RINGING_USING_NAMESPACE using namespace ringing;
# define RINGING_START_NAMESPACE namespace ringing {
# define RINGING_END_NAMESPACE }
# define RINGING_PREFIX ringing::
#else
# define RINGING_USING_NAMESPACE
# define RINGING_START_NAMESPACE
# define RINGING_END_NAMESPACE
# define RINGING_PREFIX
#endif

// Support for namespace std
#if RINGING_USE_STD
# define RINGING_USING_STD using namespace std;
# define RINGING_START_NAMESPACE_STD namespace std {
# define RINGING_END_NAMESPACE_STD }
# define RINGING_PREFIX_STD ::std::
#else
# define RINGING_USING_STD
# define RINGING_START_NAMESPACE_STD
# define RINGING_END_NAMESPACE_STD
# define RINGING_PREFIX_STD ::
#endif

// The ringing::details namespace: a namespace to enclose implementation 
// details that need to be in header files.
#if RINGING_USE_NAMESPACES
# define RINGING_USING_DETAILS using namespace details;
# define RINGING_DETAILS_PREFIX details::
# define RINGING_START_DETAILS_NAMESPACE namespace details {
# define RINGING_END_DETAILS_NAMESPACE }
#else
# define RINGING_USING_DETAILS 
# define RINGING_DETAILS_PREFIX 
# define RINGING_START_DETAILS_NAMESPACE
# define RINGING_END_DETAILS_NAMESPACE
#endif

// And for anonymous namespace
#if RINGING_USE_NAMESPACES
# define RINGING_START_ANON_NAMESPACE namespace {
# define RINGING_END_ANON_NAMESPACE }
#else
# define RINGING_START_ANON_NAMESPACE 
# define RINGING_END_ANON_NAMESPACE 
#endif

// Declare the ringing, the std namespace, and make all the names
// in namespace std in scope in namespace ringing.  This is necessary 
// so that all of the files in programs that use the library can do
// RINGING_USING_NAMESPACE, which is necessary to get around another
// Visual Studio 6 namespace bugs. 
RINGING_START_NAMESPACE_STD RINGING_END_NAMESPACE_STD
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE

#if _MSC_VER <= 1200 && RINGING_USE_STD && RINGING_USE_NAMESPACES
// Visual Studio 6 has the amusing bug that the third time you enter
// a namespace and process a using directive, the using directive
// is completely ignored.  This only appears to happen the third time,
// so we safely get this out of its system.  Any more sane suggestions
// appreciated...
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE

// Suddenly, three are no longer enough, and we find ourselves needing
// seven of these using declarations!
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE

// ... or nine as we now appear to have reached.  I *really* don't 
// understand what's going on here.
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE

//  And now for something completely different.   It's...
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
#endif

// Are we running on Windows?
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
#  define RINGING_WINDOWS 1
#else
#  define RINGING_WINDOWS 0
#endif

// Macros ready for Windows DLL support.
#if DLL_EXPORT && RINGING_AS_DLL
#  // Symbols exported from the dll:
#  if RINGING_EXPORTS
#   define RINGING_API __declspec(dllexport)
#  else
#   define RINGING_API __declspec(dllimport)
#  endif
#  // Explicit instantiations of some STL containers:
#  ifdef RINGING_DO_STL_INSTANTIATIONS
#   define RINGING_EXPLICIT_TEMPLATE template 
#  else
#   // Disable warnings about extern before explicit template instantiation
#   pragma warning(disable: 4231)
#   define RINGING_EXPLICIT_TEMPLATE extern template
#  endif
#else
# define RINGING_API
# define RINGING_EXPLICIT_TEMPLATE	Error_This_must_be_ifdefd_out
#endif

// Shorthand for all of the STL container instantiations
#define RINGING_EXPLICIT_STL_TEMPLATE \
  RINGING_EXPLICIT_TEMPLATE class RINGING_API RINGING_PREFIX_STD 
#define RINGING_EXPLICIT_RINGING_TEMPLATE \
  RINGING_EXPLICIT_TEMPLATE class RINGING_API RINGING_PREFIX 


// Shorthand macro for doing a delegating std::swap specialisation:
#if RINGING_USE_TEMPLATE_FUNCTION_SPECIALISATION
#define RINGING_DELEGATE_STD_SWAP(T)					\
RINGING_START_NAMESPACE_STD						\
  template <> inline RINGING_API					\
  void swap<RINGING_PREFIX T>(RINGING_PREFIX T &a, RINGING_PREFIX T &b)	\
  { a.swap(b); }							\
RINGING_END_NAMESPACE_STD
#else
#define RINGING_DELEGATE_STD_SWAP(T)
#endif

// Shorthand macro for doing a delegating std::hash specialisation:
#if RINGING_HAS_STD_HASH
#define RINGING_DELEGATE_STD_HASH(T)                                    \
RINGING_START_NAMESPACE_STD                                             \
  template <> struct RINGING_API hash<RINGING_PREFIX T>			\
    : public unary_function<T, size_t> 					\
  {									\
    size_t operator()(T const& val) const { return val.hash(); }	\
  };									\
RINGING_END_NAMESPACE_STD
#else
#define RINGING_DELEGATE_STD_HASH(T)
#endif

// The following construct doesn't work in all compilers:
//   class base {
//   protected:
//     class impl_base {};
//   };
//
//   class derived : public base { 
//     class impl;
//   };
//
//   class derived::impl : public base::impl_base {};
#if RINGING_PROTECTED_MEMBER_BASES
# define RINGING_PROTECTED_IMPL protected
#else 
# define RINGING_PROTECTED_IMPL public
#endif

#if RINGING_HAVE_STD_ITERATOR
# define RINGING_STD_ITERATOR( category, type )			\
    RINGING_PREFIX_STD iterator< category, type, ptrdiff_t >
# define RINGING_STD_CONST_ITERATOR( category, type )		\
    RINGING_PREFIX_STD iterator< category, type, ptrdiff_t,	\
                                 const type *, const type & >
# define RINGING_STD_OUTPUT_ITERATOR( type )			\
    RINGING_PREFIX_STD iterator< output_iterator_tag, void, 	\
				 void, void, void >

#elif defined(_MSC_VER) && _MSC_VER <= 1200 
  // MSVC-5 has a three-argument iterator, but it is important 
  // to use it because otherwise some STL algorithms do not 
  // properly work (they assume all iterators derive from it).
# define RINGING_STD_ITERATOR( category, type )			\
    RINGING_PREFIX_STD iterator< category, type, ptrdiff_t >
# define RINGING_STD_CONST_ITERATOR( category, type )		\
    RINGING_STD_ITERATOR( category, type )
# define RINGING_STD_OUTPUT_ITERATOR( type )			\
    RINGING_PREFIX_STD iterator< output_iterator_tag, void, void >

#else
  // Need some class just so that the program is syntactically correct
  class ringing_dummy_iterator {};
# define RINGING_STD_ITERATOR( category, type )       ringing_dummy_iterator
# define RINGING_STD_CONST_ITERATOR( category, type ) ringing_dummy_iterator
# define RINGING_STD_OUTPUT_ITERATOR( type )          ringing_dummy_iterator

#endif

// Compilers that prematurely instantiate members of templates
// frequently need comparison operators setting up.
#if RINGING_PREMATURE_MEMBER_INSTANTIATION
# define RINGING_FAKE_COMPARATORS(classname)		\
  bool operator==(const classname &) const;		\
  bool operator!=(const classname &) const;		\
  bool operator< (const classname &) const;		\
  bool operator> (const classname &) const;		\
  bool operator<=(const classname &) const;		\
  bool operator>=(const classname &) const;
# define RINGING_FAKE_DEFAULT_CONSTRUCTOR(classname)	\
  classname();
# define RINGING_FAKE_ASSIGNMENT(classname)		\
  classname &operator=( const classname & );
#else
# define RINGING_FAKE_COMPARATORS(classname)
# define RINGING_FAKE_DEFAULT_CONSTRUCTOR(classname)
# define RINGING_FAKE_ASSIGNMENT(classname)
#endif

// Cygwin's version of gcc 3.2 (20020927 build) has a broken
// pragma interface / implementation.  This really ought to
// be wrapped up into a autoconf test, but I can't make
// a simple test case.
// This is also broken in gcc 4.0.x (x <= 2).  Hopefully it might
// be fixed in 4.0.3 and 4.1, but both are still at the release
// candidate stage at the moment, so I'll leave it as a test of
// all gcc 4.x.y.
#if defined(__GNUG__) && !defined(__CYGWIN__) && __GNUG__ < 4
# define RINGING_HAS_PRAGMA_INTERFACE 1
#else
# define RINGING_HAS_PRAGMA_INTERFACE 0
#endif

// MSVC <= 6.0 doesn't scope variables in for loops properly.
// It treats
//   for (int i=0; i<n; ++i)  
// as 
//   int i;
//   for (i=0; i<n; ++i)
// which has obvious issues with scoping.  To get around this
// without scattering dozens of extra braces around the code,
// we use this heinous macro hack:
#if defined(_MSC_VER) && _MSC_VER <= 1200 
# define for if (false) {} else for
#endif

// Support for #pragma once.  This can significantly speed up 
// compilation with MSVC. It is supported in gcc, though produces
// a warning prior to 3.4, but makes little difference to compile
// times.  It ought to become an autoconf test, but that would 
// require writing an autoconf test with multiple files, and that's 
// non-trivial.
#if defined(_MSC_VER) && _MSC_VER > 1000
# define RINGING_HAS_PRAGMA_ONCE 1
#elif defined(__GNUG__) && __GNUG__ >= 4
# define RINGING_HAS_PRAGMA_ONCE 1
#else
# define RINGING_HAS_PRAGMA_ONCE 0
#endif

// Handle optional backwards compatibility
#define RINGING_MAKE_VERSION(major, minor, patch) 			\
  ( (major)*10000 + (minor)*100 + patch )
#define RINGING_BACKWARDS_COMPATIBLE(major,minor,patch) 		\
  ( defined(RINGING_COMPATIBLE_WITH) && 				\
    RINGING_MAKE_VERSION(major,minor,patch) >= RINGING_COMPATIBLE_WITH )

// Edit this line to enable/disable compatibility
#define RINGING_COMPATIBLE_WITH RINGING_MAKE_VERSION(0,4,0)

// RINGING_CONCAT -- concatenate two preprocessor tokens
#define RINGING_CONCAT(a, b) RINGING_CONCAT_I(a, b)
#
#if !defined(_MSC_VER)
# define RINGING_CONCAT_I(a, b) a ## b
#else
# define RINGING_CONCAT_I(a, b) RINGING_CONCAT_II(a ## b)
# define RINGING_CONCAT_II(res) res
#endif

// Long long support
#if RINGING_HAVE_LONG_LONG
# define RINGING_LLONG  long long
# define RINGING_ULLONG unsigned long long
#else
# define RINGING_LLONG  long
# define RINGING_ULLONG unsigned long
#endif

// istringstream / istrstream support
#if RINGING_USE_STRINGSTREAM
# define RINGING_ISTRINGSTREAM istringstream
#else
# define RINGING_ISTRINGSTREAM istrstream
#endif

// Abstract throwing versus returning an error value
#if RINGING_USE_EXCEPTIONS
#  define RINGING_THROW_OR_EXPR(t, e) throw t
#else
#  define RINGING_THROW_OR_EXPR(t, e) e
#endif
#define RINGING_THROW_OR_RETURN(t, r)   RINGING_THROW_OR_EXPR(t, return r)
#define RINGING_THROW_OR_RETURN_VOID(t) RINGING_THROW_OR_EXPR(t, return)

#endif // RINGING_MACROS_H
