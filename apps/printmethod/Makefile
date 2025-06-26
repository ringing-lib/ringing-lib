TOPDIR := ../..
include $(TOPDIR)/defaults.mk

ifdef HAVE_TERMCAP
CXXFLAGS += -DRINGING_USE_TERMCAP=1
LDLIBS += -ltermcap -lncurses
endif

BINARY := printmethod
SOURCES := main.cpp

include $(TOPDIR)/buildbin.mk
