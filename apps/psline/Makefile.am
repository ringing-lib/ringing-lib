# -*- Makefile -*- 

# Process this file with automake to produce Makefile.in

# Copyright (C) 2002, 2009, 2020 Martin Bright <martin@boojum.org.uk>

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

bin_PROGRAMS = psline
psline_SOURCES = psline.cpp

# Need both top_srcdir and top_builddir so that we can find common-am.h
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/apps/utils

psline_LDADD = $(top_builddir)/apps/utils/libstuff.a \
$(top_builddir)/ringing/libringing.la \
$(top_builddir)/ringing/libringingcore.la
