SUBDIRS := ringing utils apps #tests 

all docs install clean::
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

.PHONY: all docs install clean $(SUBDIRS)
