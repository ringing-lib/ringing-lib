# -*- Makefile -*- gsiril/Makefile.am - Automake script for GSiril

# Process this file with automake to produce Makefile.in

# Copyright (C) 2002, 2003 Richard Smith <richard@ex-parrot.com>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# $Id$

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = gsiril

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/apps/utils

gsiril_LDADD = $(top_builddir)/apps/utils/libstuff.a \
$(top_builddir)/ringing/libringing.a @READLINE_LIBS@

if HAVE_READLINE
additional =  rlstream.cpp rlstream.h
endif

gsiril_SOURCES = main.cpp console_stream.cpp console_stream.h \
execution_context.cpp execution_context.h expression.cpp expression.h \
common_expr.h parser.cpp parser.h util.cpp util.h $(additional) \
msparser.cpp

EXTRA_gsiril_SOURCES = rlstream.cpp rlstream.h
