## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

bin_PROGRAMS = psline
noinst_PROGRAMS = testbase testprint testtouch testlibrary testproof \
testmusic testsearch

LDADD = $(top_builddir)/ringing/libringing.a

testbase_SOURCES = testbase.cpp
testprint_SOURCES = testprint.cpp
testtouch_SOURCES = testtouch.cpp
testlibrary_SOURCES = testlibrary.cpp
psline_SOURCES = psline.cpp args.cpp
testproof_SOURCES = testproof.cpp
testmusic_SOURCES = testmusic.cpp
testsearch_SOURCES = testsearch.cpp
