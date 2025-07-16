# If you want to change any of these variables, you can put them in the 
# environment, add them to the make command line, or create a file named
# local.mk in this directory containing values to override these.

CXXFLAGS ?= -g -O2
PREFIX ?= /usr/local

# Each of these libraries has an XXX_INCLUDES variable (for setting -I
# options to find headers or -D options to add custom definitions), an
# XXX_LDFLAGS variable (for passing -L options to locate libraries), and
# an XXX_LIBS option (for -l options linking to the library and any 
# other dependent libraries).

# Some builds of libgdome depend on glib.h which may want
# -I/usr/include/glib-2.0 or -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
GDOME_INCLUDES ?= -I/usr/include/libgdome -I/usr/include/libxml2/
GDOME_LIBS ?= -lgdome
XERCES_LIBS ?= -lxerces-c
READLINE_LIBS ?= -lreadline
TERMCAP_LIBS ?= -ltermcap -lncurses

-include $(TOPDIR)/local.mk
