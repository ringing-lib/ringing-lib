## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = testbase testprint testtouch testlibrary
LDADD = $(top_builddir)/ringing/libringing.la

testbase_SOURCES = testbase.cpp
testprint_SOURCES = testprint.cpp
testtouch_SOURCES = testtouch.cpp
testlibrary_SOURCES = testlibrary.cpp
