# -*- Makefile -*-

# Process this file with automake to produce Makefile.in

# Copyright (C) 2009 Richard Smith <richard@ex-parrot.com>

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

bin_PROGRAMS = rowcalc

# Need both top_srcdir and top_builddir so that we can find common-am.h
INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/apps/utils

rowcalc_LDADD = $(top_builddir)/apps/utils/libstuff.a \
$(top_builddir)/ringing/libringing.la \
$(top_builddir)/ringing/libringingcore.la

rowcalc_SOURCES = rowcalc.cpp

