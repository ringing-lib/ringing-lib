
SUBDIRS := gsiril psline methsearch fextent extent touchsearch \
  printmethod musgrep rowcalc ringmethod splices spliceplan methodlib \
  psrows

all docs install clean::
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@ || exit; \
	done

.PHONY: all docs install clean $(SUBDIRS)
