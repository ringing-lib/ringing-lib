dnl -*- M4 -*- configure.in - Autoconf script for the Ringing Class Library

dnl Process this file with autoconf to produce a configure script.

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


AC_INIT(ringing/method.cpp)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(ringing-lib, 0.2.7)
AM_CONFIG_HEADER(config.h)
AC_SUBST(ac_aux_dir)

can_build=yes

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC


dnl --------------------------------------------------------------------------
dnl
dnl Now we check to see what the C++ headers are called
dnl
AC_LANG_CPLUSPLUS
AC_CHECK_HEADER([vector], RINGING_OLD_INCLUDES=0
  iostream_name="<iostream>"
  list_name="<list>",
  [AC_CHECK_HEADER([vector.h], RINGING_OLD_INCLUDES=1
    iostream_name="<iostream.h>"
    list_name="<list.h>",
    [AC_MSG_WARN([Can't find either <vector> or <vector.h>.  This means
that you don't appear to have the standard C++ libraries installed, which is 
necessary if you wish to compile or use the Ringing Class Library.])
    can_build=no])])
AC_SUBST(RINGING_OLD_INCLUDES)
AC_CHECK_HEADER([cstdio], RINGING_OLD_C_INCLUDES=0, RINGING_OLD_C_INCLUDES=1)
AC_SUBST(RINGING_OLD_C_INCLUDES)

dnl --------------------------------------------------------------------------
dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_CXX_EXCEPTIONS
if test "$ac_cv_cxx_exceptions" = yes; then
  RINGING_USE_EXCEPTIONS=1;
else
  RINGING_USE_EXCEPTIONS=0;
fi
AC_SUBST(RINGING_USE_EXCEPTIONS)


