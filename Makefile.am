## Makefile.am -- process this file with automake to produce Makefile.in

SUBDIRS = ringing doc examples

MAINTAINERCLEANFILES = Makefile.in configure aclocal.m4
EXTRA_DIST = mkdsp

auxdir			= @ac_aux_dir@
AUX_DIST		= $(auxdir)/install-sh $(auxdir)/missing \
			$(auxdir)/mkinstalldirs
dist-hook:
	(cd $(distdir) && mkdir $(auxdir))
	for file in $(AUX_DIST); do \
	  cp $$file $(distdir)/$$file; \
	done
	cd $(distdir) && ./mkdsp 

dist-zip: distdir
	chmod -R a+r $(distdir)
	zip -lr $(distdir).zip $(distdir)
	-rm -rf $(distdir)

dist-all: distdir
	chmod -R a+r $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) chozf $(distdir).tar.gz $(distdir)
	zip -lr $(distdir).zip $(distdir)
	-rm -rf $(distdir)

docdir = $(top_builddir)/doc
html:
	@echo Making $@ in $(docdir)
	@cd $(docdir) && make $@
