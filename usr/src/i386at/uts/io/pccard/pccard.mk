#ident	"@(#)kern-i386at:io/pccard/pccard.mk	1.1"

include $(UTSRULES)

MAKEFILE = pccard.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
SUBDIRS = pcic

.SUFFIXES: .ln

.c.ln:

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

clobber: clean
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		$(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
		fi; \
	done

$(LINTDIR):

lintit:
	@for d in $(LINTDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		$(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
		fi; \
	done

fnames:

odiHeaders = 

headinstall:
	:

depend::
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		touch $$d.mk.dep;\
		$(MAKE) -f $$d.mk depend $(MAKEARGS));\
		fi; \
	done
