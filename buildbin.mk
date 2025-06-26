CPPFLAGS += -I$(TOPDIR) -I$(TOPDIR)/utils
LDFLAGS += -L$(TOPDIR)/ringing -L$(TOPDIR)/utils
LDLIBS += -lringingcore -lringing -lringingutils

include $(TOPDIR)/buildrules.mk

all::	$(BINARY)

$(BINARY):	$(SOURCES:%.cpp=.objs/%.o)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install::	$(BINARY)
	install $^ $(PREFIX)/bin/

clean::
	rm -f $(BINARY)
