## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = test

TESTS = test

LDADD = $(top_builddir)/ringing/libringing.a

test_SOURCES = test-main.cpp test-base.cpp test-base.h \
	change-test.cpp row-test.cpp method-test.cpp
