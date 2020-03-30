## Makefile.am - process with automake to generate Makefile.in

EXTRA_DIST = common-am.h.in
DISTCLEANFILES = common-am.h
MAINTAINERCLEANFILES = Makefile.in

lib_LTLIBRARIES = libringingcore.la libringing.la

# These source files are released under the LGPL
libringingcore_la_SOURCES = bell.cpp change.cpp row.cpp mathutils.cpp \
place_notation.cpp method.cpp methodset.cpp \
library.cpp libfacet.cpp libout.cpp litelib.cpp \
xmllib.cpp xmlout.cpp peal.cpp \
lexical_cast.cpp stl.cpp

# These source files are released under the GPL
libringing_la_SOURCES = \
mslib.cpp cclib.cpp methodset.cpp extent.cpp group.cpp proof.cpp \
falseness.cpp falseness.dat touch.cpp row_wildcard.cpp music.cpp \
print.cpp print_ps.cpp dimension.cpp printm.cpp print_pdf.cpp pdf_fonts.cpp \
search_base.cpp basic_search.cpp multtab.cpp table_search.cpp streamutils.cpp 

libringingcore_la_LIBADD =
libringing_la_LIBADD = $(top_builddir)/ringing/libringingcore.la 

libringingcore_la_LDFLAGS =
libringing_la_LDFLAGS =

# Need both top_srcdir and top_builddir so that we can find common-am.h
libringingcore_la_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)
libringing_la_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)

if DLL_SUPPORT
# -no-undefined is needed for DLLs
libringingcore_la_LDFLAGS += -no-undefined
libringing_la_LDFLAGS += -no-undefined

# Preprocessor hooks to DLL export symbols
libringingcore_la_CPPFLAGS += -DRINGING_EXPORTS
libringing_la_CPPFLAGS += -DRINGING_EXPORTS
endif

# Select the right XML library
if HAVE_GDOME
libringingcore_la_SOURCES += dom_gdome.cpp
libringingcore_la_LIBADD += @GDOME_LIBS@
libringingcore_la_CPPFLAGS += @GDOME_CFLAGS@
libringingcore_la_LDFLAGS += @GDOME_LDFLAGS@
else 
if HAVE_XERCES
libringingcore_la_SOURCES += dom_xerces.cpp
libringingcore_la_LIBADD += @XERCES_LIBS@
else
libringingcore_la_SOURCES += dom_stub.cpp
endif
endif


# TODO: Split this into a list of GPL'd and LGPL'd files.
myincludedir = $(includedir)/ringing
myinclude_HEADERS = macros.h row.h method.h library.h mslib.h cclib.h proof.h \
music.h print.h print_ps.h dimension.h common.h common-am.h common-msvc.h \
touch.h printm.h print_pdf.h pdf_fonts.h pointers.h extent.h falseness.h \
search_base.h basic_search.h multtab.h table_search.h streamutils.h \
xmllib.h group.h libfacet.h peal.h xmlout.h libout.h mathutils.h bell.h \
change.h place_notation.h litelib.h dom.h libbase.h methodset.h \
lexical_cast.h istream_impl.h row_wildcard.h iteratorutils.h

# Delete common-am.h before packaging up the distribution
dist-hook:
	rm -f $(distdir)/common-am.h
