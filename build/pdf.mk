all docs::	$(DOCUMENT).pdf

$(DOCUMENT).pdf:	$(DOCUMENT).tex
	xelatex --halt-on-error $<
	makeindex $(DOCUMENT)
	xelatex --halt-on-error $<

clean::
	rm -f $(DOCUMENT).aux $(DOCUMENT).log $(DOCUMENT).out $(DOCUMENT).toc
	rm -f $(DOCUMENT).idx $(DOCUMENT).ilg $(DOCUMENT).ind
	rm -f $(DOCUMENT).pdf
