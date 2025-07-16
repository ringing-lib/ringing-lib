CPPFLAGS += -I$(TOPDIR)
CXXFLAGS += -fPIC

include $(TOPDIR)/build/compile.mk

all::	$(LIBRARY).so

$(LIBRARY).so:	$(SOURCES:%.cpp=.objs/%.o)
	$(CXX) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

install::	$(LIBRARY).so
	install $^ $(PREFIX)/lib/

clean::
	rm -f $(LIBRARY).so
