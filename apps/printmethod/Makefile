TOPDIR := ../..
include $(TOPDIR)/defaults.mk

ifdef HAVE_TERMCAP
CPPFLAGS += -DRINGING_USE_TERMCAP=1 $(TERMCAP_INCLUDES)
LDFLAGS += $(TERMCAP_LDFLAGS)
LDLIBS += $(TERMCAP_LIBS)
endif

BINARY := printmethod
SOURCES := main.cpp

include $(TOPDIR)/build/bin.mk
