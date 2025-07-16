TOPDIR := ..
include $(TOPDIR)/defaults.mk

LIBRARY := libringingcore

# These source files are released under the LGPL
SOURCES := bell.cpp change.cpp row.cpp mathutils.cpp \
  place_notation.cpp method.cpp methodset.cpp method_stream.cpp \
  library.cpp libfacet.cpp libout.cpp litelib.cpp \
  xmllib.cpp xmlout.cpp peal.cpp \
  lexical_cast.cpp stl.cpp

# XML support needs either Gdome2 or Xerces.
# If you want GDOME, define HAVE_GDOME, either in the environment or by
# editing the make file; similarly, if you want Xerces, define HAVE_XERCES.
# Otherwise the library will be built without XML support.
ifdef HAVE_GDOME
SOURCES += dom_gdome.cpp
CPPFLAGS += $(GDOME_INCLUDES)
LDFLAGS += $(GDOME_LDFLAGS)
LDLIBS += $(GDOME_LIBS)
else 
ifdef HAVE_XERCES
SOURCES += dom_xerces.cpp
CPPFLAGS += $(XERCES_INCLUDES)
LDFLAGS += $(XERCES_LDFLAGS)
LDLIBS += $(XERCES_LIBS)
else
SOURCES += dom_stub.cpp
endif
endif

MAKEFILE=core.mk
include $(TOPDIR)/build/lib.mk
