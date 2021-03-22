## Makefile.am -- process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = dist-zip
ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = Makefile.in configure aclocal.m4 \
                       $(distdir).tar.gz $(distdir).zip

EXTRA_DIST = mkdsp

SUBDIRS = ringing apps doc tests 

dist-hook:
	cd $(distdir) && ./mkdsp 

docdir = $(top_builddir)/doc
html:
	@echo Making $@ in $(docdir)
	@cd $(docdir) && make $@
