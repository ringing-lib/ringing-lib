## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

check_PROGRAMS = test

TESTS = test

LDADD = $(top_builddir)/ringing/libringing.la \
        $(top_builddir)/ringing/libringingcore.la

test_SOURCES = test-main.cpp test-base.cpp test-base.h \
	change-test.cpp row-test.cpp method-test.cpp music-test.cpp \
	extent-test.cpp
