#ident	"@(#)kern-i386at:psm/toolkits/toolkits.mk	1.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = toolkits.mk
DIR = toolkits
KBASE = ../..


all:	local FRC
	for d in `/bin/ls`; do \
		if [ -d $$d ]; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
			 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
		fi; \
	done

local:

install: localinstall FRC
	for d in `/bin/ls`; do \
		if [ -d $$d ]; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
			 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
		fi; \
	done

localinstall: local FRC

clean:	localclean FRC
	for d in `/bin/ls`; do \
		if [ -d $$d ]; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
			 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
		fi; \
	done

localclean:

localclobber:	localclean

clobber:	localclobber FRC
	for d in `/bin/ls`; do \
		if [ -d $$d ]; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
			 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
		fi; \
	done

lintit:	FRC
	for d in `/bin/ls`; do \
		if [ -d $$d ]; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
			 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
		fi; \
	done

fnames:	FRC
	for d in `/bin/ls`; do \
		if [ -d $$d ]; then \
			(cd $$d; \
			 $(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
			 $(SED) -e "s;^;$$d/;"); \
		fi; \
	done

headinstall: localhead FRC
	for d in `/bin/ls`; do \
		if [ -d $$d ]; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
			 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
		fi; \
	done

localhead:

FRC:

depend::
	for d in `/bin/ls`; do \
		if [ -d $$d ]; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk depend"; \
			 $(MAKE) -f $$d.mk depend $(MAKEARGS)); \
		fi; \
	done
