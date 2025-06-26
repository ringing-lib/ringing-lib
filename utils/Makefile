TOPDIR := ..
include $(TOPDIR)/defaults.mk

LDFLAGS += -L$(TOPDIR)/ringing
LDLIBS += -lringingcore -lringing

LIBRARY := libringingutils

SOURCES := args.cpp tokeniser.cpp stringutils.cpp exec.cpp row_calc.cpp \
  console_stream.cpp argv.cpp bell_fmt.cpp

ifdef HAVE_READLINE
SOURCES += rlstream.cpp
CPPFLAGS += -DRINGING_USE_READLINE=1
LDLIBS += -lreadline
endif

ifdef HAVE_TERMCAP
CPPFLAGS += -DRINGING_USE_TERMCAP=1
LDLIBS += -ltermcap -lncurses
endif

include $(TOPDIR)/buildlib.mk
