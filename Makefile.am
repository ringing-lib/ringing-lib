## Makefile.am -- process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = dist-zip

MAINTAINERCLEANFILES = Makefile.in configure aclocal.m4
EXTRA_DIST = mkdsp

SUBDIRS = ringing apps doc examples tests 

auxdir			= @ac_aux_dir@
AUX_DIST		= $(auxdir)/install-sh $(auxdir)/missing \
			$(auxdir)/mkinstalldirs
dist-hook:
	(cd $(distdir) && mkdir $(auxdir))
	for file in $(AUX_DIST); do \
	  cp $$file $(distdir)/$$file; \
	done
	cd $(distdir) && ./mkdsp 

docdir = $(top_builddir)/doc
html:
	@echo Making $@ in $(docdir)
	@cd $(docdir) && make $@
