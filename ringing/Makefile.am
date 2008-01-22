## Makefile.am - process with automake to generate Makefile.in

EXTRA_DIST = common-am.h.in
DISTCLEANFILES = common-am.h
MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES = libringingcore.la libringing.la

if HAVE_XERCES
additional = dom_xerces.cpp 
else
additional = dom_stub.cpp
endif

# These source files are released under the LGPL
libringingcore_la_SOURCES = bell.cpp change.cpp row.cpp mathutils.cpp \
place_notation.cpp method.cpp \
library.cpp libfacet.cpp libout.cpp litelib.cpp \
xmllib.cpp xmlout.cpp peal.cpp \
stl.cpp $(additional)

EXTRA_libringingcore_la_SOURCES = dom_xerces.cpp dom_stub.cpp

# These source files are released under the GPL
libringing_la_SOURCES = \
mslib.cpp cclib.cpp extent.cpp group.cpp proof.cpp \
falseness.cpp falseness.dat touch.cpp music.cpp \
print.cpp print_ps.cpp dimension.cpp printm.cpp print_pdf.cpp pdf_fonts.cpp \
search_base.cpp basic_search.cpp multtab.cpp table_search.cpp streamutils.cpp 

# Link against our XML library
libringingcore_la_LIBADD = @XERCES_LIBS@
libringing_la_LIBADD = $(top_builddir)/ringing/libringingcore.la 

# -no-undefined is needed for DLLs, and doesn't hurt elsewhere
libringingcore_la_LDFLAGS = -no-undefined
libringing_la_LDFLAGS = -no-undefined

# Preprocessor hooks to DLL export symbols
libringingcore_la_CPPFLAGS = -DRINGING_EXPORTS
libringing_la_CPPFLAGS = -DRINGING_EXPORTS

# TODO: Split this into a list of GPL'd and LGPL'd files.
myincludedir = $(includedir)/ringing
myinclude_HEADERS = macros.h row.h method.h library.h mslib.h cclib.h proof.h \
music.h print.h print_ps.h dimension.h common.h common-am.h common-msvc.h \
touch.h printm.h print_pdf.h pdf_fonts.h pointers.h extent.h falseness.h \
search_base.h basic_search.h multtab.h table_search.h streamutils.h \
xmllib.h group.h libfacet.h peal.h xmlout.h libout.h mathutils.h bell.h \
change.h place_notation.h litelib.h dom.h

# Delete common-am.h before packaging up the distribution
dist-hook:
	rm -f $(distdir)/common-am.h
