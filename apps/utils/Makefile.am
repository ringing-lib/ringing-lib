# -*- Makefile -*- 

# Process this file with automake to produce Makefile.in

# Copyright (C) 2002, 2009, 2010, 2020 Martin Bright <martin@boojum.org.uk> and
# Richard Smith <richard@ex-parrot.com>

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

MAINTAINERCLEANFILES = Makefile.in

noinst_LIBRARIES = libstuff.a

# Need both top_srcdir and top_builddir so that we can find common-am.h
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)

if HAVE_READLINE
additional =  rlstream.cpp rlstream.h
endif

libstuff_a_SOURCES = args.cpp args.h tokeniser.cpp tokeniser.h init_val.h \
stringutils.h stringutils.cpp exec.cpp exec.h row_calc.cpp row_calc.h \
console_stream.h console_stream.cpp argv.cpp $(additional)

EXTRA_libstuff_a_SOURCES = rlstream.cpp rlstream.h

