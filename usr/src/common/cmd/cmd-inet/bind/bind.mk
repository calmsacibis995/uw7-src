#ident "@(#)bind.mk	1.3"
#ident "$Header$"

include $(CMDRULES)

SUBDIRS = res named tools

all install clean depend:: FRC
	@for x in $(SUBDIRS); do \
		(cd $$x; pwd; $(MAKE) $@); \
	done

$(SUBDIRS):: FRC
	@for x in $@; do \
		(cd $$x; pwd; $(MAKE) all); \
	done

clobber : clean
	@for x in $(SUBDIRS); do \
		(cd $$x; pwd; $(MAKE) $@); \
	done

FRC:
