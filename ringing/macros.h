// macros.h  -*- C++ -*-

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
# define RINGING_START_NAMESPACE namespace ringing {
# define RINGING_END_NAMESPACE };
# define RINGING_PREFIX ringing::
#else
# define RINGING_START_NAMESPACE
# define RINGING_END_NAMESPACE
# define RINGING_PREFIX
#endif

// Support for namespace std
#if RINGING_USE_STD
# define RINGING_USING_STD using namespace std;
# define RINGING_START_NAMESPACE_STD namespace std {
# define RINGING_END_NAMESPACE_STD };
# define RINGING_PREFIX_STD std::
#else
# define RINGING_USING_STD
# define RINGING_START_NAMESPACE_STD
# define RINGING_END_NAMESPACE_STD
# define RINGING_PREFIX_STD ::
#endif

// The ringing::details namespace: a namespace to enclose implementation 
// details that need to be in header files.
#if RINGING_USE_NAMESPACES
# define RINGING_DETAILS_PREFIX details::
# define RINGING_START_DETAILS_NAMESPACE namespace details {
# define RINGING_END_DETAILS_NAMESPACE }
#else
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

#if _MSC_VER == 1200 && RINGING_USE_STD && RINGING_USE_NAMESPACES
// Visual Studio 6 has the amusing bug that the third time you enter
// a namespace and process a using directive, the using directive
// is completely ignored.  This only appears to happen the third time,
// so we safely get this out of its system.  Any more sane suggestions
// appreciated...
RINGING_START_NAMESPACE_STD RINGING_END_NAMESPACE_STD
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
RINGING_START_NAMESPACE RINGING_USING_STD RINGING_END_NAMESPACE
#endif

// Macros ready for Windows DLL support.
#if RINGING_AS_DLL
#  // Symbols exported from the dll:
#  if RINGING_EXPORTS
#   define RINGING_API __declspec(dllexport)
#  else
#   define RINGING_API __declspec(dllimport)
#  endif
#  // Explicit instantiations of some STL containers:
#  ifdef RINGING_IN_STL_STUB
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


// Shorthand macro for doing a delegating std::swap specialisation:
#if RINGING_USE_TEMPLATE_FUNCTION_SPECIALISATION
#define RINGING_DELEGATE_STD_SWAP(T)					\
RINGING_START_NAMESPACE_STD						\
  template <> inline RINGING_API					\
  void swap<RINGING_PREFIX T>(RINGING_PREFIX T &a, RINGING_PREFIX T &b)	\
  { a.swap(b); }							\
RINGING_END_NAMESPACE_STD
#else
#define RINGING_DELEGATE_STD_SWAP(type)
#endif

#endif

