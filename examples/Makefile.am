## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = testbase testprint testtouch testlibrary psline testproof
LDADD = $(top_builddir)/ringing/libringing.a

testbase_SOURCES = testbase.cpp
testprint_SOURCES = testprint.cpp
testtouch_SOURCES = testtouch.cpp
testlibrary_SOURCES = testlibrary.cpp
psline_SOURCES = psline.cpp
testproof_SOURCES = testproof.cpp

