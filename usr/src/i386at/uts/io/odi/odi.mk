#ident	"@(#)odi.mk	2.1"

include $(UTSRULES)

MAKEFILE = odi.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
SUBDIRS = lsl msm tsm odisr compat odimem

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

odiHeaders = \
	odi.h

headinstall:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		$(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
		fi; \
	done
	@for f in $(odiHeaders);     \
	do                              \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)    \
			-g $(GRP) $$f;  \
	done

depend::
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		touch $$d.mk.dep;\
		$(MAKE) -f $$d.mk depend $(MAKEARGS));\
		fi; \
	done
