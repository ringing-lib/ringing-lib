# -*- Makefile -*- methsearch/Makefile.am - Automake script for MethSearch

# Process this file with automake to produce Makefile.in

# Copyright (C) 2002, 2003, 2004, 2009, 2011 
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

AUTOMAKE_OPTIONS = dejagnu

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = methsearch

# Need both top_srcdir and top_builddir so that we can find common-am.h
INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/apps/utils

methsearch_LDADD = $(top_builddir)/apps/utils/libstuff.a \
$(top_builddir)/ringing/libringing.la \
$(top_builddir)/ringing/libringingcore.la \
@XERCES_LIBS@

methsearch_SOURCES = prog_args.cpp falseness.cpp format.cpp expression.cpp \
libraries.cpp main.cpp mask.cpp methodutils.cpp music.cpp search.cpp \
output.cpp \
prog_args.h falseness.h format.h expression.h libraries.h mask.h \
methodutils.h music.h search.h output.h

EXTRA_DIST = doc/methsearch.tex

# Configuration for dejagnu
EXTRA_DIST += testsuite/methsearch/basic.exp
DEJATOOL = methsearch
RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir=$$srcdir/testsuite \
   METHSEARCH=`pwd`/$$tool

if HAVE_XELATEX
EXTRA_DIST += doc/methsearch.pdf

pdf:	doc/methsearch.pdf

doc/methsearch.pdf:	$(srcdir)/doc/methsearch.tex
	$(MKDIR_P) doc	# In case $(srcdir) != $(builddir)
	$(XELATEX) --halt-on-error -output-directory doc $<
	cd doc && $(MAKEINDEX) methsearch
	$(XELATEX) --halt-on-error -output-directory doc $<

install-pdf:	pdf
	$(MKDIR_P) ${docdir}
	$(INSTALL_DATA) doc/methsearch.pdf ${docdir}/methsearch.pdf

CLEANFILES = doc/methsearch.aux doc/methsearch.idx doc/methsearch.ilg \
             doc/methsearch.ind doc/methsearch.log doc/methsearch.out \
             doc/methsearch.pdf doc/methsearch.toc

install-data-hook:	install-pdf

.PHONY: pdf install-pdf install
endif
