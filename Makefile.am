## Makefile.am -- process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = dist-zip

MAINTAINERCLEANFILES = Makefile.in configure aclocal.m4
EXTRA_DIST = mkdsp

SUBDIRS = ringing apps doc examples tests 

dist-hook:
	cd $(distdir) && ./mkdsp 

docdir = $(top_builddir)/doc
html:
	@echo Making $@ in $(docdir)
	@cd $(docdir) && make $@
