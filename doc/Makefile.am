## Makefile.am - process with automake to generate Makefile.in

MAINTAINERCLEANFILES = Makefile.in

info_TEXINFOS = ringing-lib.texi

SUFFIXES = .html
html_docs = ringing-lib.html
      
.texi.html:
	$(MAKEINFO) --html $<

.PHONY: html docdist
html: version.texi $(html_docs)

dist_docs = $(html_docs) ringing-lib.ps
docdistfile = $(PACKAGE)-doc-$(VERSION).tar.gz
docdist: $(dist_docs)
	$(TAR) cozf $(docdistfile) $(dist_docs)

DISTCLEANFILES = ringing-lib.html $(docdistfile)