dnl --------------------------------------------------------------------------
dnl
dnl Check if compile supports namespaces
dnl
dnl The use of the template class is important because it causes msvc-5
dnl to fail this test by trying to find A in namespace bar during the
dnl instantiation of the default argument of B::baz (or something equally 
dnl silly).  This is important because the STL frequently does this.
dnl
AC_CACHE_CHECK(
  [whether the compiler implements namespaces],
  [ac_cv_cxx_namespaces],
  [AC_TRY_COMPILE(
    [
      namespace bar
      {
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
    ac_cv_cxx_namespaces=no
  )]
)
if test "$ac_cv_cxx_namespaces" = yes ; then
  RINGING_USE_NAMESPACES=1;
else
  RINGING_USE_NAMESPACES=0;
fi
AC_SUBST(RINGING_USE_NAMESPACES)


dnl --------------------------------------------------------------------------
dnl
dnl Check if platform supports template function specialisation
dnl
AC_CACHE_CHECK(
  [whether compiler supports template function specialisation],
  [ac_cv_cxx_template_function_specialisation],
  [AC_TRY_COMPILE(
    [
      template <class T> void foo(const T &) {}
      template <> void foo<int>(const int &) {}
    ],  
    [ foo((int)1); ],
    ac_cv_cxx_template_function_specialisation=yes,
    ac_cv_cxx_template_function_specialisation=no
  )]
)
if test "$ac_cv_cxx_template_function_specialisation" = yes ; then
  RINGING_USE_TEMPLATE_FUNCTION_SPECIALISATION=1
else
  RINGING_USE_TEMPLATE_FUNCTION_SPECIALISATION=0
fi
AC_SUBST(RINGING_USE_TEMPLATE_FUNCTION_SPECIALISATION)


dnl --------------------------------------------------------------------------
dnl
dnl Check to see whether instantiating a class instantiates its 
dnl members' definitions or just declarations
dnl
AC_CACHE_CHECK(
  [whether class instantiation prematurely instantiates members],
  [ac_cv_cxx_premature_member_instantiation],
  [AC_TRY_COMPILE(
    [
      class A {};
      template <class T> struct foo { 
        bool bar() const { return a < b; }
        T a, b;
      };
    ],
    [ foo<A> f; return 0; ],
    ac_cv_cxx_premature_member_instantiation=no,
    ac_cv_cxx_premature_member_instantiation=yes
  )]
)
if test "$ac_cv_cxx_premature_member_instantiation" = yes ; then
  RINGING_PREMATURE_MEMBER_INSTANTIATION=1
else
  RINGING_PREMATURE_MEMBER_INSTANTIATION=0
fi
AC_SUBST(RINGING_PREMATURE_MEMBER_INSTANTIATION)


dnl --------------------------------------------------------------------------
dnl
dnl Check to see whether a member class can be derived from a protected
dnl member class of the outer class.
dnl
AC_CACHE_CHECK(
  [whether member classes' base classes may be protected],
  [ac_cv_cxx_protected_member_bases],
  [AC_TRY_COMPILE(
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
    ac_cv_cxx_protected_member_bases=no
  )]
)
if test "$ac_cv_cxx_protected_member_bases" = yes ; then
  RINGING_PROTECTED_MEMBER_BASES=1
else
  RINGING_PROTECTED_MEMBER_BASES=0
fi
AC_SUBST(RINGING_PROTECTED_MEMBER_BASES)


dnl --------------------------------------------------------------------------
dnl
dnl Check to see whether the standard includes put their things in
dnl the std namespace
dnl
AC_CACHE_CHECK(whether standard includes use namespace std,
ac_cv_cxx_std,
[AC_TRY_COMPILE([#include] $iostream_name, [std::cout << 'x'; return 0;],
  ac_cv_cxx_std=yes, ac_cv_cxx_std=no)]
)
if test "$ac_cv_cxx_std" = yes; then
  RINGING_USE_STD=1;
  std_using_directive="using namespace std;"
else
  RINGING_USE_STD=0;
  std_using_directive=
fi
AC_SUBST(RINGING_USE_STD)


dnl --------------------------------------------------------------------------
dnl
dnl Now whether it is a standard or a pre-standard iostream implementation
dnl
AC_CACHE_CHECK(
  [whether the compiler implements standard iostreams],
  [ac_cv_cxx_std_iostreams],
  [AC_TRY_COMPILE(
    [#include <iostream>
    ] $std_using_directive,
    [ typedef basic_ostream<char> my_ostream; ],
    ac_cv_cxx_std_iostreams=yes,
    ac_cv_cxx_std_iostreams=no
  )]
)
if test "$ac_cv_cxx_std_iostreams" = yes ; then
  RINGING_HAVE_OLD_IOSTREAMS=0
else
  AC_CACHE_CHECK(
    [whether the compiler implements pre-standard iostreams],
    [ac_cv_cxx_old_iostreams],
    [AC_TRY_COMPILE(
      [#include <iostream.h>
      ] $std_using_directive,
      [ typedef ostream my_ostream; ],
      ac_cv_cxx_old_iostreams=yes,
      ac_cv_cxx_old_iostreams=no
    )]
  )
  if test "$ac_cv_cxx_old_iostreams" = yes ; then
    RINGING_HAVE_OLD_IOSTREAMS=1
  else
    AC_MSG_WARN(
[You appear not to have either standard iostreams or pre-standard iostreams
avaliable.  It is necessary to have one of these to use the Ringing Class 
Library.]);
    can_build=no;
  fi
fi
AC_SUBST(RINGING_HAVE_OLD_IOSTREAMS)

dnl --------------------------------------------------------------------------
dnl
dnl Check to see whether the STL has a compliant std::iterator
dnl
AC_CACHE_CHECK(
  [whether the STL has a compliant std::iterator],
  [ac_cv_cxx_std_iterator],
  [AC_TRY_COMPILE(
    [
      #if ] $RINGING_OLD_INCLUDES [
      #  include <iterator.h>
      #else
      #  include <iterator>
      #endif
      ] $std_using_directive [

      class iter
        : public iterator< input_iterator_tag, int, ptrdiff_t, int *, int & >
      {};
    ],
    [ return 0; ],
    ac_cv_cxx_std_iterator=yes,
    ac_cv_cxx_std_iterator=no
  )]
)
if test "$ac_cv_cxx_std_iterator" = yes ; then
  RINGING_HAVE_STD_ITERATOR=1
else
  RINGING_HAVE_STD_ITERATOR=0
fi
AC_SUBST(RINGING_HAVE_STD_ITERATOR)


dnl --------------------------------------------------------------------------
dnl
dnl Check to see whether the STL has a compliant std::ostringstream
dnl
AC_CACHE_CHECK(
  [whether the STL has stringstreams],
  [ac_cv_cxx_stringstream],
  [AC_TRY_COMPILE(
    [
      #if ] $RINGING_OLD_INCLUDES [
      #  include <sstream.h>
      #else
      #  include <sstream>
      #endif
      #include <string>
      ] $std_using_directive [
    ],
    [ 
      ostringstream os;
      os << "Hello, world: " << 42 << "!";
      string s( os.str() ); 
    ],
    ac_cv_cxx_stringstream=yes,
    ac_cv_cxx_stringstream=no
  )]
)
if test "$ac_cv_cxx_stringstream" = yes ; then
  RINGING_USE_STRINGSTREAM=1
else
  AC_CACHE_CHECK(
    [whether the STL has strstreams],
    [ac_cv_cxx_strstream],
    [AC_TRY_COMPILE(
      [
	#if ] $RINGING_OLD_INCLUDES [
	#  include <strstream.h>
	#else
	#  include <strstream>
	#endif
        #include <string>
	] $std_using_directive [
      ],
      [
        ostrstream os;
        os << "Hello, world: " << 42 << "!";
        string s( os.str(), os.pcount() );
        os.freeze(0);
      ],
      ac_cv_cxx_strstream=yes,
      ac_cv_cxx_strstream=no
    )]
  )
  if test "$ac_cv_cxx_strstream" = yes ; then
    RINGING_USE_STRINGSTREAM=0
  else
    AC_MSG_WARN([You appear not to have either a working stringstream class 
or a working strstream class.  It is necessary to have one of these to use the
Ringing Class Library.]);
    can_build=no;
  fi
fi
AC_SUBST(RINGING_USE_STRINGSTREAM)

dnl --------------------------------------------------------------------------
dnl
dnl Now check whether the STL seems to work
dnl
AC_CACHE_CHECK(whether the Standard Template Library works,
  ac_cv_cxx_stl,
  [AC_TRY_COMPILE(
    [#include] $list_name [
    ] $std_using_directive,
    [
      list<int> l; l.push_back(3);
      list<int>::iterator i = l.begin(); ++i; 
      return 0;
    ],
    ac_cv_cxx_stl=yes, ac_cv_cxx_stl=no)]
)
if test "$ac_cv_cxx_stl" = no; then
  AC_MSG_WARN([You do not appear to have a working version of the Standard
Template Library (STL), which is necessary for compiling and using the
Ringing Class Library.]);
  can_build=no;
fi

if test "$can_build" = no; then
  AC_MSG_WARN([Some problems were encountered which mean that you will not
be able to build the Ringing Class Library.  You will still be able to install
and read the documentation.  If you edit the file ringing/common-am.h to 
reflect what environment will be present when using the library, then you will
also be able to install the header files.]);
fi

dnl --------------------------------------------------------------------------
dnl
dnl See whether we've got GNU readline installed
dnl
AC_ARG_WITH(
  readline,
  AC_HELP_STRING([--with-readline], [support fancy command line editing])
  ac_cv_use_readline=$withval, ac_cv_use_readline=no
)

AC_CACHE_CHECK(
  [for GNU readline],
  [ac_cv_use_readline],
  [ OLD_LIBS=$LIBS
    LIBS="$LIBS -lreadline" 
    AC_TRY_LINK(
    [#include <readline/readline.h>
    ],
    [readline( ">> " );],
    ac_cv_use_readline=yes,
    ac_cv_use_readline=no)
    LIBS=$OLD_LIBS]
)
if test "$ac_cv_use_readline" = yes ; then
  READLINE_LIBS=[-lreadline]
  RINGING_USE_READLINE=1
else
  READLINE_LIBS=[]
  RINGING_USE_READLINE=0
fi
AM_CONDITIONAL(HAVE_READLINE, test "$ac_cv_use_readline" = yes )
AC_SUBST(RINGING_USE_READLINE)
AC_SUBST(READLINE_LIBS)

dnl --------------------------------------------------------------------------
dnl
dnl See whether we're using Microsoft Visual Studio
dnl

# mkdsp automatically sets this Automake conditional to true.  We don't just 
# set it to false in case anyone is using Visual Studio with Autoconf.
AC_CACHE_CHECK(
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
AM_CONDITIONAL(USING_MSVC, test "$ac_cv_msvc" = yes)

dnl --------------------------------------------------------------------------

AC_OUTPUT(Makefile ringing/Makefile apps/Makefile apps/gsiril/Makefile
  apps/utils/Makefile apps/psline/Makefile 
  doc/Makefile examples/Makefile tests/Makefile ringing/common-am.h)
