TOPDIR := ../..
include $(TOPDIR)/defaults.mk

BINARY := methsearch
SOURCES := prog_args.cpp falseness.cpp format.cpp expression.cpp \
  libraries.cpp main.cpp mask.cpp methodutils.cpp music.cpp search.cpp \
  output.cpp

include $(TOPDIR)/buildbin.mk

docs clean::
	$(MAKE) -C doc $@
