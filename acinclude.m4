dnl -*- M4 -*- acinclude.m4 - Tests for compiler functionality

dnl Process this file with aclocal to produce aclocal.m4.

dnl Copyright (C) 2001, 2002 Martin Bright <martin@boojum.org.uk>
dnl and Richard Smith <richard@ex-parrot.com>

dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.

dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

dnl $Id$

dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_EXCEPTIONS
dnl
dnl If the C++ compiler supports exceptions handling (try,
dnl throw and catch), define USE_EXCEPTIONS.
dnl
dnl @author Luc Maisonobe  (slightly modified)
dnl
AC_DEFUN([AC_CXX_EXCEPTIONS],
  [AC_CACHE_CHECK(whether the compiler supports exceptions,
  ac_cv_cxx_exceptions,
  [AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   AC_TRY_COMPILE(,[try { throw  1; } catch (int i) { return i; }],
   ac_cv_cxx_exceptions=yes, ac_cv_cxx_exceptions=no)
   AC_LANG_RESTORE
  ])
  if test "$ac_cv_cxx_exceptions" = yes; then
    USE_EXCEPTIONS=1
  else
    USE_EXCEPTIONS=0
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_NAMESPACES
dnl
dnl Does the compiler properly support namespaces?  If so, define
dnl USE_NAMESPACES.
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_CXX_NAMESPACES],
  [AC_CACHE_CHECK(
    [whether the compiler implements namespaces],
    [ac_cv_cxx_namespaces],
    [AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_TRY_COMPILE(
      [
	namespace bar
	{
	  // The use of the template class is important because it causes 
	  // msvc-5 to fail this test by trying to find A in namespace bar 
          // during the instantiation of the default argument of B::baz 
	  // (or something equally silly).  This is important because the 
	  // STL frequently does this.
	  //
	  template <class T> struct B 
	  { 
	    void baz(const T & = T());
	  };
	}
	namespace foo 
	{
	  using namespace bar;
	  class A {};
	  void f() { B<A> b; b.baz(); }
	}
      ],
      [ foo::f(); ],
      ac_cv_cxx_namespaces=yes,
      ac_cv_cxx_namespaces=no)
     AC_LANG_RESTORE]
  )
  if test "$ac_cv_cxx_namespaces" = yes ; then
    USE_NAMESPACES=1;
  else
    USE_NAMESPACES=0;
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_FUNCTION_TEMPLATE_SPECIALISATION
dnl
dnl Does the compiler support function template specialisation?
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_CXX_FUNCTION_TEMPLATE_SPECIALISATION],
  [AC_CACHE_CHECK(
    [whether compiler supports template function specialisation],
    [ac_cv_cxx_template_function_specialisation],
    [AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_TRY_COMPILE(
      [
	template <class T> void foo(const T &) {}
	template <> void foo<int>(const int &) {}
      ],  
      [ foo((int)1); ],
      ac_cv_cxx_template_function_specialisation=yes,
      ac_cv_cxx_template_function_specialisation=no)
     AC_LANG_RESTORE]
  )
  if test "$ac_cv_cxx_template_function_specialisation" = yes ; then
    USE_TEMPLATE_FUNCTION_SPECIALISATION=1
  else
    USE_TEMPLATE_FUNCTION_SPECIALISATION=0
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_PREMATURE_MEMBER_INSTANTIATION
dnl
dnl Check to see whether instantiating a class instantiates its 
dnl members' definitions or just declarations
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_CXX_PREMATURE_MEMBER_INSTANTIATION],
  [AC_CACHE_CHECK(
    [whether class instantiation prematurely instantiates members],
    [ac_cv_cxx_premature_member_instantiation],
    [AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_TRY_COMPILE(
      [
	class A {};
	template <class T> struct foo { 
	  bool bar() const { return a < b; }
	  T a, b;
	};
      ],
      [ foo<A> f; return 0; ],
      ac_cv_cxx_premature_member_instantiation=no,
      ac_cv_cxx_premature_member_instantiation=yes)
     AC_LANG_RESTORE]
  )
  if test "$ac_cv_cxx_premature_member_instantiation" = yes ; then
    PREMATURE_MEMBER_INSTANTIATION=1
  else
    PREMATURE_MEMBER_INSTANTIATION=0
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_PROTECTED_MEMBER_BASES
dnl
dnl Check to see whether a member class can be derived from a protected
dnl member class of the outer class.
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_CXX_PROTECTED_MEMBER_BASES],
  [AC_CACHE_CHECK(
    [whether member classes' base classes may be protected],
    [ac_cv_cxx_protected_member_bases],
    [AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_TRY_COMPILE(
      [
	class A {
	protected:
	  class B {};
	};

	class C : public A {
	  class D;
	};

	class C::D : public A::B {};
      ],
      [ return 0; ],
      ac_cv_cxx_protected_member_bases=yes,
      ac_cv_cxx_protected_member_bases=no)
     AC_LANG_RESTORE]
  )
  if test "$ac_cv_cxx_protected_member_bases" = yes ; then
    PROTECTED_MEMBER_BASES=1
  else
    PROTECTED_MEMBER_BASES=0
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_USE_STD
dnl
dnl Check to see whether the standard includes put their things in
dnl the std namespace
dnl
dnl @author Martin Bright <martin@boojum.org.uk>
dnl
AC_DEFUN([AC_CXX_USE_STD],
  [AC_CACHE_CHECK(
    [whether standard includes use namespace std],
    [ac_cv_cxx_std],
    [AC_REQUIRE([AC_CXX_OLD_INCLUDES])
     AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_TRY_COMPILE(
      [#include] $list_name [
      ], 
      [ 
        std::list<int> l; l.push_back(3);
        std::list<int>::iterator i = l.begin(); ++i;
        return 0;
      ],
      ac_cv_cxx_std=yes, 
      ac_cv_cxx_std=no)
     AC_LANG_RESTORE]
  )
  if test "$ac_cv_cxx_std" = yes; then
    USE_STD=1;
    std_using_directive="using namespace std;"
  else
    USE_STD=0;
    std_using_directive=
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_HAVE_OLD_IOSTREAMS
dnl
dnl Check whether it is a standard or a pre-standard iostream implementation
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_CXX_HAVE_OLD_IOSTREAMS],
  [AC_CACHE_CHECK(
    [whether the compiler implements standard iostreams],
    [ac_cv_cxx_std_iostreams],
    [AC_REQUIRE([AC_CXX_USE_STD])
     AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_TRY_COMPILE(
      [#include <iostream>
      ] $std_using_directive,
      [ typedef basic_ostream<char> my_ostream; ],
      ac_cv_cxx_std_iostreams=yes,
      ac_cv_cxx_std_iostreams=no)
     AC_LANG_RESTORE]
  )
  if test "$ac_cv_cxx_std_iostreams" = yes ; then
    iostream_name="<iostream>"
    sstream_name="<sstream>"
    strstream_name="<strstream>"
    HAVE_OLD_IOSTREAMS=0
  else
    AC_CACHE_CHECK(
      [whether the compiler implements pre-standard iostreams],
      [ac_cv_cxx_old_iostreams],
      [AC_REQUIRE([AC_CXX_USE_STD])
       AC_LANG_SAVE
       AC_LANG_CPLUSPLUS
       AC_TRY_COMPILE(
	[#include <iostream.h>
	] $std_using_directive,
	[ typedef ostream my_ostream; ],
	ac_cv_cxx_old_iostreams=yes,
	ac_cv_cxx_old_iostreams=no)
       AC_LANG_RESTORE]
    )
    if test "$ac_cv_cxx_old_iostreams" = yes ; then
      iostream_name="<iostream.h>"
      sstream_name="<sstream.h>"
      strstream_name="<strstream.h>"
      HAVE_OLD_IOSTREAMS=1
    else
     AC_MSG_WARN([Can't find either <iostream.h> or <iostream>.])
     can_build=no
    fi
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_OLD_INCLUDES
dnl
dnl Now we check to see what the C++ headers are called
dnl
dnl @author Martin Bright <martin@boojum.org.uk>
dnl
AC_DEFUN([AC_CXX_OLD_INCLUDES],
  [AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   AC_CHECK_HEADER([vector], 
    OLD_INCLUDES=0
    iterator_name="<iterator>"
    list_name="<list>",
    [AC_CHECK_HEADER([vector.h], 
      OLD_INCLUDES=1
      iterator_name="<iterator.h>"
      list_name="<list.h>")
    ])
   AC_LANG_RESTORE
   if test -z "$OLD_INCLUDES" ; then
     AC_MSG_WARN([Can't find either <vector> or <vector.h>.])
     can_build=no
   fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_OLD_C_INCLUDES
dnl
dnl Now we check to see what the C++ C headers are called
dnl
dnl @author Martin Bright <martin@boojum.org.uk>
dnl
AC_DEFUN([AC_CXX_OLD_C_INCLUDES],
  [AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   AC_CHECK_HEADER([cstdio], OLD_C_INCLUDES=0, OLD_C_INCLUDES=1)
   AC_LANG_RESTORE
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_HAVE_STD_ITERATOR
dnl
dnl Check to see whether the STL has a compliant std::iterator
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_CXX_HAVE_STD_ITERATOR],
  [AC_CACHE_CHECK(
    [whether the STL has a compliant std::iterator],
    [ac_cv_cxx_std_iterator],
    [AC_REQUIRE([AC_CXX_OLD_INCLUDES])
     AC_REQUIRE([AC_CXX_USE_STD])
     AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_TRY_COMPILE(
      [ #include ] $iterator_name [
      ] $std_using_directive [
	class iter
	  : public iterator< input_iterator_tag, int, ptrdiff_t, int *, int & >
	{};
      ],
      [ return 0; ],
      ac_cv_cxx_std_iterator=yes,
      ac_cv_cxx_std_iterator=no)
     AC_LANG_RESTORE]
  )
  if test "$ac_cv_cxx_std_iterator" = yes ; then
    HAVE_STD_ITERATOR=1
  else
    HAVE_STD_ITERATOR=0
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CXX_USE_STRINGSTREAM
dnl
dnl Check to see whether the STL has a compliant std::ostringstream
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_CXX_USE_STRINGSTREAM],
 [AC_CACHE_CHECK(
    [whether the STL has stringstreams],
    [ac_cv_cxx_stringstream],
    [AC_REQUIRE([AC_CXX_HAVE_OLD_IOSTREAMS])
     AC_REQUIRE([AC_CXX_USE_STD])
     AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_TRY_COMPILE(
      [ #include ] $sstream_name [
	#include <string>
      ] $std_using_directive,
      [ 
	ostringstream os;
	os << "Hello, world: " << 42 << "!";
	string s( os.str() ); 
      ],
      ac_cv_cxx_stringstream=yes,
      ac_cv_cxx_stringstream=no)
     AC_LANG_RESTORE]
  )
  if test "$ac_cv_cxx_stringstream" = yes ; then
    USE_STRINGSTREAM=1
  else
    AC_CACHE_CHECK(
      [whether the STL has strstreams],
      [ac_cv_cxx_strstream],
      [AC_REQUIRE([AC_CXX_HAVE_OLD_IOSTREAMS])
       AC_REQUIRE([AC_CXX_USE_STD])
       AC_LANG_SAVE
       AC_LANG_CPLUSPLUS
       AC_TRY_COMPILE(
        [ #include ] $strstream_name [
	  #include <string>
	] $std_using_directive,
	[
	  ostrstream os;
	  os << "Hello, world: " << 42 << "!";
	  string s( os.str(), os.pcount() );
	  os.freeze(0);
	],
	ac_cv_cxx_strstream=yes,
	ac_cv_cxx_strstream=no)
       AC_LANG_RESTORE]
    )
    if test "$ac_cv_cxx_strstream" = yes ; then
      USE_STRINGSTREAM=0
    else
      AC_MSG_WARN([Can't find either ]$sstream_name[ or ]$strstream_name[.])
      can_build=no
    fi
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_CHECK_CXX_LIB(LIBRARY, INCLUDES, MAIN, ACTION-IF-FOUND,
dnl                            [ACTION-IF-NOT-FOUND], [OTHER-LIBRARIES])
dnl
dnl Check to see whether a given library exists and is usable from C++
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_CHECK_CXX_LIB],
 [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_check_cxx_lib_save_LIBS=$LIBS
  LIBS="-l$1 $6 $LIBS"
  AC_TRY_LINK([$2],[$3],[$4],[$5])
  LIBS=$ac_check_cxx_lib_save_LIBS
  AC_LANG_RESTORE
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_USE_READLINE
dnl
dnl See whether we've got GNU readline installed
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_USE_READLINE],
 [AC_ARG_WITH(
    readline,
    AC_HELP_STRING([--with-readline], [support fancy command line editing]),
    ac_cv_use_readline=$withval
  )
  if test -z "$ac_cv_use_readline" ; then
    AC_CACHE_CHECK(
      [for GNU readline library],
      [ac_cv_use_readline],
      [AC_CHECK_CXX_LIB(
	readline, [#include <readline/readline.h>
	], [ readline(">"); ],
	ac_cv_use_readline=yes,
	ac_cv_use_readline=no
    )])
  fi
  if test "$ac_cv_use_readline" = yes ; then
    READLINE_LIBS=[-lreadline]
    USE_READLINE=1
  else
    READLINE_LIBS=[]
    USE_READLINE=0
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_USE_XERCES
dnl
dnl See whether we've got the Apache Xerces library installed
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_USE_XERCES],
 [AC_ARG_WITH(
    xerces,
    AC_HELP_STRING([--with-xerces], [support XML libraries with Xerces]),
    ac_cv_use_xerces=$withval
  )
  if test -z "$ac_cv_use_xerces" ; then
    AC_CACHE_CHECK(
      [for Apache xerces-c library],
      [ac_cv_use_xerces],
      [AC_CHECK_CXX_LIB(
        xerces-c, 
        [#include <xercesc/util/XercesDefs.hpp>
         #include <xercesc/util/PlatformUtils.hpp>
         XERCES_CPP_NAMESPACE_USE
        ], [ XMLPlatformUtils::Initialize(); ],
        ac_cv_use_xerces=yes,
        ac_cv_use_xerces=no
    )])
  fi
  if test "$ac_cv_use_xerces" = yes ; then
    XERCES_LIBS=[-lxerces-c]
    USE_XERCES=1
  else
    XERCES_LIBS=[]
    USE_XERCES=0
  fi
])
dnl --------------------------------------------------------------------------
dnl @synopsis AC_IS_MSVC
dnl
dnl See whether we're using Microsoft Visual Studio
dnl
dnl @author Richard Smith <richard@ex-parrot.com>
dnl
AC_DEFUN([AC_IS_MSVC],
 [AC_CACHE_CHECK(
    [if the compile is Microsoft Visual C++],
    [ac_cv_msvc],
    [AC_TRY_COMPILE(
      [ #if !defined(_MSC_VER)
	#error "Not using MSVC"
	#endif
      ], [], 
      ac_cv_msvc=yes,
      ac_cv_msvc=no)]
  )
  if test "$ac_cv_msvc" = yes ; then
    IS_MSVC=1
  else
    IS_MSVC=0
  fi
])