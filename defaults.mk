# If you want to change any of these variables, you can put them in the 
# environment, add them to the make command line, or create a file named
# local.mk in this directory containing values to override these.

CXXFLAGS ?= -g -O2
PREFIX ?= /usr/local

# Set these to 1 to enable optional functionality.
HAVE_GDOME ?= 0
HAVE_XERCES ?= 0
HAVE_READLINE ?= 0
HAVE_TERMCAP ?= 0

# It is very likely that libgdome will depend on glib.h which may want
# -I/usr/include/glib-2.0 or -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
GDOME_INCLUDES ?= -I/usr/include/libgdome -I/usr/include/libxml2/
GDOME_LIBS ?= -lgdome
XERCES_INCLUDES ?=
XERCES_LIBS ?=

-include $(TOPDIR)/local.mk
