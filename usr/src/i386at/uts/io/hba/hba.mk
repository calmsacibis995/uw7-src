#ident	"@(#)kern-i386at:io/hba/hba.mk	1.27.7.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	hba.mk
KBASE = ../..

SUBDIRS = ide adsc ictha dpt wd7000 mcis mitsumi adsa adse adsl adss ida cpqsc sony lmsi dak blc efp2 fdeb fdsb amd c7xx c8xx zl5380 flashpt qlc1020

all:	FRC
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
		fi; \
	 done

install: FRC
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

fnames:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
	    (cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
		fi; \
	done

FRC:

headinstall: FRC
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) hba.h
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
		fi; \
	 done

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
		fi; \
	done

include $(MAKEFILE).dep
