## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = testbase testprint
LDADD = $(top_builddir)/ringing/libringing.a

testbase_SOURCES = testbase.cc
testprint_SOURCES = testprint.cc
