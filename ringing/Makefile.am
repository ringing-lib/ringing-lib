## Makefile.am - process with automake to generate Makefile.in

EXTRA_DIST = common.h.in
DISTCLEANFILES = common.h

lib_LIBRARIES = libringing.a
libringing_a_SOURCES = row.cc row.h method.cc method.h library.cc library.h \
mslib.cc mslib.h print.cc print.h print_ps.cc print_ps.h dimension.cc \
dimension.h stuff.h

myincludedir = $(includedir)/ringing
myinclude_HEADERS = row.h method.h library.h mslib.h print.h print_ps.h \
dimension.h stuff.h common.h

# Delete common.h before packaging up the distribution
dist-hook:
	rm -f $(distdir)/common.h