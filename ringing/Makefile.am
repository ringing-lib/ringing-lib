## Makefile.am - process with automake to generate Makefile.in

EXTRA_DIST = common-am.h.in
DISTCLEANFILES = common-am.h
MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

lib_LIBRARIES = libringing.a
libringing_a_SOURCES = row.cpp method.cpp library.cpp mslib.cpp cclib.cpp \
proof.cpp music.cpp print.cpp print_ps.cpp dimension.cpp touch.cpp \
printm.cpp print_pdf.cpp pdf_fonts.cpp stl.cpp extent.cpp falseness.cpp \
search_base.cpp basic_search.cpp multtab.cpp table_search.cpp streamutils.cpp

myincludedir = $(includedir)/ringing
myinclude_HEADERS = macros.h row.h method.h library.h mslib.h cclib.h proof.h \
music.h print.h print_ps.h dimension.h common.h common-am.h common-msvc.h \
touch.h printm.h print_pdf.h pdf_fonts.h pointers.h extent.h falseness.h \
search_base.h basic_search.h multtab.h table_search.h streamutils.h


# Delete common-am.h before packaging up the distribution
dist-hook:
	rm -f $(distdir)/common-am.h
