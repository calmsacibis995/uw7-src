#       copyright       "%c%"

#ident	"@(#)stdcmds:stdcmds.mk	1.2.1.1"
#ident	"$Header$"

include $(CMDRULES)

MAKEFILE =	stdcmds.mk

SUBDIRS = suscfg

all:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
		fi; \
	 done

install:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
		fi; \
	 done

clean:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
		fi; \
	 done

clobber:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
		fi; \
	 done


lintit:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
		fi; \
	 done
