## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in

# Need both top_srcdir and top_builddir so that we can find common-am.h
INCLUDES = -I$(top_srcdir) -I$(top_builddir) 

check_PROGRAMS = test

TESTS = test

LDADD = $(top_builddir)/ringing/libringing.la \
        $(top_builddir)/ringing/libringingcore.la

test_SOURCES = test-main.cpp test-base.cpp test-base.h \
	change-test.cpp row-test.cpp method-test.cpp music-test.cpp \
	extent-test.cpp
