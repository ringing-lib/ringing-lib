all::

DEPFILES:=$(patsubst %.cpp,.deps/%.d,$(SOURCES))
ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean) \
                 $(findstring $(MAKECMDGOALS), docs)))
-include $(DEPFILES)
endif

MAKEFILE ?= Makefile

.deps/%.d:	%.cpp $(MAKEFILE)
	@mkdir -p .deps
	$(CXX) $(CPPFLAGS) -MM -MT $(<:%.cpp=.objs/%.o) $< -MF $@

.objs/%.o:	%.cpp $(MAKEFILE)
	@mkdir -p .objs
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean::
	rm -rf .objs .deps

docs::

.PHONY: all install clean docs
