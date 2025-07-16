TOPDIR := ..
include $(TOPDIR)/defaults.mk

LDFLAGS += -L.
LDLIBS += -lringingcore

LIBRARY := libringing

# These source files are released under the GPL
SOURCES := mslib.cpp cclib.cpp methodset.cpp extent.cpp group.cpp \
  proof.cpp falseness.cpp touch.cpp row_wildcard.cpp \
  music.cpp print.cpp print_ps.cpp dimension.cpp printm.cpp \
  print_pdf.cpp pdf_fonts.cpp search_base.cpp basic_search.cpp \
  multtab.cpp table_search.cpp streamutils.cpp 

MAKEFILE=main.mk
include $(TOPDIR)/build/lib.mk
