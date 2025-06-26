TOPDIR := ../..
include $(TOPDIR)/defaults.mk

BINARY := gsiril
SOURCES := main.cpp execution_context.cpp expr_base.cpp proof_context.cpp \
  symbol_table.cpp expression.cpp statement.cpp parser.cpp prog_args.cpp \
  import.cpp functions.cpp 

ifdef HAVE_TERMCAP
CPPFLAGS += -DRINGING_USE_TERMCAP=1
LDLIBS += -ltermcap -lncurses
endif

include $(TOPDIR)/buildbin.mk

init.dat:	escape.awk init/gsiril.init
	awk -f escape.awk init/gsiril.init > $@

.objs/main.o .deps/main.d:	init.dat

clean::
	rm -f init.dat

install::	gsiril
	for link in msiril gsirilic; do \
	  ln -sf gsiril $(PREFIX)/bin/$$link; \
	done

docs clean::
	$(MAKE) -C doc $@
