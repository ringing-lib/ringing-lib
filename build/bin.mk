CPPFLAGS += -I$(TOPDIR) -I$(TOPDIR)/utils
LDFLAGS += -L$(TOPDIR)/ringing -L$(TOPDIR)/utils
LDLIBS += -lringingcore -lringing -lringingutils

include $(TOPDIR)/build/compile.mk

all::	$(BINARY)

$(BINARY):	.objs/$(BINARY)
	sed 's!@TOPDIR@!$(TOPDIR)!g' $(TOPDIR)/build/run.sh > $@
	chmod a+x $@

.objs/$(BINARY):	$(SOURCES:%.cpp=.objs/%.o)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install::	.objs/$(BINARY)
	mkdir -p $(PREFIX)/bin/
	install $^ $(PREFIX)/bin/

clean::
	rm -f $(BINARY)
