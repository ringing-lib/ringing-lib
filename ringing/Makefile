TOPDIR := ..
include $(TOPDIR)/defaults.mk

all docs install clean::
	for lib in core main; do \
	  $(MAKE) -f $$lib.mk $@; \
	done

libringingcore.so:
	$(MAKE) -f core.mk $@

libringing.so:
	$(MAKE) -f main.mk $@

HEADERS = macros.h row.h method.h library.h mslib.h cclib.h proof.h \
music.h print.h print_ps.h dimension.h common.h common-am.h common-msvc.h \
touch.h printm.h print_pdf.h pdf_fonts.h pointers.h extent.h falseness.h \
search_base.h basic_search.h multtab.h table_search.h streamutils.h \
xmllib.h group.h libfacet.h peal.h xmlout.h libout.h mathutils.h bell.h \
change.h place_notation.h litelib.h dom.h libbase.h methodset.h \
lexical_cast.h istream_impl.h row_wildcard.h iteratorutils.h method_stream.h

install::
	mkdir -p $(PREFIX)/include/ringing/
	install -m644 $(HEADERS) $(PREFIX)/include/ringing/
