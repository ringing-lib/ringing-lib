## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = testbase testprint testtouch testlibrary testproof \
testmusic testsearch

LDADD = $(top_builddir)/ringing/libringing.la \
        $(top_builddir)/ringing/libringingcore.la

testbase_SOURCES = testbase.cpp
testprint_SOURCES = testprint.cpp
testtouch_SOURCES = testtouch.cpp
testlibrary_SOURCES = testlibrary.cpp
testproof_SOURCES = testproof.cpp
testmusic_SOURCES = testmusic.cpp
testsearch_SOURCES = testsearch.cpp
