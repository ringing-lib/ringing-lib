## Makefile.am - process with automake to generate Makefile.in

info_TEXINFOS = ringing-lib.texi

SUFFIXES = .html
html_docs = ringing-lib.html
      
.texi.html:
	$(MAKEINFO) --html $<

.PHONY: html
html: version.texi $(html_docs)

DISTCLEANFILES = ringing-lib.html
