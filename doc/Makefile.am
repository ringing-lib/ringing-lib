## Makefile.am - process with automake to generate Makefile.in

MAINTAINERCLEANFILES = Makefile.in

# Automake doesn't support conditional building of info_TEXINFOS 
# so we cannot wrap this in `if HAVE_MAKEINFO'.  See bug 191 in
# the Automake GNATS database.  This means that a working MAKEINFO
# is required.
info_TEXINFOS = ringing-lib.texi

SUFFIXES = .html .pdf
      
.texi.pdf:
	$(TEXI2DVI) --pdf $<

.PHONY: html docdist pdf
html: version.texi ringing-lib.html ringing-lib/index.html
pdf: version.texi ringing-lib.pdf

ringing-lib.html: ringing-lib.texi
	$(MAKEINFO) --html --no-split $<

ringing-lib/index.html : ringing-lib.texi
	$(MAKEINFO) --html $<

dist_docs = ringing-lib.pdf ringing-lib
dist_docs_dep = ringing-lib.pdf ringing-lib/index.html

docdistfile = $(PACKAGE)-doc-$(VERSION).tar.gz
docdistzip = $(PACKAGE)-doc-$(VERSION).zip
docdist: $(dist_docs_dep)
	$(TAR) cozf $(docdistfile) $(dist_docs)
	zip -rq $(docdistzip) $(dist_docs)

DISTCLEANFILES = ringing-lib.html $(docdistfile) $(docdistzip) ringing-lib/*
