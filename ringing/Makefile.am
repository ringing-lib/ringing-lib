## Makefile.am - process with automake to generate Makefile.in

EXTRA_DIST = common.h.in
DISTCLEANFILES = common.h
MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

lib_LIBRARIES = libringing.a
libringing_a_SOURCES = row.cpp method.cpp library.cpp mslib.cpp cclib.cpp \
proof.cpp music.cpp print.cpp print_ps.cpp dimension.cpp touch.cpp \
printm.cpp print_pdf.cpp pdf_fonts.cpp

myincludedir = $(includedir)/ringing
myinclude_HEADERS = macros.h row.h method.h library.h mslib.h cclib.h proof.h \
music.h print.h print_ps.h dimension.h stuff.h common.h touch.h printm.h \
print_pdf.h pdf_fonts.h

# Delete common.h before packaging up the distribution
dist-hook:
	rm -f $(distdir)/common.h
