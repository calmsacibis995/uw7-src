#ident	"@(#)priv.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	priv.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/priv

SUBDIRS = sum lpm

all:
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk all"; \
		( cd $$i; $(MAKE) -f $$i.mk all $(MAKEARGS)	); \
	done

install:
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk install"; \
		( cd $$i; $(MAKE) -f $$i.mk install $(MAKEARGS) ); \
	done

clean:
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk clean"; \
		( cd $$i; $(MAKE) -f $$i.mk clean $(MAKEARGS) ); \
	done

clobber: clean FRC
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk clobber"; \
		( cd $$i; $(MAKE) -f $$i.mk clobber $(MAKEARGS) ); \
	done
	
lintit:
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk lintit"; \
		( cd $$i; $(MAKE) -f $$i.mk lintit $(MAKEARGS) ); \
	done

fnames:
	@for i in $(SUBDIRS);\
	do\
		( \
		cd $$i;\
		$(MAKE) -f $$i.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$i/;" ; \
		) \
	done

headinstall: localhead FRC
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk headinstall"; \
		( cd $$i; $(MAKE) -f $$i.mk headinstall $(MAKEARGS) ); \
	done

sysHeaders = \
	privilege.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep
