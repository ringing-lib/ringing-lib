## Makefile.am - process with automake to generate Makefile.in

EXTRA_DIST = common.h.in
DISTCLEANFILES = common.h
MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

lib_LIBRARIES = libringing.a
libringing_a_SOURCES = row.cpp row.h method.cpp method.h library.cpp \
library.h mslib.cpp mslib.h cclib.cpp cclib.h proof.cpp proof.h music.h \
music.cpp print.cpp print.h print_ps.cpp print_ps.h dimension.cpp \
dimension.h stuff.h touch.h touch.cpp

myincludedir = $(includedir)/ringing
myinclude_HEADERS = row.h method.h library.h mslib.h cclib.h proof.h \
music.h print.h print_ps.h dimension.h stuff.h common.h touch.h

# Delete common.h before packaging up the distribution
dist-hook:
	rm -f $(distdir)/common.h
