## Makefile.am - process with automake to generate Makefile.in

EXTRA_DIST = common-am.h.in
DISTCLEANFILES = common-am.h
MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES = libringingcore.la libringing.la

# These source files are released under the LGPL
libringingcore_la_SOURCES = bell.cpp change.cpp row.cpp mathutils.cpp \
place_notation.cpp method.cpp \
library.cpp libfacet.cpp libout.cpp xmllib.cpp xmlout.cpp peal.cpp \
stl.cpp

# These source files are released under the GPL
libringing_la_SOURCES = $(libringingcore_la_SOURCES) \
mslib.cpp cclib.cpp extent.cpp group.cpp proof.cpp \
falseness.cpp falseness.dat touch.cpp music.cpp \
print.cpp print_ps.cpp dimension.cpp printm.cpp print_pdf.cpp pdf_fonts.cpp \
search_base.cpp basic_search.cpp multtab.cpp table_search.cpp streamutils.cpp 

# Link against Xerces; -no-undefined is needed for DLLs
libringingcore_la_LIBADD = @XERCES_LIBS@
libringing_la_LIBADD = @XERCES_LIBS@

# -no-undefined is needed for DLLs
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
change.h place_notation.h

# Delete common-am.h before packaging up the distribution
dist-hook:
	rm -f $(distdir)/common-am.h
