# -*- Makefile -*- gsiril/Makefile.am - Automake script for GSiril

# Process this file with automake to produce Makefile.in

# Copyright (C) 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2020
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

bin_PROGRAMS = gsiril

# Need both top_srcdir and top_builddir so that we can find common-am.h
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/apps/utils

gsiril_LDADD = $(top_builddir)/apps/utils/libstuff.a \
$(top_builddir)/ringing/libringing.la \
$(top_builddir)/ringing/libringingcore.la \
@READLINE_LIBS@ @TERMCAP_LIBS@

gsiril_SOURCES = main.cpp \
execution_context.cpp execution_context.h expr_base.cpp expr_base.h \
proof_context.cpp proof_context.h symbol_table.cpp symbol_table.h \
expression.h expression.cpp statement.h statement.cpp \
parser.cpp parser.h prog_args.h prog_args.cpp import.cpp $(additional)

EXTRA_gsiril_SOURCES = rlstream.cpp rlstream.h

# Build init.dat from the file in init/
$(builddir)/main.o:	$(builddir)/init.dat

$(builddir)/init.dat:	$(srcdir)/escape.awk $(srcdir)/init/gsiril.init
	$(AWK) -f $(srcdir)/escape.awk $(srcdir)/init/gsiril.init > $@

CLEANFILES = $(builddir)/init.dat


# When gsiril is run under the name msiril, it automatically runs
# in MicroSiril compatibility mode.  Create a symlink to assist
# with this.  Similarly for sirilic and Sirilic compatibility mode.
gsiril_SYMLINKS = msiril gsirilic

install-exec-hook:
	for link in $(gsiril_SYMLINKS); do \
	  cd $(DESTDIR)$(bindir) && $(RM) $$link$(EXEEXT) && \
	    $(LN_S) gsiril$(EXEEXT) $$link$(EXEEXT); \
	done

uninstall-hook:
	for link in $(gsiril_SYMLINKS); do \
	  cd $(DESTDIR)$(bindir) && $(RM) $$link$(EXEEXT); \
	done

# Configuration for dejagnu
EXTRA_DIST = testsuite/config/unix.exp testsuite/gsiril/basic.exp
DEJATOOL = gsiril
RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir=$$srcdir/testsuite \
   GSIRIL=`pwd`/$$tool

if HAVE_HELP2MAN
dist_man_MANS = gsiril.1

# Man page.  (See documentation for distcleancheck in automake manual
# for the reasons for not making gsiril.1 depend on gsiril$(EXEEXT).)
gsiril.1: $(srcdir)/prog_args.cpp $(srcdir)/Makefile.am
	$(MAKE) $(AM_MAKEFLAGS) gsiril$(EXEEXT)
	$(HELP2MAN) -N --output=gsiril.1 ./gsiril$(EXEEXT)

CLEANFILES += gsiril.1
endif
