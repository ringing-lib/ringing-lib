dnl -*- M4 -*- configure.ac - Autoconf script for the Ringing Class Library

dnl Process this file with autoconf to produce a configure script.

dnl Copyright (C) 2001, 2002, 2008 Martin Bright <martin@boojum.org.uk>
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
AM_INIT_AUTOMAKE(ringing-lib, 0.3.0)
AC_SUBST(ac_aux_dir)

can_build=yes

dnl --------------------------------------------------------------------------
dnl Checks for programs.
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_LN_S

dnl --------------------------------------------------------------------------
dnl Checks for libraries.
AC_USE_TERMCAP
AC_SUBST(USE_TERMCAP)
AC_SUBST(TERMCAP_LIBS)

AC_USE_READLINE
AM_CONDITIONAL(HAVE_READLINE, test "$USE_READLINE" = 1 )
AC_SUBST(USE_READLINE)
AC_SUBST(READLINE_NEEDS_STDIO_H)
AC_SUBST(READLINE_LIBS)

AC_USE_XERCES
AM_CONDITIONAL(HAVE_XERCES, test "$USE_XERCES" = 1 )
AC_SUBST(USE_XERCES)
AC_SUBST(XERCES_LIBS)

dnl --------------------------------------------------------------------------
dnl Checks for header files.
AC_HEADER_STDC

AC_CXX_OLD_INCLUDES
AC_SUBST(OLD_INCLUDES)

AC_CXX_OLD_C_INCLUDES
AC_SUBST(OLD_C_INCLUDES)

AC_CXX_HAVE_OLD_IOSTREAMS
AC_SUBST(HAVE_OLD_IOSTREAMS)

dnl --------------------------------------------------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.
AC_CXX_EXCEPTIONS
AC_SUBST(USE_EXCEPTIONS)

AC_CXX_NAMESPACES
AC_SUBST(USE_NAMESPACES)

AC_CXX_FUNCTION_TEMPLATE_SPECIALISATION
AC_SUBST(USE_TEMPLATE_FUNCTION_SPECIALISATION)

AC_CXX_PREMATURE_MEMBER_INSTANTIATION
AC_SUBST(PREMATURE_MEMBER_INSTANTIATION)

AC_CXX_PROTECTED_MEMBER_BASES
AC_SUBST(PROTECTED_MEMBER_BASES)

AC_CXX_USE_STD
AC_SUBST(USE_STD)

AC_CXX_HAVE_STD_ITERATOR
AC_SUBST(HAVE_STD_ITERATOR)

AC_CXX_USE_STRINGSTREAM
AC_SUBST(USE_STRINGSTREAM)

dnl --------------------------------------------------------------------------
dnl Report any fatal errors
if test "$can_build" = no; then
  AC_MSG_WARN([Some problems were encountered which mean that you will not
be able to build the Ringing Class Library.  You will still be able to install
and read the documentation.  If you edit the file ringing/common-am.h to 
reflect what environment will be present when using the library, then you will
also be able to install the header files.]);
fi

dnl --------------------------------------------------------------------------

# mkdsp automatically sets this Automake conditional to true.  We don't just 
# set it to false in case anyone is using Visual Studio with Autoconf.
AC_IS_MSVC
AM_CONDITIONAL(USING_MSVC, test "$IS_MSVC" = 1)

dnl --------------------------------------------------------------------------

AC_OUTPUT(Makefile ringing/Makefile apps/Makefile apps/gsiril/Makefile
  apps/utils/Makefile apps/psline/Makefile apps/methsearch/Makefile
  apps/fextent/Makefile apps/extent/Makefile apps/touchsearch/Makefile
  doc/Makefile examples/Makefile tests/Makefile ringing/common-am.h)
