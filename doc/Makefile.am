## Makefile.am - process with automake to generate Makefile.in

MAINTAINERCLEANFILES = Makefile.in

info_TEXINFOS = ringing-lib.texi

SUFFIXES = .html .pdf
html_docs = ringing-lib.html
      
.texi.html:
	$(MAKEINFO) --html $<

.texi.pdf:
	$(TEXI2DVI) --pdf $<

.PHONY: html docdist pdf
html: version.texi $(html_docs)
pdf: version.texi ringing-lib.pdf

dist_docs = $(html_docs) ringing-lib.ps
docdistfile = $(PACKAGE)-doc-$(VERSION).tar.gz
docdistzip = $(PACKAGE)-doc-$(VERSION).zip
docdist: $(dist_docs)
	$(TAR) cozf $(docdistfile) $(dist_docs)
	zip -rq $(docdistzip) $(dist_docs)

DISTCLEANFILES = ringing-lib.html $(docdistfile)
